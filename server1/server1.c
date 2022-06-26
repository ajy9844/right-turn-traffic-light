#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include <time.h>


#define IN    0
#define OUT   1
#define LOW   0
#define HIGH  1
#define PIN  20
#define POUT 21

#define BUFFER_MAX 45
#define DIRECTION_MAX 45
#define VALUE_MAX 256

#define PIN2 24 //ultrawave
#define POUT2 23

double distance=0;



static int GPIOUnexport(int pin){
   char buffer[BUFFER_MAX];
   ssize_t bytes_written;
   int fd;
   
   fd=open("/sys/class/gpio/unexport",O_WRONLY);
   if(-1==fd){
      fprintf(stderr,"falied to pen unexport\n");
      return -1;
   }
   
   bytes_written=snprintf(buffer,BUFFER_MAX,"%d",pin);
   write(fd,buffer,bytes_written);
   close(fd);
   return 0;
}

static int GPIODirection(int pin,int dir){
      static const char s_directions_str[]="in\0out";
      
   
   char path[DIRECTION_MAX]="/sys/class/gpio/gpio%d/direction";
   int fd;
   
   snprintf(path, DIRECTION_MAX, "/sys/class/gpio/gpio%d/direction",pin);
   
   fd=open(path,O_WRONLY);
   if(-1==fd){
      fprintf(stderr,"Failed to open gpio direction for writing!\n");
      return -1;
   }
   
   if(-1==write(fd,&s_directions_str[IN == dir ? 0 :3], IN==dir ? 2:3)){
      fprintf(stderr,"failed to set direction!\n");
      return -1;
   }
   
   close(fd);
   return 0;
}

static int GPIOExport(int pin){

   char buffer[BUFFER_MAX];
   ssize_t bytes_written;
   int fd;
   
   fd=open("/sys/class/gpio/export",O_WRONLY);
   if(-1==fd){
      fprintf(stderr, "failed export wr");
      return 1;
   }
   bytes_written=snprintf(buffer,BUFFER_MAX, "%d", pin);
   write(fd,buffer,bytes_written);
   close(fd);
   return 0;
}


static int GPIOWrite(int pin, int value){
      static const char s_values_str[] ="01";
      
      char path[VALUE_MAX];
      int fd;
      
      printf("write value!\n");
      
      snprintf(path,VALUE_MAX, "/sys/class/gpio/gpio%d/value",pin);
      fd=open(path,O_WRONLY);
      if(-1==fd){
         fprintf(stderr,"failed open gpio write\n");
         return -1;
      }
      
      //0 1 selection
      if(1!=write(fd,&s_values_str[LOW==value ? 0:1],1)){
         fprintf(stderr,"failed to write value\n");
         return -1;
      }
      close(fd);
      return 0;
}

static int GPIORead(int pin){
    char path[VALUE_MAX];
    char value_str[3];
    int fd;

    snprintf(path, VALUE_MAX, "/sys/class/gpio/gpio%d/value",pin);
    fd=open(path, O_RDONLY);
    if(-1==fd){
        fprintf(stderr,"failed to open gpio value for reading\n");
        return -1;
    }

    if(-1==read(fd,value_str,3)){
        fprintf(stderr,"failed to read val\n");
        return -1;
    }
    close(fd);

    return(atoi(value_str));
}


void error_handling(char *message){
    fputs(message,stderr);
    fputc('\n',stderr);
    exit(1);
}

// ultra
void *server1_socket_6879(){
	clock_t start_t,end_t;
	double time;
    
    int serv_sock, clnt_sock = -1;
    struct sockaddr_in serv_addr, clnt_addr;
    socklen_t clnt_addr_size;
    char msg[12];
    
    serv_sock = socket(PF_INET, SOCK_STREAM, 0);
    if (serv_sock == -1)
        error_handling("ultra_socket() error");

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(6879);
    
    if (bind(serv_sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
        error_handling("ultra_bind() error");
    printf("ultra_bind clear\n");
    if (listen(serv_sock, 5) == -1)
        error_handling("ultra_listen() error");
    printf("ultra_listen clear\n");
    if (clnt_sock < 0)
    {printf("eeeeee\n");
        clnt_addr_size = sizeof(clnt_addr);
        clnt_sock = accept(serv_sock, (struct sockaddr *)&clnt_addr,
                           &clnt_addr_size);
        if (clnt_sock == -1)
            error_handling("ultra_accept() error");
    }
    printf("dddddD\n");
    while (1)
    {
        
        
        if (-1 == GPIOWrite(POUT2, 1)) {
			
			printf("gpio write/trigger err\n");
			exit(0);
		}
		usleep(1000);
   
        GPIOWrite(POUT2,0);
   
     while(GPIORead(PIN2) ==0){
      start_t =clock();
      }
   
      while(GPIORead(PIN2) ==1){
      end_t=clock();
      }
      
      time = (double)(end_t-start_t)/CLOCKS_PER_SEC;
      distance = time/2*34000;
		
      if (distance > 900)
			distance = 900;
      
      
      printf("time : %.4lf\n",time);
      printf("distance : %.2lfcm\n",time/2*34000);
      snprintf(msg, 12,"%.2lf", distance);  //later state change
      write(clnt_sock, msg, sizeof(msg));
      printf("ultra_msg = %s\n",msg); 
      usleep(2000000);
   } 
  
        close(clnt_sock);
    close(serv_sock);
    
    }
    

void *server1_socket_7087(){
    
    int serv_sock, clnt_sock = -1;
    struct sockaddr_in serv_addr, clnt_addr;
    socklen_t clnt_addr_size;
    char msg[2];
    
    int state=1;
    int prev_state=1;
    
    serv_sock = socket(PF_INET, SOCK_STREAM, 0);
    if (serv_sock == -1)
        error_handling("button_socket() error");

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(7087);
   
    
    if (bind(serv_sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
        error_handling("button_bind() error");
    printf("button_bind clear\n");
    if (listen(serv_sock, 5) == -1)
        error_handling("button_listen() error");
    printf("button_isten clear\n");
    if (clnt_sock < 0)
    {
        clnt_addr_size = sizeof(clnt_addr);
        clnt_sock = accept(serv_sock, (struct sockaddr *)&clnt_addr,
                           &clnt_addr_size);
        if (clnt_sock == -1)
            error_handling("button_accept() error");
    }
    
    while (1)
    {
      //printf("accept clear, in while\n");
      state =GPIORead(PIN);
      printf("button state: %d!!!!!!!\n", state);
      if(prev_state ==0 && state==1){
        
         printf("~~~~~~~button_while~~~~~~~~~~\n");
         snprintf(msg,2,"%d",9);  //later state change
         write(clnt_sock, msg, sizeof(msg));
         printf("msg=%s\n",msg);
         printf("msg=%s\n",msg);
         sleep(1); 
        }
    
        prev_state = state;
        usleep(5000);
       
    }
    
    close(clnt_sock);
    close(serv_sock);
    
}



int main(int argc, char *argv[])
{
	
	//Enable GPIO pins
    if (-1 == GPIOExport(PIN) || -1 == GPIOExport(POUT) || -1 == GPIOExport(POUT2) || -1 == GPIOExport(PIN2))
        return (1);
    usleep(5000);
    //Set GPIO directions
    if (-1 == GPIODirection(PIN, IN) || -1 == GPIODirection(POUT, OUT) || -1 ==GPIODirection(POUT2,OUT) || -1==GPIODirection(PIN2,IN))
        return (2);
    if (-1 == GPIOWrite(POUT, 1))
        return (3);

    

    pthread_t p_thread[2];
    int thr_id;
    
    char p1[] = "thread_1";
	char p2[] = "thread_2";

    thr_id = pthread_create(&p_thread[0], NULL, server1_socket_7087, (void **)p1);
    if (thr_id < 0)
    {
        perror("thread_1 create error : ");
        exit(0);
    }
    
    thr_id = pthread_create(&p_thread[1], NULL, server1_socket_6879, (void **)p2);
    if (thr_id < 0)
    {
        perror("thread_2 create error : ");
        exit(0);
    }
    sleep(300);
    pthread_cancel(p_thread[0]);
    pthread_cancel(p_thread[1]);
    
    //Disable GPIO pins
    if (-1 == GPIOUnexport(PIN) || -1 == GPIOUnexport(POUT) || GPIOUnexport(PIN2) || GPIOUnexport(POUT2))
        return (4);
    return (0);
    
    
}
