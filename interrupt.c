#include <linux/debugfs.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/kernel.h>
#include <linux/keyboard.h>
#include <linux/kobject.h> 
#include <linux/module.h>
#include <linux/moduleparam.h>

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Me");
MODULE_DESCRIPTION("Count keyboard presses");

static struct kobject *interrupt_module;
static unsigned int interrupt_count = 0;

static int interupt_callback(struct notifier_block *nblock,
		unsigned long code,
		void *_param);

static ssize_t interrupt_count_show(struct kobject *kobj,
                               struct kobj_attribute *attr, char *buf) {
    return sprintf(buf, "%d\n", interrupt_count);
}

static ssize_t interrupt_count_store(struct kobject *kobj,
                                struct kobj_attribute *attr, char *buf,
                                size_t count)
{
    sscanf(buf, "%du", &interrupt_count);
    return count;
}

static struct kobj_attribute myvariable_attribute =
    __ATTR(interrupt_count, 0660, interrupt_count_show, (void *)interrupt_count_store);

//this block is registered with keyboard events
static struct notifier_block interupt_block = {
	.notifier_call = interupt_callback,
};

// called when keyboard event raises
int interupt_callback(struct notifier_block *nblock,
		  unsigned long code,
		  void *_param) {
	struct keyboard_notifier_param *param = _param;

	if(param->down) {
		if(param->value > KEY_RESERVED && param->value < KEY_MAX) {
			interrupt_count++;
			pr_info("Keyboard interupt count: %d, value: %d\n", interrupt_count, param->value);
		}
	}

	return NOTIFY_OK;
}

static int __init interupt_init(void) {
	int error = 0;
	
	interrupt_module = kobject_create_and_add("interruptModule", kernel_kobj);
    if (!interrupt_module)
        return -ENOMEM;

    error = sysfs_create_file(interrupt_module, &myvariable_attribute.attr);
    if (error) {
        pr_info("failed to create the interrupt_count file "
                "in /sys/kernel/interruptModule\n");
    }

	// register notifier_block with keyboard events
	register_keyboard_notifier(&interupt_block);
	return error;
}

static void __exit interupt_exit(void) {
	kobject_put(interrupt_module);
	// unregister what was registerd in __init
	unregister_keyboard_notifier(&interupt_block);
}

module_init(interupt_init);
module_exit(interupt_exit);