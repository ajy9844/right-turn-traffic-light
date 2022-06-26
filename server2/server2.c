#include "server2.h"

extern int fd;
extern int pressure1;
extern int pressure2;

void error_handling(char *message) 
{
    fputs(message, stderr);
    fputc('\n', stderr);
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("Usage : %s <port>\n", argv[0]);
        exit(1);
    }
    
    pthread_t p_thread[1];
    int thr_id;
    char p1[] = "thread_1";

    thr_id = pthread_create(&p_thread[0], NULL, pres_sensor_act, (void *)p1);
    if (thr_id < 0)
    {
        perror("thread_1 create error : ");
        exit(0);
    }
    printf("Force Sensing Resistors are ready!\n");

    int in_state  = 0; // (2)
    int out_state = 0; // (1)

    int serv_sock, clnt_sock = -1;
    struct sockaddr_in serv_addr, clnt_addr;
    socklen_t clnt_addr_size;
    char msg[4];
    
    serv_sock = socket(PF_INET, SOCK_STREAM, 0);
    if (serv_sock == -1)
        error_handling("socket() error");

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(atoi(argv[1]));

    if (bind(serv_sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
        error_handling("bind() error");
    printf("bind!!!!!!\n");
    if (listen(serv_sock, 5) == -1)
        error_handling("listen() error");
    printf("listen!!!!!!!!!!!1\n");
    if (clnt_sock < 0)
    {
        clnt_addr_size = sizeof(clnt_addr);
        clnt_sock = accept(serv_sock, (struct sockaddr *)&clnt_addr,
                           &clnt_addr_size);
        if (clnt_sock == -1)
            error_handling("accept() error");
        printf("accept!!!!!!!\n");
    }
    
    int res = -1;
    while (1)
    {
        out_state = pressure1;
        printf("*** out_state: %d ***\n", out_state);
        if (out_state > 30) { // by person; 100
            //usleep(10000);    // 5_sec
            snprintf(msg, 4, "%d", out_state);
            res = write(clnt_sock, msg, sizeof(msg));
            sleep(1);
            in_state = pressure2;
            printf("*** in_state: %d ***\n", in_state);
            if (in_state < 30) {
                snprintf(msg, 4, "%d", 0);
                res = write(clnt_sock, msg, sizeof(msg));
                sleep(1);
                printf("res = %d, msg = %s\n", res, msg);
            } 
        } else {
            snprintf(msg, 4, "%d", 0);
            res = write(clnt_sock, msg, 4);
            sleep(1);
            printf("res = %d, msg = %s\n", res, msg);
        }
        usleep(50000);
    }
    
    
   
    pthread_cancel(p_thread[0]);
    
    close(clnt_sock);
    close(serv_sock);

    return 0;
}
