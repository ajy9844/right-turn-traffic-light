#include "pressure.h"

int fd = -1;
int pressure1 = 0;
int pressure2 = 0;

/*
 * Ensure all settings are correct for the ADC
 */
static int prepare(int fd) 
{
    if (ioctl(fd, SPI_IOC_WR_MODE, &MODE) == -1) {
        perror("Can't set MODE");
        return -1;
    }

    if (ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &BITS) == -1) {
        perror("Can't set number of BITS");
        return -1;
    }

    if (ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &CLOCK) == -1) {
        perror("Can't set write CLOCK");
        return -1;
    }

    if (ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ, &CLOCK) == -1) {
        perror("Can't set read CLOCK");
        return -1;
    }

    return 0;
}

/*
 * (SGL/DIF = 0, D2=D1=D0=0)
 */
uint8_t control_bits_differential(uint8_t channel) 
{
    return (channel & 7) << 4;
}

/*
 * (SGL/DIF = 1, D2=D1=D0=0)
 */
uint8_t control_bits(uint8_t channel) 
{
    return 0x8 | control_bits_differential(channel);
}

/*
 * Given a prep'd descriptor, and an ADC channel, fetch the
 * raw ADC value for the given channel.
 */
int readadc(int fd, uint8_t channel) 
{
    uint8_t tx[] = {1, control_bits(channel), 0};
    uint8_t rx[3];

    struct spi_ioc_transfer tr = {
        .tx_buf = (unsigned long)tx,
        .rx_buf = (unsigned long)rx,
        .len = ARRAY_SIZE(tx),
        .delay_usecs = DELAY,
        .speed_hz = CLOCK,
        .bits_per_word = BITS,          
    };

    if (ioctl(fd, SPI_IOC_MESSAGE(1), &tr) == 1) {
        perror("IO Error");
        abort();
    }

    return ((rx[1] << 8) & 0x300) | (rx[2] & 0xFF);
}

static int PWMExport(int pwmnum)
{
    char buffer[BUFFER_MAX];
    int bytes_written;
    int fd;

    fd = open("/sys/class/pwm/pwmchip0/export", O_WRONLY);
    if (-1 == fd) {
        fprintf(stderr, "Failed to open in export!\n");
        return(-1);
    }
    bytes_written = snprintf(buffer, BUFFER_MAX, "%d", pwmnum);
    write(fd, buffer, bytes_written);
    close(fd);
    sleep(1);
    return(0);
}

static int PWMUnexport(int pwmnum)
{
    char buffer[BUFFER_MAX];
    int bytes_written;
    int fd;
    
    fd = open("/sys/class/pwm/pwmchip0/unexport", O_WRONLY);
    if (-1 == fd) {
        fprintf(stderr, "Failed to open in unexport!\n");
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
        fprintf(stderr, "Failed to open in enable!(x)\n");
        return(-1);
    }

    write(fd, s_unenable_str, strlen(s_unenable_str));
    close(fd);

    fd = open(path, O_WRONLY);
    if (-1 == fd) {
        fprintf(stderr, "Failed to open in enable!(o)\n");
        return(-1);
    }

    write(fd, s_enable_str, strlen(s_enable_str));
    close(fd);
    return(0);
}

static int PWMUnable(int pwmnum)
{
    static const char s_unenable_str[] = "0";
    
    char path[DIRECTION_MAX];
    int fd;

    snprintf(path, DIRECTION_MAX, "/sys/class/pwm/pwmchip0/pwm%d/enable", pwmnum);
    fd = open(path, O_WRONLY);
    if (-1 == fd) {
        fprintf(stderr, "Failed to open in enable!(x)\n");
        return(-1);
    }

    write(fd, s_unenable_str, strlen(s_unenable_str));
    close(fd);

    return(0);
}

static int PWMWritePeriod(int pwmnum, int value)
{
    char s_values_str[VALUE_MAX];
    char path[VALUE_MAX];
    int fd, byte;

    snprintf(path, VALUE_MAX, "/sys/class/pwm/pwmchip0/pwm%d/period", pwmnum);
    fd = open(path, O_WRONLY);
    if (-1 == fd) {
        fprintf(stderr, "Failed to open in period!\n");
        return(-1);
    }

    byte = snprintf(s_values_str, 10, "%d", value);

    if (-1 == write(fd, s_values_str, byte)) {
        fprintf(stderr, "Failed to write value in period!\n");
        close(fd);
        return(-1);
    }

    close(fd);
    return(0);
}

static int PWMWriteDutyCycle(int pwmnum, int value)
{
    char path[VALUE_MAX];
    char s_values_str[VALUE_MAX];
    int fd, byte;

    snprintf(path, VALUE_MAX, "/sys/class/pwm/pwmchip0/pwm%d/duty_cycle", pwmnum);
    fd = open(path, O_WRONLY);
    if (-1 == fd) {
        fprintf(stderr, "Failed to open in duty_cycle!\n");
        return(-1);
    }

    byte = snprintf(s_values_str, 10, "%d", value);

    if (-1 == write(fd, s_values_str, byte)) {
        fprintf(stderr, "Failed to write value in duty_cycle!\n");
        close(fd);
        return(-1);
    }

    close(fd);
    return(0);
}

void *pressure_1_thd() 
{
    while (1) {
        pressure1 = readadc(fd, 0);
        printf("pressure_1 : %d\n", pressure1);
        usleep(50000);
    }
}

void *pressure_2_thd()
{
    while (1) {
        pressure2 = readadc(fd, 1);
        printf("pressure_2 : %d\n", pressure2);
        usleep(50000);
    }
}

void *pres_sensor_act()
{
    pthread_t p_thread[2];
    int thr_id;
    int status;
    
    char p2[] = "thread_2"; // pressure sensor 1
    char p3[] = "thread_3"; // pressure sensor 2

    fd = open(DEVICE, O_RDWR);
    if (fd <= 0) {
        printf("Device %s not found\n", DEVICE);
        exit(1);
    }

    if (prepare(fd) == -1) {
        printf("Force Sensing Resistors are not ready!\n");
        exit(1);
    }
    
    thr_id = pthread_create(&p_thread[0], NULL, pressure_1_thd, (void *)p2);
    if (thr_id < 0)
    {
        perror("thread_2 create error : ");
        exit(0);
    }

    thr_id = pthread_create(&p_thread[1], NULL, pressure_2_thd, (void *)p3);
    if (thr_id < 0)
    {
        perror("thread_3 create error : ");
        exit(0);
    }
    
    sleep(300); // 5_min
    pthread_cancel(p_thread[0]);
    pthread_cancel(p_thread[1]);
    printf("Force Sensing Resistors are terminated!\n");

    close(fd);
    PWMUnexport(PWM); // Disable PWM
}
