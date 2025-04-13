#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

#define DEVICE_FILE "/dev/AL_UART"
#define TRANSMIT_MESSAGE 0x01

int main(int argc, char *argv[])
{
    int fd;
    char message[256];

    if (argc != 2) {
        fprintf(stderr, "(UserProgram)Usage: %s <message>\n", argv[0]);
        return EXIT_FAILURE;
    }

    snprintf(message, sizeof(message), "%s", argv[1]);

    fd = open(DEVICE_FILE, O_RDWR);
    if (fd < 0) {
        perror("(UserProgram)Failed to open device file");
        return EXIT_FAILURE;
    }

    if (ioctl(fd, TRANSMIT_MESSAGE, message) < 0) {
        perror("(UserProgram)Failed to send message via IOCTL");
        close(fd);
        return EXIT_FAILURE;
    }

    printf("(UserProgram)Message sent to UART: %s\n", message);

    close(fd);
    return EXIT_SUCCESS;
}
