#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>


#define FIFO_NAME_RESP "RESP_PIPE_70782"
#define FIFO_NAME_REQ "REQ_PIPE_70782"


typedef struct section_head
{
    char name[12];
    char type;
    int offset;
    int size;
} section;

int main(void)
{
    int fd_req = -1, fd_resp = -1;


    if (mkfifo(FIFO_NAME_RESP, 0600) != 0) {
        printf("ERROR\n");
        printf("cannot create the response pipe\n");
        return 1;
    }

    //open, write and close fifo
    fd_req = open(FIFO_NAME_REQ, O_RDONLY);
    if (fd_req == -1) {
        printf("ERROR\n");
        printf("cannot open the request pipe\n");
        return 1;
    }

    fd_resp = open(FIFO_NAME_RESP, O_WRONLY);
    if (fd_resp == -1) {
        printf("ERROR\n");
        printf("cannot open the response pipe\n");
        return 1;
    }

    if (write(fd_resp, "HELLO!", 6) != -1 )
        printf("SUCCESS\n");

    char buffer[250];


    char c = 0;
    int indexf = 0;
    int index = 0;
    int fd = 0;


    unsigned int memsize = 0;
    unsigned int filesize = 0;


    int shmFd;
    volatile char *sharedMem = NULL;

    char * data = NULL;

    while (read(fd_req, &c, 1) > 0)
    {

        //printf("%c\n", c);
        buffer[index++] = c;


        if (c == '!')
        {

            if (strncmp(buffer, "PING", 4) == 0)
            {
                write(fd_resp, "PING!", 5);
                write(fd_resp, "PONG!", 5);
                unsigned int ping = 70782;
                write(fd_resp, &ping, sizeof(unsigned int));

                index = 0;
            }

            else if (strncmp(buffer, "CREATE_SHM", 10) == 0)
            {

                read(fd_req, &memsize, sizeof(unsigned int));

                shmFd = shm_open("/wgENP78J", O_CREAT | O_RDWR, 0664);
                if (shmFd < 0) {
                    perror("Could not aquire shm");
                    return 1;
                }
                ftruncate(shmFd, memsize);
                sharedMem = (volatile char*)mmap(0, memsize, PROT_READ | PROT_WRITE,
                                                 MAP_SHARED, shmFd, 0);
                if (sharedMem == (void*) - 1)
                    write(fd_resp, "CREATE_SHM!ERROR!", 17);
                else
                    write(fd_resp, "CREATE_SHM!SUCCESS!", 19);

            }
            else if (strncmp(buffer, "WRITE_TO_SHM", 12) == 0)
            {
                unsigned int offset = 0;
                unsigned int value = 0;

                read(fd_req, &offset, sizeof(unsigned int));
                read(fd_req, &value, sizeof(unsigned int));

                if (offset + 4 < memsize)
                {
                    *(unsigned int *)(sharedMem + offset) = value;
                    write(fd_resp, "WRITE_TO_SHM!SUCCESS!", 21);
                }
                else
                    write(fd_resp, "WRITE_TO_SHM!ERROR!", 19);

            }

            else if (strncmp(buffer, "MAP_FILE", 8) == 0)
            {
                char filename[1000];
                char f = 0;
                while (read(fd_req, &f, sizeof(char)))
                {
                    if (f == '!')
                    {
                        filename[indexf++] = 0;
                        break;
                    }
                    else
                        filename[indexf++] = f;
                }


                fd = open(filename, O_RDONLY);
                if (fd == -1)
                {


                    write(fd_resp, "MAP_FILE!ERROR!", 15);

                    index = 0;
                    indexf = 0;
                    continue;


                }

                filesize = lseek(fd, 0, SEEK_END);
                lseek(fd, 0, SEEK_SET);

                data = (char*)mmap(NULL, filesize, PROT_READ , MAP_SHARED, fd, 0);
                if (data == (void*) - 1) {

                    write(fd_resp, "MAP_FILE!ERROR!", 15);
                    index = 0;
                    indexf = 0;
                    continue;

                }
                write(fd_resp, "MAP_FILE!SUCCESS!", 17);

            }

            else if (strncmp(buffer, "READ_FROM_FILE_OFFSET", 21) == 0)
            {
                unsigned int offset = 0;
                unsigned int bytes = 0;

                read(fd_req, &offset, sizeof(unsigned int));
                read(fd_req, &bytes, sizeof(unsigned int));

                if (offset + bytes < filesize)
                {
                    for (int i = 0 ; i < bytes; i++)
                    {

                        sharedMem[i] = data[offset + i];
                    }
                    write(fd_resp, "READ_FROM_FILE_OFFSET!SUCCESS!", 30);
                }
                else
                    write(fd_resp, "READ_FROM_FILE_OFFSET!ERROR!", 28);
            }

            else if (strncmp(buffer, "READ_FROM_FILE_SECTION", 22) == 0)
            {
                unsigned int section_no = 0;
                unsigned int offset = 0;
                unsigned int bytes = 0;
                unsigned int no_sections = (unsigned int)(data[10]);

                read(fd_req, &section_no, sizeof(unsigned int));
                read(fd_req, &offset, sizeof(unsigned int));
                read(fd_req, &bytes, sizeof(unsigned int));

                unsigned int offset_sect = *(unsigned int *)((char *)data + 11 + (section_no - 1) * 21 + 13);
                unsigned int size_sect = *(unsigned int *)((char *)data + (section_no - 1) * 21 + 17);


                //printf("section_no = %d\noffset = %d\nbytes = %d\nno_sections=%d\noffset_sect=%d\nsize_sect=%d\n", section_no, offset, bytes, no_sections, offset_sect, size_sect);

                if ((no_sections < section_no) || ((offset + bytes) > size_sect) )
                    write(fd_resp, "READ_FROM_FILE_SECTION!ERROR!", 29);
                else
                {
                    for (int i = 0 ; i < bytes; i++)
                    {

                        sharedMem[i] = data[offset_sect + offset + i];

                    }
                    write(fd_resp, "READ_FROM_FILE_SECTION!SUCCESS!", 31);
                }

            }

            else if (strncmp(buffer, "READ_FROM_LOGICAL_SPACE_OFFSET", 30) == 0)
            {
                unsigned int logical_offset = 0;
                unsigned int bytes = 0;

                unsigned int no_sections = (unsigned int)(data[10]);

                read(fd_req, &logical_offset, sizeof(unsigned int));
                read(fd_req, &bytes, sizeof(unsigned int));
                long current_offset = 0;
                int logic_sect = 0;


                for (int i = 0; i < no_sections && current_offset < logical_offset + bytes; i++)
                {
                    unsigned int size_sect = *(unsigned int *)((char *)data + 11 + i * 21 + 17);

                    current_offset += ((size_sect + 1023) / 1024) * 1024;
                    logic_sect = i;

                }

                if ((current_offset < logical_offset + bytes) )
                    write(fd_resp, "READ_FROM_LOGICAL_SPACE_OFFSET!ERROR!", 37);
                else
                {

                    unsigned int size_sect = *(unsigned int *)((char *)data + 11 + logic_sect * 21 + 17);
                    unsigned int offset_sect = *(unsigned int *)((char *)data + 11 +  logic_sect * 21 + 13);
                    unsigned int of = current_offset - size_sect;
                    unsigned int rounded_offset = (of / 1024) * 1024;

                    unsigned int fis_offfset = logical_offset - rounded_offset;

                    for (int i = 0 ; i < bytes; i++)
                    {

                        sharedMem[i] = data[fis_offfset + offset_sect + i];

                    }
                    write(fd_resp, "READ_FROM_LOGICAL_SPACE_OFFSET!SUCCESS!", 39);
                }

            }

            else if (strncmp(buffer, "EXIT", 4) == 0)
            {
                break;
            }
            index = 0;
            indexf = 0;
        }

    }


    munmap(data, filesize);
    close(fd);

    close(fd_resp);
    close(fd_req);

    close(shmFd);
    shm_unlink("/wgENP78J");

    //delete fifo
    unlink(FIFO_NAME_REQ);
    unlink(FIFO_NAME_RESP);

    return 0;
}