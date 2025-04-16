#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

#define DEVICE_FILE "/dev/AL_UART"

int main(int argc, char *argv[])
{
    int fd;
    char message[256];

    if (argc != 2) 
    {
        fprintf(stderr, "(UserProgram)Usage: %s <message>\n", argv[0]);
        return EXIT_FAILURE;
    }

    // Copy into message buffer
    snprintf(message, sizeof(message), "%s", argv[1]);

    fd = open(DEVICE_FILE, O_RDWR);
    if (fd < 0) {
        perror("(UserProgram)Failed to open device file");
        return EXIT_FAILURE;
    }

    if (write(fd, message, strlen(message)) < 0) 
    {
        perror("(UserProgram)Failed to send message via WRITE");
        close(fd);
        return EXIT_FAILURE;
    }

    close(fd);
    return EXIT_SUCCESS;
}
