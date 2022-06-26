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

#define IN 0
#define OUT 1
#define LOW 0
#define HIGH 1
#define POUT 20
//#define POUT2 19 -> BLUE
#define PWM 0
#define VALUE_MAX 256
#define BUFFER_MAX 3
#define DIRECTION_MAX 100
#define BLUE 19
#define GREEN 23
#define RED 24

/*  Clinet, output control                            */
/*   - RGB LED                                        */
/*   - LED                                            */
/*   - BUZZER                                         */
/*  Button push -> program Exit                       */
int g_ultrawave, g_pressure, g_button;

static int PWMExport(int pwmnum) {

   char buffer[BUFFER_MAX];
   int bytes_written;
   int fd;
   
   fd = open("/sys/class/pwm/pwmchip0/export", O_WRONLY);
   if (-1 == fd) {
      fprintf(stderr, "failed to open in unexport\n");
      return(-1);
   }
   bytes_written = snprintf(buffer, BUFFER_MAX, "%d", pwmnum);
   write(fd, buffer, bytes_written);
   close(fd);
   sleep(1);
   return(0);
}

static int PWMUnexport(int pwmnum) {
   char buffer[BUFFER_MAX];
   ssize_t bytes_written;
   int fd;
   
   fd= open("/sys/class/pwm/pwmchip0/unexport",O_WRONLY);
   if (-1 == fd) {
      fprintf(stderr, "failed to open in unexport!\n");
      return(-1);
   }
   
   bytes_written = snprintf(buffer, BUFFER_MAX, "%d", pwmnum);
   write(fd, buffer, bytes_written);
   close(fd);
   
   sleep(1);
   return(0);
}

static int PWMEnable(int pwmnum)
{
   static const char s_unenable_str[] = "0";
   static const char s_enable_str[] = "1";

   char path[DIRECTION_MAX];
   int fd;
   
   snprintf(path, DIRECTION_MAX, "/sys/class/pwm/pwmchip0/pwm%d/enable", pwmnum);
   fd = open(path, O_WRONLY);
   if (-1 == fd) {
      fprintf(stderr, "failed to open in enable\n");
      return -1;
   }
   
   write(fd, s_unenable_str, strlen(s_unenable_str));
   close(fd);
   
   fd  = open(path, O_WRONLY);
   if (-1 ==fd){
      fprintf(stderr, "failed to open in enable\n");
      return -1;
      
   }
   write(fd, s_enable_str, strlen(s_enable_str));
   close(fd);
   return(0);
}


static int PWMUnable(int pwmnum)
{
   static const char s_unable_str[] = "0";

   char path[DIRECTION_MAX];
   int fd;
   
   snprintf(path, DIRECTION_MAX, "/sys/class/pwm/pwmchip0/pwm%d/enable", pwmnum);
   fd = open(path, O_WRONLY);
   if (-1 == fd) {
      fprintf(stderr, "failed to open in enable\n");
      return -1;
   }
   
   write(fd, s_unable_str, strlen(s_unable_str));
   close(fd);
   
   return(0);
   
}


static int PWMWritePeriod (int pwmnum, int value)
{
   char s_values_str[VALUE_MAX];
   char path[VALUE_MAX];
   int fd,byte;
   
   snprintf(path, VALUE_MAX, "/sys/class/pwm/pwmchip0/pwm%d/period", pwmnum);
   fd  = open(path, O_WRONLY);
   if (-1 ==fd){
      fprintf(stderr, "failed to open in period\n");
      return(-1);
      
   }
   
   byte= snprintf(s_values_str, 10, "%d", value );
   
   if (-1 ==write(fd, s_values_str, byte)){
      fprintf(stderr, "failed to write value in period!\n");
      close(fd);
      return -1;
      
   }
   close(fd);
   return(0);
}


static int PWMWriteDutyCycle(int pwmnum, int value)
{
   char path[VALUE_MAX];
   char s_values_str[VALUE_MAX];
   int fd,byte;
   
   snprintf(path, VALUE_MAX, "/sys/class/pwm/pwmchip0/pwm%d/duty_cycle", pwmnum);
   fd = open(path, O_WRONLY);
   if( -1 == fd){
      fprintf(stderr, "failed to open in duty cycle!\n");
      return(-1);
   }
   
   byte = snprintf(s_values_str, 10, "%d", value);
   if (-1 ==write(fd, s_values_str, byte)){
      fprintf(stderr, "failed to open in duty cycle\n");
      close(fd);
      return -1;
      
   }
   close(fd);
   return(0);
   
}

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
   
   //in out selection
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

void error_handling(char *message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}

void *button_thd() {
   int sock;
   int button;
   struct sockaddr_in b_serv_addr;
   char msg[2];

   sock = socket(PF_INET, SOCK_STREAM, 0);
   if (sock == -1)
      error_handling("socket() error");
   memset(&b_serv_addr, 0, sizeof(b_serv_addr));
   b_serv_addr.sin_family = AF_INET;
   b_serv_addr.sin_addr.s_addr = inet_addr("192.168.0.35");         // IP 
   b_serv_addr.sin_port = htons(7087);                                 // port 1
   if (connect(sock, (struct sockaddr *)&b_serv_addr, sizeof(b_serv_addr)) == -1)
      error_handling("connect() error1");
   
   printf("button thread is opened\n");   
   while (1)
   {
      //printf("in while button %d\n\n", button);
      button = read(sock, msg, sizeof(msg));
      printf("in while button %d\n\n", button);

      if (button == -1)
         error_handling("read() error");
      
      if(button != 0){
         printf("Receive button data from Server : %s \n", msg);
         g_button = atoi(msg);
      }
   }
   close(sock);

}


void *pressure_thd(){
    int sock;
    int pressure;
    struct sockaddr_in p_serv_addr;
    char msg[4];

    sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock == -1)
        error_handling("socket() error");
   memset(&p_serv_addr, 0, sizeof(p_serv_addr));
   p_serv_addr.sin_family = AF_INET;
   p_serv_addr.sin_addr.s_addr = inet_addr("192.168.0.28");            // IP 
   p_serv_addr.sin_port = htons(7777);                                   //port 2
    if (connect(sock, (struct sockaddr *)&p_serv_addr, sizeof(p_serv_addr)) == -1)
        error_handling("connect() error2");
    printf("here!!!!!!!");
    while(1)
    {
   printf("in while\n\n\n");
   pressure =read(sock,msg,sizeof(msg));
   
   if (pressure == -1)
            error_handling("read() error");
   /*    
   if (pressure > 1){
      g_oressure = atoi(msg);
      sleep(2);
   }
   */  
       
   g_pressure= atoi(msg);    
   printf("Receive pressure data from Server : %s\n\n", msg);    
   sleep(1);
    }
    close(sock);

}


void *ultrawave_thd(){
    int sock;
    int ultrawave;
    struct sockaddr_in u_serv_addr;
    char msg[12];

    sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock == -1)
        error_handling("socket() error");
   memset(&u_serv_addr, 0, sizeof(u_serv_addr));
   u_serv_addr.sin_family = AF_INET;
   u_serv_addr.sin_addr.s_addr = inet_addr("192.168.0.35");                   // IP 
   u_serv_addr.sin_port = htons(6879);                                       // port 1'
    if (connect(sock, (struct sockaddr *)&u_serv_addr, sizeof(u_serv_addr)) == -1)
        error_handling("connect() error3");
   
    printf("ultrawave thread is opened\n\n");   
    while(1)
    {
   printf("in while ultra wave \n\n");
   ultrawave =read(sock,msg,sizeof(msg));
   printf("read done\n\n");
   if (ultrawave == -1)
            error_handling("read() error");
   printf("Receive Ultrawave data from Server : %s\n", msg);   
   g_ultrawave = atoi(msg); 
   sleep(1);
    }
    close(sock);

}




int main(int argc, char *argv[])
{
   
   int b_check, u_check, p_check;
   pthread_t p_thread[3];
   b_check = pthread_create(&p_thread[0], NULL, button_thd, NULL);     //button thread
   if (b_check < 0) {
      perror("button create error : \n");
      exit(0);
   }
   
   u_check = pthread_create(&p_thread[1], NULL, ultrawave_thd, NULL);   // ultrawave thread
   if (u_check < 0) {
      perror("ultrawave thread create error : \n");
      exit(0);
   }
   
   p_check = pthread_create(&p_thread[2], NULL, pressure_thd, NULL);   // pressure thread
   if (p_check < 0) {
      perror("pressure thread create error : \n");
      exit(0);
   }
   
   PWMExport(0); 
   PWMWritePeriod(0, 10000000);
   PWMWriteDutyCycle(0,3200000);
   
   if (-1 == GPIOExport(POUT) || -1 == GPIOExport(RED)|| -1 == GPIOExport(BLUE)|| -1 == GPIOExport(GREEN))                     //walker LED export
      return (1);
   usleep(50000);
   
   if (-1 == GPIODirection(POUT, OUT) || -1 == GPIODirection(RED,OUT)|| -1 == GPIODirection(BLUE, OUT)|| -1 == GPIODirection(GREEN,OUT))             //walker LED direction
      return (2);
      
   usleep(50000);
   
   
   
   
   
   printf("output in here5 \n\n\n\n\n\n g_pressure : %d \n g_ultrawave : %d\n, g_button : %d\n\n\n", g_pressure,g_ultrawave,g_button);
   
   while(1){
       printf("output in while \n\n\n\n\n\n g_pressure : %d \n g_ultrawave : %d\n, g_button : %d\n\n\n", g_pressure,g_ultrawave,g_button);
       if(g_button != 9){                                   //walker signal on
          if(g_pressure > 1 && g_ultrawave < 20){
            printf("This is situation A\n\n\n\n\n\n\n");
            //RGB RED output
            usleep(50000);
    
            PWMEnable(0);
            usleep(100000);
            //PWMUnable(0);
             
         if(-1 == GPIOWrite(POUT,1)|| -1 == GPIOWrite(RED,OUT))
            return (3);
         
         usleep(100000);   
         PWMUnable(0);
         if(-1 == GPIOWrite(POUT,0)|| -1 == GPIOWrite(RED,0))
            return (3);
      }
      
      
          else if(g_pressure > 1 && g_ultrawave >20){//b
             printf("This is situation B\n\n\n\n\n\n\n");
             //RGB RED output
             usleep(50000);
         if(-1 == GPIOWrite(RED,OUT))
            return (3);
         
         usleep(100000);   
         if(-1 == GPIOWrite(RED,0))
            return (3);
          }
      
          else if(g_pressure < 1 && g_ultrawave < 20){//c
             printf("This is situation C\n\n\n\n\n\n\n");
             //RGB Green output
             usleep(50000);
         
             
             PWMEnable(0);
         usleep(100000);
         
             
         if(-1 == GPIOWrite(POUT,1) || -1 == GPIOWrite(GREEN,1))
            return (3);
         usleep(100000);
         if(-1 == GPIOWrite(POUT,0)|| -1 == GPIOWrite(GREEN,0))
            return (3);   
            
         PWMUnable(0);
          }
      
          else if(g_pressure < 1 && g_ultrawave >20){//d
             printf("This is situation D\n\n\n\n\n\n\n");
             //RGB yellow output
             if(-1 == GPIOWrite(GREEN,1) || -1 == GPIOWrite(RED,1))
            return (3);
             usleep(100000);
             if(-1 == GPIOWrite(RED,0)|| -1 == GPIOWrite(GREEN,0))
            return (3);
               
          }
       }
   
       else if (g_button == 9){                                       //walker signal off, exit program
          printf("no walker signal \n\n\n");
          PWMUnable(0);
          PWMUnexport(PWM);
   
          if (-1 == GPIOUnexport(POUT))
             return (4);
          if (-1 == GPIOUnexport(RED))
             return (4);
          if (-1 == GPIOUnexport(BLUE))
             return (4);
          if (-1 == GPIOUnexport(GREEN))
             return (4);              
          break;
       }
       sleep(1);
   }
   
   pthread_cancel(p_thread[0]);
   pthread_cancel(p_thread[1]);
   pthread_cancel(p_thread[2]);
   
   printf("\n\n\n\n\n g_button : %d \n g_ultrawave : %d, \n g_pressure : %d", g_button, g_ultrawave, g_pressure);
   return 0;
    
}