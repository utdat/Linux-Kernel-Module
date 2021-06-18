#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#define BUFFER_LENGTH 256

int main()
{
    char file_path[BUFFER_LENGTH];

    FILE *file_dir = fopen("/dev/hidefile", "w");
    if (file_dir == NULL)
    {
        perror("Open device failed!!!");
        return errno;
    }
    printf("Path of the file:\n");
    scanf("%[^\n]%*c", file_path);

    int flag = fprintf(file_dir, "%s", file_path); 
    if (flag < 0)
    {
        perror("Write the message to the device failed!!!");
        return errno;
    }

    printf("Hide file successfully.\n");
    fclose(file_dir);

    return 0;
}
