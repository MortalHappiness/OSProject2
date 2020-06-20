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
int main (int argc, char* argv[])
{
    char buf[BUF_SIZE];
    int dev_fd, file_fd;// the fd for the device and the fd for the input file
    size_t ret, file_size, data_size = -1;
    int n_files;
    char *file_name;
    struct timeval start;
    struct timeval end;
    double trans_time; //calulate the time between the device is opened and it is closed
    char *kernel_address, *file_address;

    // Deal with input parameters
    if (argc < 5)
    {
        help_message();
        return 1;
    }
    n_files = atoi(argv[1]);
    if (n_files <= 0 || n_files != argc - 4)
    {
        help_message();
        return 1;
    }
    const char *method = argv[argc - 2];
    const char *ip = argv[argc - 1];

    // ==============================

    if( (dev_fd = open("/dev/slave_device", O_RDWR)) < 0)//should be O_RDWR for PROT_WRITE when mmap()
    {
        perror("failed to open /dev/slave_device\n");
        return 1;
    }

    // TODO:
    // Now, for each file I create a socket.
    // Maybe we need to consider how to transmit all files with single socket.

    for (int i = 2; n_files > 0; ++i, --n_files)
    {
        file_name = argv[i];
        file_size = 0;
        if( (file_fd = open (file_name, O_RDWR | O_CREAT | O_TRUNC)) < 0)
        {
            perror("failed to open input file\n");
            return 1;
        }

        gettimeofday(&start ,NULL);
        if(ioctl(dev_fd, master_IOCTL_CREATESOCK, ip) == -1) //0x12345677 : connect to master in the device
        {
            perror("ioctl create slave socket error\n");
            return 1;
        }

        switch(method[0])
        {
            case 'f'://fcntl : read()/write()
                // read from the the device
                while ((ret = read(dev_fd, buf, sizeof(buf))) > 0)
                {
                    write(file_fd, buf, ret); //write to the input file
                    file_size += ret;
                }
                break;

            case 'm': ;//mmap
				size_t pageoff = 0, diff = 0;
				char *dst, *src;
				if(file_size = ioctl(dev_fd, master_IOCTL_MMAP) == 0) // recv file size from slave device
				{
					perror("failed to recv file size from slave device\n");
					return 1;
				}
				printf("file_size is %d\n", file_size);
				dst = mmap(NULL, file_size, PROT_READ|PROT_WRITE, MAP_SHARED, file_fd, pageoff);
				char *tmp = dst;
				while(pageoff < file_size)
				{
					// read diff bytes each round
					diff = file_size - pageoff;
					// maybe we can use mmap instead
					read(dev_fd, buf, diff); // read from the the device

					if(diff > BUF_SIZE)
					{
						memcpy(tmp, buf, BUF_SIZE);
						tmp += BUF_SIZE;
						pageoff += BUF_SIZE;
					}
					else
					{
						memcpy(tmp, buf, diff);
						tmp += diff;
						pageoff += diff;
						break;
					}
				}
				munmap(dst, file_size);
				break;

            default:
                fprintf(stderr, "Invalid method : %s\n", method);
                return 1;
        }

        if(ioctl(dev_fd, master_IOCTL_EXIT) == -1)// end receiving data, close the connection
        {
            perror("ioclt client exits error\n");
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

void help_message()
{
    printf("Usage: ./slave [NUM] [FILES] [fcntl | mmap] [IP_ADDRESS]\n");
}
