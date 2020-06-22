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

struct mmap_ioctl_args {
    char *file_address;
    size_t length;
};

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
    struct mmap_ioctl_args mmap_args;
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

    gettimeofday(&start ,NULL);

    if( (dev_fd = open("/dev/master_device", O_RDWR)) < 0)
    {
        perror("failed to open /dev/master_device\n");
        return 1;
    }

    // TODO:
    // Now, for each file I create a socket.
    // Maybe we need to consider how to transmit all files with single socket.

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
        total_file_size += file_size;

        // Create socket and accept the connection from the slave
        if(ioctl(dev_fd, master_IOCTL_CREATESOCK) == -1)
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
                        return 1;
                    }
                    mmap_args.file_address = file_address;
                    mmap_args.length = length;

                    if (ioctl(dev_fd, master_IOCTL_MMAP, &mmap_args) != 0)
                    {
                        perror("ioctl server mmap error\n");
                        return 1;
                    }
                    if (munmap(file_address, length) != 0)
                    {
                        perror("master file munmap error\n");
                        return 1;
                    }

                    // MAP_SIZE is integer multiple of PAGE_SIZE,
                    // and length is MAP_SIZE except at EOF,
                    // so it is safe to update offset by length
                    offset += length;
                }

                break;

            default:
                fprintf(stderr, "Invalid method : %s\n", method);
                return 1;
        }

        // end sending data, close the connection
        if(ioctl(dev_fd, master_IOCTL_EXIT) == -1)
        {
            perror("ioctl server exits error\n");
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

