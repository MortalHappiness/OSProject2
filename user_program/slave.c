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
#define MAP_SIZE 8192 // 2 PAGE_SIZE
#define slave_IOCTL_CREATESOCK 0x12345677
#define slave_IOCTL_MMAP 0x12345678
#define slave_IOCTL_EXIT 0x12345679

void help_message();

// ========================================

int main (int argc, char* argv[])
{
    char buf[BUF_SIZE];
    int dev_fd, file_fd;// the fd for the device and the fd for the input file
    size_t ret, file_size, total_file_size = 0, data_size = -1, offset;
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

    // mmap the device if the method is mmap
    if (method[0] == 'm')
    {
        kernel_address = mmap(NULL, MAP_SIZE,
                              PROT_READ, MAP_SHARED,
                              dev_fd, 0);
        if (kernel_address == MAP_FAILED)
        {
            perror("slave device mmap error\n");
            close(dev_fd);
            return 1;
        }
    }

    for (int i = 2; n_files > 0; ++i, --n_files)
    {
        file_name = argv[i];
        file_size = 0;
        if( (file_fd = open (file_name, O_RDWR | O_CREAT | O_TRUNC, 0666)) < 0)
        {
            perror("failed to open input file\n");
            goto ERROR_BUT_DEV_OPENED;
        }

        // Connect to master in the device
        if(ioctl(dev_fd, slave_IOCTL_CREATESOCK, ip) == -1)
        {
            perror("ioctl create slave socket error\n");
            goto ERROR_BUT_FILE_OPENED;
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

            case 'm': //mmap
                offset = 0; // Note that offset of mmap must be page aligned

                do
                {
                    if (ftruncate(file_fd, offset + MAP_SIZE) == -1)
                    {
                        perror("slave ftruncate error\n");
                        goto ERROR_BUT_SOCKET_CREATED;
                    }
                    file_address = mmap(NULL, MAP_SIZE,
                                        PROT_WRITE, MAP_SHARED,
                                        file_fd, offset);
                    if (file_address == MAP_FAILED)
                    {
                        perror("slave file mmap error\n");
                        goto ERROR_BUT_SOCKET_CREATED;
                    }

                    ret = ioctl(dev_fd, slave_IOCTL_MMAP, MAP_SIZE);
                    if (ret < 0)
                    {
                        perror("ioctl client mmap error\n");
                        goto ERROR_BUT_SOCKET_CREATED;
                    }

                    // Copy device map into file map
                    memcpy(file_address, kernel_address, ret);

                    if (munmap(file_address, MAP_SIZE) != 0)
                    {
                        perror("master file munmap error\n");
                        goto ERROR_BUT_SOCKET_CREATED;
                    }

                    // MAP_SIZE is integer multiple of PAGE_SIZE,
                    // and ret is MAP_SIZE except at EOF,
                    // so it is safe to update offset by ret
                    offset += ret;
                    total_file_size += ret;

                } while (ret == MAP_SIZE);

                if (ftruncate(file_fd, offset) == -1)
                {
                    perror("slave ftruncate error\n");
                    goto ERROR_BUT_SOCKET_CREATED;
                }

                break;

            default:
                fprintf(stderr, "Invalid method : %s\n", method);
                goto ERROR_BUT_SOCKET_CREATED;
        }

        // end receiving data, close the connection
        if(ioctl(dev_fd, slave_IOCTL_EXIT) == -1)
        {
            perror("ioclt client exits error\n");
            goto ERROR_BUT_FILE_OPENED;
            return 1;
        }

        close(file_fd);
    }

    if (method[0] == 'm' && munmap(kernel_address, MAP_SIZE) != 0)
    {
        perror("slave device munmap error\n");
        goto ERROR_BUT_DEV_OPENED;
    }

    close(dev_fd);

    gettimeofday(&end, NULL);
    trans_time = (end.tv_sec - start.tv_sec)*1000 +
                 (end.tv_usec - start.tv_usec)*0.0001;
    printf("Transmission time: %lf ms, File size: %ld bytes\n",
            trans_time, total_file_size);

    return 0;

// Error handling
ERROR_BUT_SOCKET_CREATED:
    if(ioctl(dev_fd, slave_IOCTL_EXIT) == -1)
    {
        perror("ioctl client exits error\n");
    }
ERROR_BUT_FILE_OPENED:
    close(file_fd);
ERROR_BUT_DEV_OPENED:
    close(dev_fd);

    return 1;
}

void help_message()
{
    printf("Usage: ./slave [NUM] [FILES] [fcntl | mmap] [IP_ADDRESS]\n");
}
