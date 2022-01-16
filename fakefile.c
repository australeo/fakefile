#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/uaccess.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/kdev_t.h>
#include <linux/slab.h>

MODULE_LICENSE("GPL");

//func decls
int init_module(void);
void cleanup_module(void);
static int fake_open(struct inode *, struct file *);
static int fake_release(struct inode *, struct file *);
static ssize_t fake_read(struct file *filp, char __user *user_buffer, 
                           size_t read_length, loff_t *offset);
static ssize_t fake_write(struct file *file, const char __user *user_buffer,
                    size_t write_length, loff_t * offset);
static loff_t fake_seek(struct file* file, loff_t offset, int whence);

//struct decls
const struct file_operations fake_fops = {
    .owner = THIS_MODULE,
    .open = fake_open,
    .read = fake_read,
    .write = fake_write,
    .release = fake_release,
    .llseek = fake_seek
};

struct fakefile_s {
    size_t size;
    loff_t offset;
    char* buffer;
};

//globals
static const char DEVICE_NAME[] = "fakefile";
static const int FAKE_MAJOR = 42;
static const int FAKE_MINOR = 7;
static const size_t fake_size = 1024;

static int major_num;
static int user_count;
static struct fakefile_s fakefile = { 0 };

//functions

///
/// Open the device
///
static int fake_open(struct inode *inode, struct file *file)
{
	user_count++;

    try_module_get(THIS_MODULE);
    printk(KERN_ALERT "Device opened.\n");

	if(fakefile.buffer == 0) {
        fakefile.offset = 0;

        fakefile.buffer = kmalloc(fakefile.size, GFP_KERNEL);
        if(fakefile.buffer == 0) {
            printk(KERN_ALERT "Failed to allocate %ld bytes for fake file buffer.\n", fake_size);
            return -1;
        }
        
        memset(fakefile.buffer, 0, fakefile.size);
    }

	return 0;
}

///
/// Close the device
///
static int fake_release(struct inode *inode, struct file *file)
{
	user_count--;

    if(user_count == 0) {
        kfree(fakefile.buffer);
        fakefile.buffer = 0;
    }

	module_put(THIS_MODULE);

    printk(KERN_ALERT "Device closed.\n");

	return 0;
}

///
/// Read from the device
///
static ssize_t fake_read(struct file *filp, char __user *user_buffer, 
                           size_t read_length, loff_t *offset)
{
    ssize_t checked_read_length;

    if(fakefile.size - fakefile.offset < read_length) {
        checked_read_length = fakefile.size - fakefile.offset;
    } else {
        checked_read_length = read_length;
    }

    if(!access_ok(user_buffer, checked_read_length)) {
        printk(KERN_ALERT "Invalid user address `%p` provided!\n", user_buffer);
        return 0;
    }

    int result = copy_to_user(user_buffer, fakefile.buffer, checked_read_length);
    if(result != 0) {
        printk(KERN_ALERT "Error calling `copy_to_user`: %d\n", result);
        return 0;
    }

    fakefile.offset += checked_read_length;

	return checked_read_length;
}

///
/// Write to device
///
static ssize_t fake_write(struct file *file, const char __user *user_buffer,
                    size_t write_length, loff_t * offset)
{
    size_t checked_write_length;
    if(fakefile.size - fakefile.offset < write_length) {
        checked_write_length = fakefile.size - fakefile.offset;
    } else {
        checked_write_length = write_length;
    }
    
    if(!access_ok(user_buffer, checked_write_length)) {
        printk(KERN_ALERT "Invalid user address `%p` provided!\n", user_buffer);
        return 0;
    }
    
    int result = copy_from_user(fakefile.buffer, user_buffer, checked_write_length);
    if(result != 0) {
        printk(KERN_ALERT "Error calling `copy_from_user`: %d\n", result);
        return 0;
    }

    fakefile.offset += checked_write_length;

	return checked_write_length;
}


static loff_t fake_seek(struct file* file, loff_t offset, int whence)
{   
    if(offset < 0) {
        return -1;
    }

    switch (whence)
    {
    case SEEK_SET:
        if(offset > fakefile.size) {
            return -1;
        }

        fakefile.offset = offset;
        break;

    case SEEK_CUR:
        if(offset + fakefile.offset > fakefile.size) {
            return -1;
        }

        fakefile.offset += offset;
        break;

    case SEEK_END:
        if(offset != 0) {
            return -1;
        }

        fakefile.offset = fakefile.size - 1;
        break;

    default:
        return -1;
    }
    
    return 0;
}

///
/// Setup and register the device
///
int init_module(void)
{
    major_num = register_chrdev(0, DEVICE_NAME, &fake_fops);

	if (major_num < 0) {
	    printk(KERN_ALERT "Registering char device failed with %d.\n", major_num);
	    return major_num;
	}

    printk(KERN_ALERT "Registering `%s` driver.\n", DEVICE_NAME);

    fakefile.size = fake_size;
    fakefile.offset = 0;
    fakefile.buffer = 0;

	printk(KERN_ALERT "Open with:\n");
	printk(KERN_ALERT "`mknod /dev/%s c %d 0`\n", DEVICE_NAME, major_num);

	return 0;
}

//
// Cleanup and unload the device
//
void cleanup_module(void)
{
    printk(KERN_ALERT "Unregistering `%s` driver.\n", DEVICE_NAME);

    if(fakefile.buffer != 0) {
        kfree(fakefile.buffer);
    }

	unregister_chrdev(major_num, DEVICE_NAME);
}