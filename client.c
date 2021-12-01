#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <wait.h>
#include <sys/types.h>
#include <sys/stat.h>

#define FIFO_CS "FIFO_CS"
#define FIFO_SC "FIFO_SC"

int main()
{
    int fd = open(FIFO_CS, O_WRONLY);  //scriu din client in server
    int fd2 = open(FIFO_SC, O_RDONLY); //invers
    char comanda[100];                 //comanda de la tastatura

    int PID = fork();
    if (PID != 0) //tat
    {
        char c;
        printf("Scrie o comanda\n");
        while (fgets(comanda, 100, stdin), !feof(stdin))
        {
            char comanda_noua[200];
            sprintf(comanda_noua, "%d %s", getpid(), comanda);
            //printf("comanda: %s\n", comanda_noua);
            write(fd, comanda_noua, strlen(comanda_noua));
            strcpy(comanda, "");
        }
    }
    else //cop
    {
        int num;
        do
        {
            char str[500];
            int n = read(fd2, str, 500);
            if (strcmp(str, "Q") == 0)
            {
                kill(getpid(), SIGKILL);
            }
            str[n] = '\0';
            printf("rezultatul comenzii:\n%s\n", str);
            printf("Scrie o comanda\n");
        } while (1);
    }
}