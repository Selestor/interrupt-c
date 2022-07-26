/* device specifics, such as ioctl numbers and the
 * major device file. */
#include "chardev.h"

#include <linux/input.h>
#include <linux/keyboard.h>
#include <linux/module.h>

#include <linux/ktime.h>

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Me");
MODULE_DESCRIPTION("Count keyboard presses");

//static struct kobject *interrupt_module;
static unsigned int interrupt_count = 0;
static unsigned long seconds = 0;

// sysfs class structure
static struct class *mychardev_class = NULL;

static unsigned long get_current_time(void);
static int interupt_callback(struct notifier_block *nblock, unsigned long code, void *_param);
static int mychardev_open(struct inode *inode, struct file *file);
static int mychardev_release(struct inode *inode, struct file *file);
static long interrupt_ioctl(struct file *file, unsigned int ioctl_num, unsigned long ioctl_param);
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
            if( copy_to_user((unsigned int __user *)ioctl_param, &interrupt_count, sizeof(interrupt_count)) ) {
                ret = -EFAULT;
            }
            break;
        }
        case IOCTL_RESET_COUNTER: {
            interrupt_count = 0;
            seconds = get_current_time();
            break;
        }
        case IOCTL_GET_RESET_DATE:
            if( copy_to_user((unsigned long __user *)ioctl_param, &seconds, sizeof(seconds)) ) {
                ret = -EFAULT;
            }
            break;
        }

    return ret;
}

static int mychardev_open(struct inode *inode, struct file *file)
{
    printk("MYCHARDEV: Device open\n");
    return 0;
}

static int mychardev_release(struct inode *inode, struct file *file)
{
    printk("MYCHARDEV: Device close\n");
    return 0;
}

// called when keyboard event raises
int interupt_callback(struct notifier_block *nblock, unsigned long code, void *_param) {
    struct keyboard_notifier_param *param = _param;

    if(param->down) {
        if(param->value > KEY_RESERVED && param->value < KEY_MAX) {
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
    mychardev_class = class_create(THIS_MODULE, "class_create_device");
    mychardev_class->dev_uevent = mychardev_uevent;
    device_create(mychardev_class, NULL, device, NULL, DEVICE_FILE_NAME);

    pr_info("Device created on /dev/%s\n", DEVICE_FILE_NAME);

    // register notifier_block with keyboard events
    register_keyboard_notifier(&interupt_block);
    return error;
}

static void __exit interupt_exit(void) {
    device_destroy(mychardev_class, MKDEV(MAJOR_NUM, 0));
    class_destroy(mychardev_class);

    /* Unregister the character device */
    unregister_chrdev(MAJOR_NUM, DEVICE_FILE_NAME);
    // unregister keyboard notifier that tracks interrupts
    unregister_keyboard_notifier(&interupt_block);
}

module_init(interupt_init);
module_exit(interupt_exit);