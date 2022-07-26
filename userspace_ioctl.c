/* device specifics, such as ioctl numbers and the
 * major device file. */
#include "chardev.h"

#include <stdio.h> /* standard I/O */
#include <fcntl.h> /* open */
#include <unistd.h> /* close */
#include <stdlib.h> /* exit */
#include <sys/ioctl.h> /* ioctl */

#include <time.h>

static char reset_date[32];

int ioctl_get_counter(int file_desc, unsigned int *counter);
int ioctl_reset_counter(int file_desc);
long ioctl_get_reset_date(int file_desc, unsigned long *timep);
void seconds_to_date(unsigned long seconds);

/* Functions for the ioctl calls */
int ioctl_get_counter(int file_desc, unsigned int *counter) {
    int ret_val;

    ret_val = ioctl(file_desc, IOCTL_GET_COUNTER, counter);
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

long ioctl_get_reset_date(int file_desc, unsigned long *timep) {
    long ret_val;

    ret_val = ioctl(file_desc, IOCTL_GET_RESET_DATE, timep);

    if (ret_val < 0) {
        printf("ioctl_reset_date failed:%ld\n", ret_val);
    }

    return ret_val;
}

void seconds_to_date(unsigned long seconds) {
    struct tm *tm = localtime(&seconds);
    strftime(reset_date, sizeof(reset_date), "%Y/%m/%d/%H:%M:%S", tm);
}

/* Main - Call the ioctl functions */
int main(int argc, char *argv[]) {
    int file_desc, ret_val, option;
    unsigned int counter = 0;
    unsigned long time_sec = 0;

    file_desc = open(DEVICE_PATH, O_RDWR);
    if (file_desc < 0) {
        printf("Can't open device file: %s, error:%d\n", DEVICE_PATH,
               file_desc);
        exit(EXIT_FAILURE);
    }

    option = getopt(argc, argv, "srd"); // take just one, options are mutually exclusive
    if (option != -1) { //if there is any
        switch(option) {
            case 's':
                printf("Show interrupt option.\n");
                ret_val = ioctl_get_counter(file_desc, &counter);
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
                ret_val = ioctl_get_reset_date(file_desc, &time_sec);
                if (ret_val < 0)
                    goto error;
                else
                    seconds_to_date(time_sec);
                    printf("Interrupt reset date: %s\n", reset_date);
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