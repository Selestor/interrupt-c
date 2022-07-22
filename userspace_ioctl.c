/*  userspace_ioctl.c - the process to use ioctl's to control the kernel module
 *
 *  Until now we could have used cat for input and output.  But now
 *  we need to do ioctl's, which require writing our own process.
 */

/* device specifics, such as ioctl numbers and the
 * major device file. */
#include "chardev.h"

#include <stdio.h> /* standard I/O */
#include <fcntl.h> /* open */
#include <unistd.h> /* close */
#include <stdlib.h> /* exit */
#include <sys/ioctl.h> /* ioctl */
#include <time.h>

struct myTimeStruct *get_current_time(void) {
    struct myTimeStruct myTime;
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    myTime.year = tm.tm_year + 1900;
    myTime.month = tm.tm_mon + 1;
    myTime.day = tm.tm_mday;
    myTime.hour = tm.tm_hour;
    myTime.minute = tm.tm_min;
    myTime.second = tm.tm_sec;
    return &myTime;
}

/* Functions for the ioctl calls */
int ioctl_get_counter(int file_desc) {
    int ret_val;

    ret_val = ioctl(file_desc, IOCTL_GET_COUNTER);

    if (ret_val < 0) {
        printf("ioctl_get_counter failed:%d\n", ret_val);
    }

    return ret_val;
}

int ioctl_reset_counter(int file_desc) {
    int ret_val;
    struct myTimeStruct *time;

    time = get_current_time();
    ret_val = ioctl(file_desc, IOCTL_RESET_COUNTER, time);

    if (ret_val < 0) {
        printf("ioctl_reset_counter failed:%d\n", ret_val);
    }

    return ret_val;
}

void ioctl_get_reset_date(int file_desc, char* buff) {
    buff = ioctl(file_desc, IOCTL_GET_RESET_DATE);
}

/* Main - Call the ioctl functions */
int main(int argc, char *argv[]) {
    int file_desc, ret_val, option;

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
                    printf("Interrupt count: %d\n", ret_val);
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