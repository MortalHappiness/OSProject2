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
#define master_IOCTL_CREATESOCK 0x12345677
#define master_IOCTL_MMAP 0x12345678
#define master_IOCTL_EXIT 0x12345679

void help_message(); // print the help message
size_t get_filesize(const char* filename); // get the size of the input file

// ========================================

int main (int argc, char* argv[])
{
    char buf[BUF_SIZE];
    int dev_fd, file_fd;// the fd for the device and the fd for the input file
    size_t ret, file_size, total_file_size = 0, offset, length, tmp;
    int n_files;
    char *file_name;
    char *kernel_address = NULL, *file_address = NULL;
    struct timeval start;
    struct timeval end;
    //calulate the time between the device is opened and it is closed
    double trans_time;

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

    gettimeofday(&start, NULL);

    if( (dev_fd = open("/dev/master_device", O_RDWR)) < 0)
    {
        perror("failed to open /dev/master_device\n");
        return 1;
    }

    // mmap the device if the method is mmap
    if (method[0] == 'm')
    {
        kernel_address = mmap(NULL, MAP_SIZE,
                              PROT_WRITE, MAP_SHARED,
                              dev_fd, 0);
        if (kernel_address == MAP_FAILED)
        {
            perror("master device mmap error\n");
            close(dev_fd);
            return 1;
        }
    }

    for (int i = 2; n_files > 0; ++i, --n_files)
    {
        file_name = argv[i];

        if( (file_size = get_filesize(file_name)) < 0)
        {
            perror("failed to get filesize\n");
            goto ERROR_BUT_DEV_OPENED;
        }
        total_file_size += file_size;

        if( (file_fd = open(file_name, O_RDWR)) < 0 )
        {
            perror("failed to open input file\n");
            goto ERROR_BUT_DEV_OPENED;
        }

        // Create socket and accept the connection from the slave
        if(ioctl(dev_fd, master_IOCTL_CREATESOCK) == -1)
        {
            perror("ioctl server create socket error\n");
            goto ERROR_BUT_FILE_OPENED;
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

            case 'm': // mmap
                offset = 0; // Note that offset of mmap must be page aligned
                while (offset != file_size)
                {
                    if ((length = file_size - offset) > MAP_SIZE)
                    {
                        length = MAP_SIZE;
                    }

                    file_address = mmap(NULL, length,
                                        PROT_READ, MAP_SHARED,
                                        file_fd, offset);
                    if (file_address == MAP_FAILED)
                    {
                        perror("master file mmap error\n");
                        goto ERROR_BUT_SOCKET_CREATED;
                    }

                    // Copy file map into device map
                    memcpy(kernel_address, file_address, length);

                    if (ioctl(dev_fd, master_IOCTL_MMAP, length) != 0)
                    {
                        perror("ioctl server mmap error\n");
                        goto ERROR_BUT_SOCKET_CREATED;
                    }
                    if (munmap(file_address, length) != 0)
                    {
                        perror("master file munmap error\n");
                        goto ERROR_BUT_SOCKET_CREATED;
                    }

                    // MAP_SIZE is integer multiple of PAGE_SIZE,
                    // and length is MAP_SIZE except at EOF,
                    // so it is safe to update offset by length
                    offset += length;
                }

                break;

            default:
                fprintf(stderr, "Invalid method : %s\n", method);
                goto ERROR_BUT_SOCKET_CREATED;
        }

        close(file_fd);

        // end sending data, close the connection
        if(ioctl(dev_fd, master_IOCTL_EXIT) == -1)
        {
            perror("ioctl server exits error\n");
            goto ERROR_BUT_FILE_OPENED;
        }
    }

    if (method[0] == 'm' && munmap(kernel_address, MAP_SIZE) != 0)
    {
        perror("master device munmap error\n");
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
    if(ioctl(dev_fd, master_IOCTL_EXIT) == -1)
    {
        perror("ioctl server exits error\n");
    }
ERROR_BUT_FILE_OPENED:
    close(file_fd);
ERROR_BUT_DEV_OPENED:
    close(dev_fd);

    return 1;
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

