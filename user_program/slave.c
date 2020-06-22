#include <stdio.h>
#include <stdlib.h>
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
#define MMAP_SIZE PAGE_SIZE * 128

#define slave_IOCTL_CREATESOCK 0x12345677
#define slave_IOCTL_MMAP 0x12345678
#define slave_IOCTL_EXIT 0x12345679
#define slave_IOCTL_FILESIZE 0x12345680


void help_message();

// ========================================

int main (int argc, char* argv[])
{
    char buf[BUF_SIZE];
    int dev_fd, file_fd;// the fd for the device and the fd for the input file
    size_t ret, file_size, total_file_size = 0, data_size = -1;
    int n_files;
    char *file_name;
    struct timeval start;
    struct timeval end;
    //calulate the time between the device is opened and it is closed
    double trans_time;
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

    gettimeofday(&start ,NULL);

    //should be O_RDWR for PROT_WRITE when mmap()
    if( (dev_fd = open("/dev/slave_device", O_RDWR)) < 0)
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
        if( (file_fd = open (file_name, O_RDWR | O_CREAT | O_TRUNC, 0666)) < 0)
        {
            perror("failed to open input file\n");
            return 1;
        }

        // Connect to master in the device
        if(ioctl(dev_fd, slave_IOCTL_CREATESOCK, ip) == -1)
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
                    total_file_size += ret;
                }
                break;

            case 'm': ;//mmap
                size_t offset = 0, len;
                if((file_size = (size_t)ioctl(dev_fd, slave_IOCTL_MMAP)) == 0) // recv file size from slave device
                {
                    perror("failed to recv file size from slave device\n");
                    return 1;
                }
                printf("file_size received is %zu\n", file_size);
                while (1){
                    // if find a way to pass multiple parameters, then we don't need mmap in every iteration
                    ret = ioctl(dev_fd, slave_IOCTL_MMAP);
                    if (ret < 0) {
                        perror("Master ioctl mmap failed!\n");
                        return 1;
		            }
                    else if (ret == 0){
                        break;
                    }
                    posix_fallocate(file_fd, offset, ret);
                    file_address = mmap(NULL, ret, PROT_READ, MAP_SHARED, file_fd, offset); // mmap for file
                    kernel_address = mmap(NULL, ret, PROT_WRITE, MAP_SHARED, dev_fd, 0); // mmap for device 
                    memcpy(file_address, kernel_address, ret);
                    offset += ret;
                    munmap(kernel_address, ret);
                    munmap(file_address, ret);
                }
                break;


            default:
                fprintf(stderr, "Invalid method : %s\n", method);
                return 1;
        }

        // end receiving data, close the connection
        if(ioctl(dev_fd, slave_IOCTL_EXIT) == -1)
        {
            perror("ioclt client exits error\n");
            return 1;
        }

        close(file_fd);
    }

    close(dev_fd);

    gettimeofday(&end, NULL);
    trans_time = (end.tv_sec - start.tv_sec)*1000 +
                 (end.tv_usec - start.tv_usec)*0.0001;
    printf("Transmission time: %lf ms, File size: %ld bytes\n",
            trans_time, total_file_size);

    return 0;
}

void help_message()
{
    printf("Usage: ./slave [NUM] [FILES] [fcntl | mmap] [IP_ADDRESS]\n");
}
