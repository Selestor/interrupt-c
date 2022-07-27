/* device specifics, such as ioctl numbers and the
 * major device file. */
#include "chardev.h"

#include <linux/input.h>
#include <linux/keyboard.h> // to be able to get interrupts from keyboard
#include <linux/module.h>
#include <linux/ktime.h> // to get current time in kernel
#include <linux/kdev_t.h> // for MKDEV(major, minor)
#include <linux/err.h> // for ERR_PTR
#include <asm-generic/errno-base.h> // for EBUSY

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Me");
MODULE_DESCRIPTION("Count keyboard presses");


// used to keep track if device is in use by .open and .release
static unsigned int device_in_use = 0;

// data
static unsigned int interrupt_count = 0;
static unsigned long seconds = 0;

// sysfs class structure
static struct class *mychardev_class = NULL;

static unsigned long get_current_time(void);
static int interupt_callback(struct notifier_block *nblock, unsigned long code, void *_param);
static int mychardev_open(struct inode *inode, struct file *file);
static int mychardev_release(struct inode *inode, struct file *file);
static long interrupt_ioctl(struct file *file, unsigned int ioctl_num, unsigned long ioctl_param);
static long get_counter_ioctl(unsigned int *ptr_to_user);
static long get_reset_date_ioctl(unsigned long *ptr_to_user);
static int mychardev_uevent(struct device *dev, struct kobj_uevent_env *env);

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = mychardev_open,
    .release = mychardev_release,
    .unlocked_ioctl = interrupt_ioctl,
};

//this block is registered with keyboard events
static struct notifier_block interupt_block = {
    .notifier_call = interupt_callback,
};

static unsigned long get_current_time() {
    unsigned long ret;
    struct timespec64 now;

    ktime_get_real_ts64(&now);
    ret = now.tv_sec;
    return ret;
}

// this is called when ioctl is called
static long interrupt_ioctl(struct file *file, unsigned int ioctl_num, unsigned long ioctl_param) {
    long ret = 0;
    printk("Device ioctl called\n");

    /* Switch according to the ioctl called */
    switch (ioctl_num) {
        case IOCTL_GET_COUNTER: {
            ret = get_counter_ioctl((unsigned int *)ioctl_param);
            break;
        }
        case IOCTL_RESET_COUNTER: {
            interrupt_count = 0;
            seconds = get_current_time();
            break;
        }
        case IOCTL_GET_RESET_DATE:
            ret = get_reset_date_ioctl((unsigned long *)ioctl_param);
            break;
        }

    return ret;
}

static long get_counter_ioctl(unsigned int *ptr_to_user) {
    long ret = 0;
    if (copy_to_user((unsigned int __user *)ptr_to_user, &interrupt_count, sizeof(interrupt_count)))
        ret = -EFAULT;
    return ret;
}

static long get_reset_date_ioctl(unsigned long *ptr_to_user) {
    long ret = 0;
    if(copy_to_user((unsigned long __user *)ptr_to_user, &seconds, sizeof(seconds)))
        ret = -EFAULT;
    return ret;
}

static int mychardev_open(struct inode *inode, struct file *file)
{
    printk("MYCHARDEV: Device open\n");

    // use some var to keep track if device is used in any way
    if (device_in_use)
        return -EBUSY;
    device_in_use++;

    // set as 1st date of interrupt
    seconds = get_current_time();

    return 0;
}

static int mychardev_release(struct inode *inode, struct file *file)
{
    pr_info("device_release(%p,%p)\n", inode, file);

    // remember to release the device in the same way
    device_in_use--;

    return 0;
}

// called when keyboard event raises
int interupt_callback(struct notifier_block *nblock, unsigned long code, void *_param) {
    struct keyboard_notifier_param *param = _param;

    if (param->down) {
        if (param->value > KEY_RESERVED && param->value < KEY_MAX) {
            interrupt_count++;
            pr_info("Keyboard interupt count: %d, value: %d\n", interrupt_count, param->value);
        }
    }

    return NOTIFY_OK;
}

// add permission to character device so it can be read/written/opened
static int mychardev_uevent(struct device *dev, struct kobj_uevent_env *env) {
    add_uevent_var(env, "DEVMODE=%#o", 0666);
    return 0;
}

static int __init interupt_init(void) {
    int error = 0;
    int device = 0;

    /* Register the character device (atleast try) */
    int ret_val = register_chrdev(MAJOR_NUM, DEVICE_FILE_NAME, &fops);

    /* Negative values signify an error */
    if (ret_val < 0) {
        pr_alert("%s failed with %d\n", "Sorry, registering the character device ", ret_val);
        return ret_val;
    }

    device = MKDEV(MAJOR_NUM, 0);

    // create sysfs class; /sys/devices/virtual/interrupt_sysfs
    mychardev_class = class_create(THIS_MODULE, "interrupt_sysfs"); // handle error
    if (mychardev_class == (void *)ERR_PTR)
        goto class_create_error;
    // add permissions to read/open
    mychardev_class->dev_uevent = mychardev_uevent;

    // this creates device 'file' called DEVICE_FILE_NAME
    if (device_create(mychardev_class, NULL, device, NULL, DEVICE_FILE_NAME) == (void *)ERR_PTR)
        goto device_create_error;
    pr_info("Device created on /dev/%s\n", DEVICE_FILE_NAME);

    // register notifier_block with keyboard events
    register_keyboard_notifier(&interupt_block);
    return error;

device_create_error:
    class_destroy(mychardev_class);
class_create_error:
    unregister_chrdev(MAJOR_NUM, DEVICE_FILE_NAME);

    return error;
}

static void __exit interupt_exit(void) {
    // unregister keyboard notifier that tracks interrupts
    unregister_keyboard_notifier(&interupt_block);

    // destroy char device
    device_destroy(mychardev_class, MKDEV(MAJOR_NUM, 0));
    // destroy char device class
    class_destroy(mychardev_class);
    /* Unregister the character device */
    unregister_chrdev(MAJOR_NUM, DEVICE_FILE_NAME);
}

module_init(interupt_init);
module_exit(interupt_exit);