#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>

#define PAGE_SIZE 4096
#define BUF_SIZE 512
#define master_IOCTL_CREATESOCK 0x12345677
#define master_IOCTL_MMAP 0x12345678
#define master_IOCTL_EXIT 0x12345679
//#define master_IOCTL_FILESIZE 0x12345688 

void help_message(); // print the help message
size_t get_filesize(const char* filename); // get the size of the input file

int main (int argc, char* argv[])
{
	char buf[BUF_SIZE];
    int dev_fd, file_fd;// the fd for the device and the fd for the input file
    size_t ret, file_size, offset = 0, tmp;
    int n_files;
    char *file_name;
    char *kernel_address = NULL, *file_address = NULL;
    struct timeval start;
    struct timeval end;
    double trans_time; //calulate the time between the device is opened and it is closed

    // Deal with input parameters
    if (argc < 4)
    {
        help_message();
        return 1;
    }
    n_files = atoi(argv[1]);
    if (n_files <= 0 || n_files != argc - 3)
    {
        help_message();
        return 1;
    }
    const char *method = argv[argc - 1];

    // ==============================

    if( (dev_fd = open("/dev/master_device", O_RDWR)) < 0)
    {
        perror("failed to open /dev/master_device\n");
        return 1;
    }

	for (int i = 2; n_files > 0; ++i, --n_files)
    {
        file_name = argv[i];
        if( (file_fd = open (file_name, O_RDWR)) < 0 )
        {
            perror("failed to open input file\n");
            return 1;
        }

        if( (file_size = get_filesize(file_name)) < 0)
        {
            perror("failed to get filesize\n");
            return 1;
        }

        gettimeofday(&start ,NULL);
        if(ioctl(dev_fd, master_IOCTL_CREATESOCK) == -1) //0x12345677 : create socket and accept the connection from the slave
        {
            perror("ioctl server create socket error\n");
            return 1;
        }

        switch(method[0])
        {
            case 'f': //fcntl : read()/write()
                // read from the input file
                while ((ret = read(file_fd, buf, sizeof(buf))) > 0)
                {
                    write(dev_fd, buf, ret); // write to the device
                }
                break;

            case 'm': ;// mmap
                size_t pageoff = 0, diff = 0;
				char *src;
				// if(ioctl(dev_fd, master_IOCTL_MMAP) == -1) // Send file size to slave device
				// {
				// 	perror("failed to init mmap\n");
				// 	return 1;
				// }
				send_filesize(dev_fd, &file_size);
				src = mmap(NULL, file_size, PROT_READ|PROT_WRITE, MAP_SHARED, file_fd, 0);
				char *tmp = src;
				while(pageoff < file_size)
				{
					// read from file
					diff = file_size - pageoff;
					if(diff > BUF_SIZE) 
					{
						// read BUF_SIZE bytes each round
						memcpy(buf, tmp, BUF_SIZE);
						tmp += BUF_SIZE;
						pageoff += BUF_SIZE;
						// maybe this step can use mmap
						write(dev_fd, buf, BUF_SIZE);
					}
					else
					{
						// reset to 0
						memset(buf, 0, BUF_SIZE);
						// read the remain data
						memcpy(buf, tmp, diff);
						tmp += diff;
						pageoff = diff;
						// maybe this step can use mmap
						write(dev_fd, buf, diff);
						break;
					}
				}
				munmap(src, file_size);
				break;
            default:
                fprintf(stderr, "Invalid method : %s\n", method);
                return 1;
        }

        if(ioctl(dev_fd, master_IOCTL_EXIT) == -1) // end sending data, close the connection
        {
            perror("ioclt server exits error\n");
            return 1;
        }

        gettimeofday(&end, NULL);
        trans_time = (end.tv_sec - start.tv_sec)*1000 + (end.tv_usec - start.tv_usec)*0.0001;
        printf("Transmission time: %lf ms, File size: %ld bytes\n", trans_time, file_size / 8);

        close(file_fd);
    }

    close(dev_fd);
    return 0;
}

size_t get_filesize(const char* filename)
{
    struct stat st;
    stat(filename, &st);
    return st.st_size;
}

void help_message()
{
    printf("Usage: ./master [NUM] [FILES] [fcntl | mmap]\n");
}