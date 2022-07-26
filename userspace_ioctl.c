/* device specifics, such as ioctl numbers and the
 * major device file. */
#include "chardev.h"

#include <stdio.h> /* standard I/O */
#include <fcntl.h> /* open */
#include <unistd.h> /* close */
#include <stdlib.h> /* exit */
#include <sys/ioctl.h> /* ioctl */

#include <time.h>

static unsigned int counter;
static unsigned int *counter_pointer = &counter;

static unsigned long myTime;
static unsigned long *myTime_pointer = &myTime;
static char reset_time[32];

int ioctl_get_counter(int file_desc);
int ioctl_reset_counter(int file_desc);
long ioctl_get_reset_date(int file_desc);
void seconds_to_date(unsigned long seconds);

/* Functions for the ioctl calls */
int ioctl_get_counter(int file_desc) {
    int ret_val;

    ret_val = ioctl(file_desc, IOCTL_GET_COUNTER, counter_pointer);
    if (ret_val < 0) {
        printf("ioctl_get_counter failed:%d\n", ret_val);
    }

    return ret_val;
}

int ioctl_reset_counter(int file_desc) {
    int ret_val;

    ret_val = ioctl(file_desc, IOCTL_RESET_COUNTER);

    if (ret_val < 0) {
        printf("ioctl_reset_counter failed:%d\n", ret_val);
    }

    return ret_val;
}

long ioctl_get_reset_date(int file_desc) {
    long ret_val;

    ret_val = ioctl(file_desc, IOCTL_GET_RESET_DATE, myTime_pointer);

    if (ret_val < 0) {
        printf("ioctl_reset_date failed:%ld\n", ret_val);
    }

    return ret_val;
}

void seconds_to_date(unsigned long seconds) {
    struct tm *tm = localtime(&seconds);
    strftime(reset_time, sizeof(reset_time), "%Y/%m/%d/%H:%M:%S", tm);
}

/* Main - Call the ioctl functions */
int main(int argc, char *argv[]) {
    int file_desc, ret_val, option;
    long seconds;

    file_desc = open(DEVICE_PATH, O_RDWR);
    if (file_desc < 0) {
        printf("Can't open device file: %s, error:%d\n", DEVICE_PATH,
               file_desc);
        exit(EXIT_FAILURE);
    }

    option = getopt(argc, argv, "srd"); // take just one, options are mutually exclusive
    if(option != -1){ //if there is any
        switch(option){
            case 's':
                printf("Show interrupt option.\n");
                ret_val = ioctl_get_counter(file_desc);
                if (ret_val < 0)
                    goto error;
                else
                    printf("Interrupt count: %d\n", counter);
                break;
            case 'r':
                printf("Reset interrupt option.\n");
                ret_val = ioctl_reset_counter(file_desc);
                if (ret_val < 0)
                    goto error;
                printf("Interrupt count reset.\n");
                break;
            case 'd':
                printf("Show reset date option.\n");
                ret_val = ioctl_get_reset_date(file_desc);
                if (ret_val < 0)
                    goto error;
                else
                    seconds_to_date(myTime);
                    printf("Interrupt reset date: %s\n", reset_time);
                break;
        }
    } else
        printf("No option selected. use -s/-r/-d to show count/reset count/show reset date.\n");
    close(file_desc);
    return 0;
error:
    close(file_desc);
    exit(EXIT_FAILURE);
}