#pragma once
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <stdint.h>
#include <getopt.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>

#include <time.h>
#include <pthread.h>
#include <signal.h>

#define IN   0
#define OUT  1
#define LOW  0
#define HIGH 1

#define PWM 0

#define BUFFER_MAX 3
#define DIRECTION_MAX 45
#define VALUE_MAX 256

#define ARRAY_SIZE(array) sizeof(array) / sizeof(array[0])

static const char *DEVICE = "/dev/spidev0.0";
static uint8_t MODE = SPI_MODE_0;
static uint8_t BITS = 8;
static uint32_t CLOCK = 1000000;
static uint16_t DELAY = 5;

static int PWMExport(int pwmnum);
static int PWMUnexport(int pwmnum);
static int PWMEnable(int pwmnum);
static int PWMUnable(int pwmnum);
static int PWMWritePeriod(int pwmnum, int value);
static int PWMWriteDutyCycle(int pwmnum, int value);

static int prepare(int fd);
uint8_t control_bits_differential(uint8_t channel);
uint8_t control_bits(uint8_t channel);
int readadc(int fd, uint8_t channel);

void *pressure_1_thd();
void *pressure_2_thd();
void *pres_sensor_act();
