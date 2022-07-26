/*
 * chardev.h - the header file with the ioctl definitions.
 *
 * The declarations here have to be in a header file, because they need
 * to be known both to the kernel module (in interrupt.c) and the process
 * calling ioctl() (in userspace_ioctl.c).
 */

#ifndef CHARDEV_H
#define CHARDEV_H

#include <linux/ioctl.h>

/* The major device number. We can not rely on dynamic registration
 * any more, because ioctls need to know it.
 */
#define MAJOR_NUM 100

/* Get the message of the device driver */
#define IOCTL_GET_COUNTER _IOR(MAJOR_NUM, 0, unsigned int *)
/* This IOCTL is used for output, to get the message of the device driver.
 */

/* No data is sent or received, no need for _IOR or _IOW; use _IO*/
#define IOCTL_RESET_COUNTER _IO(MAJOR_NUM, 1)

/* Get the message of the device driver */
#define IOCTL_GET_RESET_DATE _IOR(MAJOR_NUM, 2, unsigned long *)
/* This IOCTL is used for output, to get the message of the device driver.
 */

/* The name of the device file */
#define DEVICE_FILE_NAME "interrupt_dev"
#define DEVICE_PATH "/dev/interrupt_dev"

#endif