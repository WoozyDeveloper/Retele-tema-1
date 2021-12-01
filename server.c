//server

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
#include <utmp.h>
#include <time.h>
#include <pwd.h>
#include <sys/socket.h>

#define FIFO_CS "FIFO_CS"
#define FIFO_SC "FIFO_SC"

int main()
{
    int logged_in_pids[7] = {0};
    if (mkfifo(FIFO_CS, 0606) == -1)
    {
        if (errno != EEXIST)
            exit(404);
    }

    if (mkfifo(FIFO_SC, 0606) == -1)
    {
        if (errno != EEXIST)
            exit(404);
    }

    printf("Astept sa intre cineva...\n");
    int fd = open(FIFO_CS, O_RDONLY);
    int fd2 = open(FIFO_SC, O_WRONLY);
    printf("A venit cineva:\n");

    //bagam in usrs utilizatorii din fisier
    char users[100][40];
    FILE *f = fopen("users", "r");
    for (int i = 0; i < 3; i++)
        fgets(users[i], 40, f);
    //-------------------------------------

    int num;
    do
    {
        char comanda[100];
        num = read(fd, comanda, 100);
        if (num < 0)
            perror("EROARE LA PRELUAREA COMENZII\n");
        printf("COMANDA= %s\n", comanda);
        char *p = strtok(comanda, " ");
        char pid_copy[10];
        strcpy(pid_copy, p); //aici ai pidul
        printf("%s\n", p);
        p = strtok(NULL, " \n");
        printf("%s\n", p);
        if (strcmp(p, "login") == 0)
        {
            int pipe_fd[2];
            pipe(pipe_fd);
            //pipe_fd[0] pt read
            //pipe_fd[1] pt write
            int PID = fork();
            if (PID != 0)
            {
                int num;
                int response;
                wait(NULL);
                if (num = read(pipe_fd[0], &response, sizeof(response)) < 0)
                    perror("Eroare la citirea din pipe_fd\n");
                else
                {
                    if (response == 1) //m am logat
                    {
                        char *name = strtok(NULL, " "); //iau numele
                        printf("AM AJUNS AICI\n");
                        write(fd2, "te ai logat\n", 12);    //il scriu in fd2
                        logged_in_pids[0] = atoi(pid_copy); //marchez ca s logat in vector
                    }
                    else //nieman
                    {
                        write(fd2, "Nu esti logat\n", 14);
                    }
                }
            }
            else
            {
                bool logat = false;
                char *p = strtok(NULL, " \n"); //parametru(user)
                char copy_name[30];            //copy of name
                strcpy(copy_name, p);          //in copy am numele
                strcat(copy_name, "\n\0");
                for (int i = 0; i < 3; i++)
                    if (strcmp(users[i], copy_name) == 0)
                    {
                        //printf("A VENIT %s cu pid=%s\n", copy_name, pid_copy);
                        if (logged_in_pids[strlen(copy_name)] == 0)
                        {
                            logat = true;
                            printf("te ai logat\n");
                            int x = 1;
                            write(pipe_fd[1], &x, sizeof(x));
                            exit(0);
                        }
                    }
                if (logat == false)
                {
                    int x = 0;
                    write(pipe_fd[1], &x, sizeof(x));
                }
                close(pipe_fd[1]);
                exit(0);
            }
        }
        else if (strcmp(p, "get-logged-users") == 0)
        {
            bool is_connected;
            int pipe_logged[2]; //prin asta trimit rezultatul la parinte
            pipe(pipe_logged);  //0 read, 1 write
            int PID = fork();
            if (PID != 0) //tat
            {
                //return the feedback from the child
                wait(NULL);
                char result[500];
                read(pipe_logged[0], result, 300);
                printf("%s", result);
                write(fd2, result, strlen(result));
            }
            else //cop
            {
                is_connected = false;
                //vezi daca i logat
                for (int i = 0; i < 1 && is_connected == false; i++)
                    if (logged_in_pids[i] != 0)
                        is_connected = true;
                if (is_connected == false)
                {
                    printf("Not connected\n");
                    write(pipe_logged[1], "You are not connected\n", 22);
                }
                else
                {
                    printf("Connected\n");

                    //sursa de inspiratie -> https://www.titanwolf.org/Network/q/51687e75-0439-4efb-b3d2-27d42dd11e8f/y
                    struct utmp *n;
                    setutent();
                    n = getutent();

                    while (n)
                    {
                        if (n->ut_type == USER_PROCESS)
                        {
                            time_t timestamp = n->ut_tv.tv_sec;
                            char result[500];
                            sprintf(result, "%9s%12s --------------> %s\n", n->ut_user, n->ut_host, asctime(localtime(&timestamp)));
                            write(pipe_logged[1], result, strlen(result));
                        }
                        n = getutent();
                    }

                    endutent();
                    exit(0);
                }
            }
        }
        else if (strcmp(p, "get-proc-info") == 0)
        {
            //vezi daca i logat
            bool is_connected;
            int sockp[2]; //prin asta trimit rezultatul la parinte
            if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockp) < 0)
            {
                perror("Err... socketpair");
                exit(12321);
            }
            int PID = fork();
            if (PID != 0) //tat
            {
                //return the feedback from the child
                close(sockp[1]);
                wait(NULL);
                char result[500];
                int num = read(sockp[0], result, 500);
                result[num] = '\0';
                write(fd2, result, strlen(result));
                close(sockp[0]);
            }
            else //cop
            {
                close(sockp[0]);
                is_connected = false;
                //vezi daca i logat
                for (int i = 0; i < 1 && is_connected == false; i++)
                    if (logged_in_pids[i] != 0)
                        is_connected = true;
                if (is_connected == false)
                {
                    printf("Not connected\n");
                    write(fd2, "You are not connected\n", 22);
                }
                else
                {
                    printf("Connected\n");
                    p = strtok(NULL, " \n"); //PID-ul cerul e aici
                    char created_path[20], current_line[100];
                    sprintf(created_path, "/proc/%s/status", p);
                    FILE *read_file = fopen(created_path, "r");
                    int dim = 100;
                    while (fgets(current_line, sizeof(current_line), read_file) != NULL)
                    {
                        printf("%s\n", current_line);

                        //name, state, ppid, uid, vmsize
                        if (strstr(current_line, "Name") != NULL)
                        {
                            write(sockp[1], current_line, strlen(current_line));
                        }

                        else if (strstr(current_line, "State") != NULL)
                        {
                            write(sockp[1], current_line, strlen(current_line));
                        }

                        else if (strstr(current_line, "PPid") != NULL)
                        {
                            write(sockp[1], current_line, strlen(current_line));
                        }

                        else if (strstr(current_line, "Uid") != NULL)
                        {
                            write(sockp[1], current_line, strlen(current_line));
                        }

                        else if (strstr(current_line, "VmSize") != NULL)
                        {
                            write(sockp[1], current_line, strlen(current_line));
                        }
                    }

                    //executa in continuare comanda
                }
                close(sockp[1]);
                exit(0); //end while
            }
        }
        else if (strcmp(p, "quit") == 0)
        {
            write(fd2, "Q", 1);
            kill(getpid(), SIGKILL);
        }
        else if (strcmp(p, "logout") == 0)
        {
            bool connected = false;
            for (int i = 0; i < 7; i++)
            {
                if (logged_in_pids[i] != 0)
                    connected = true;
            }
            if (connected == true)
            {
                for (int i = 0; i < 7; i++)
                    if (logged_in_pids[i] != 0)
                        logged_in_pids[i] = 0;
                write(fd2, "Disconnected\n", 13);
            }
            else
            {
                write(fd2, "You are not connected\n", 22);
            }
        }
        else
        {
            write(fd2, "Comanda invalida\n", 17);
        }
    } while (num > 0);
}