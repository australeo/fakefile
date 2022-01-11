#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <asm/uaccess.h>

//struct decls
const struct file_operations fake_fops = {
    .owner = THIS_MODULE,
    .open = fake_open,
    .read = fake_read,
    .write = fake_write,
    .release = fake_release
};

struct fakefile_s {
    size_t size;
    size_t offset;
    char* buffer;
};

//func decls
int init_module(void);
void cleanup_module(void);
static int device_open(struct inode *, struct file *);
static int device_release(struct inode *, struct file *);
static ssize_t device_read(struct file *, char *, size_t, loff_t *);
static ssize_t device_write(struct file *, const char *, size_t, loff_t *);

//globals
static const char DEVICE_NAME[] = "fakefile";
static const int FAKE_MAJOR = 42;
static const int FAKE_MINOR = 7;
static const size_t fake_size = 4096;

static int major_num;
static int user_count;
static fakefile_s fakefile = { 0 };

//functions

///
/// Open the device
///
static int device_open(struct inode *inode, struct file *file)
{
	user_count++;

	if(fakefile.buffer = 0) {
        fakefile.offset = 0;
        fakefile.buffer = kmalloc(fakefile.size, GFP_KERNEL);
        if(fakefile.buffer == '\0') {
            printk(KERN_ALERT "Failed to allocate %d bytes for fake file buffer.\n", fake_size);
            return -1;
        }
        memset(fakefile.buffer, 0, fakefile.size);
    }

	try_module_get(THIS_MODULE);

    printk(KERN_ALERT "Device opened.\n");

	return SUCCESS;
}

///
/// Close the device
///
static int device_release(struct inode *inode, struct file *file)
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
static ssize_t device_read(struct file *filp, char __user *user_buffer, 
                           size_t read_length, loff_t *offset)
{
	if (read_length + fakefile.offset > fakefile.size)
		return 0;

	unsigned long result = __copy_to_user(user_buffer, fakefile.buffer, read_length);

    size_t bytes_read = read_length - result;
    fakefile.offset += bytes_read;

	return bytes_read;
}

///
/// Write to device
///
static int device_write(struct file *file, const char __user *user_buffer,
                    size_t write_length, loff_t * offset)
{
    if (write_length + fakefile.offset > fakefile.size)
		return 0;
    
    unsigned long result = __copy_from_user(user_buffer, fakefile.buffer, write_length);

    size_t bytes_written = write_length - result;
    fakefile.offset += bytes_written;

	return bytes_written;
}

///
/// Setup and register the device
///
int init_module(void)
{
    major_num = register_chrdev(0, DEVICE_NAME, &fops);

	if (major_num < 0) {
	    printk(KERN_ALERT "Registering char device failed with %d.\n", major_num);
	    return major_num;
	}

    fakefile.size = fake_size;
    fakefile.offset = 0;
    fakefile.buffer = 0;

	printk(KERN_INFO "Major number is %d.n", major_num);
	printk(KERN_INFO "Open with:\n");
	printk(KERN_INFO "`mknod /dev/%s c %d 0`\n", DEVICE_NAME, major_num);

	return SUCCESS;
}

//
// Cleanup and unload the device
//
void cleanup_module(void)
{
    if(fakefile.buffer != 0) {
        kfree(fakefile.buffer);
    }

	int ret = unregister_chrdev(major_num, DEVICE_NAME);
	if (ret < 0)
		printk(KERN_ALERT "Error in unregister_chrdev: %d\n", ret);
}