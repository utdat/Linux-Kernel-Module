#include <linux/init.h>    
#include <linux/module.h>  
#include <linux/device.h>  
#include <linux/kernel.h>  
#include <linux/fs.h>     
#include <linux/uaccess.h> 
#include <linux/namei.h>

#define DEVICE_NAME "hidefile" 
#define CLASS_NAME "hide"     
#define SUCCESS_FLAG 0

MODULE_LICENSE("GPL");
MODULE_AUTHOR("UTD_VND");
MODULE_DESCRIPTION("Driver to hide file");
MODULE_VERSION("1");

static int g_device_open = 0;
static int major_number;                      
static struct class *hide_file_class = NULL;  
static struct device *hide_file_device = NULL; 

static int dev_open(struct inode *, struct file *);
static int dev_release(struct inode *, struct file *);
static ssize_t dev_write(struct file *, const char *, size_t, loff_t *);

void allocate_memmory(void);
void reallocate_memmory(void);
unsigned long backup_functions(void);
unsigned long hook_functions(const char *file_path);

static struct file_operations fops =
{
	.owner = THIS_MODULE,
	.open = dev_open,
	.write = dev_write,
	.release = dev_release,
};

static int __init hidefile_init(void)
{
    printk(KERN_INFO "Hidefile: Initializing the hidefile LKM\n");

    major_number = register_chrdev(0, DEVICE_NAME, &fops);
    if (major_number < 0)
    {
        printk(KERN_ALERT "hidefile failed to register a major number\n");
        return major_number;
    }
    printk(KERN_INFO "Hidefile: registered correctly with major number %d\n", major_number);

    hide_file_class = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(hide_file_class))
    { 
        unregister_chrdev(major_number, DEVICE_NAME);
        printk(KERN_ALERT "Failed to register device class\n");
        return PTR_ERR(hide_file_class);
    }
    printk(KERN_INFO "Hidefile: device class registered correctly\n");

    hide_file_device = device_create(hide_file_class, NULL, MKDEV(major_number, 0), NULL, DEVICE_NAME);
    if (IS_ERR(hide_file_device))
    {                                
        class_destroy(hide_file_class); 
        unregister_chrdev(major_number, DEVICE_NAME);
        printk(KERN_ALERT "Failed to create the device\n");
        return PTR_ERR(hide_file_device);
    }
    printk(KERN_INFO "Hidefile: device class created correctly\n"); 
    return 0;
}

static void __exit hidefile_exit(void)
{
    backup_functions();
    device_destroy(hide_file_class, MKDEV(major_number, 0)); 
    class_unregister(hide_file_class);                      
    class_destroy(hide_file_class);                        
    unregister_chrdev(major_number, DEVICE_NAME);         
    printk(KERN_INFO "Hidefile: Goodbye from the LKM!\n");
}

static int dev_open(struct inode *inodep, struct file *filep)
{
    if (g_device_open)
        return -EBUSY;

    g_device_open++;
    try_module_get(THIS_MODULE);

    return SUCCESS_FLAG;
}

static ssize_t dev_write(struct file *file_ptr, const char *buffer, size_t length, loff_t *offset)
{
    char *pFile_Path;
    pFile_Path = (char *)kmalloc(sizeof(char *) * length, GFP_KERNEL);

    if (strncpy_from_user(pFile_Path, buffer, length) == -EFAULT)
    {
        printk(KERN_NOTICE "Entered in fault get_user state");
        length = -1;
        goto finish;
    }

    if (strstr(pFile_Path, "\n"))
    {
        pFile_Path[length - 1] = 0;
        printk(KERN_NOTICE "Entered in end line filter");
    }

    printk(KERN_NOTICE "File path is %s without EOF", pFile_Path);

    if (hook_functions(pFile_Path) == -1)
    {
        length = -2;
    }
finish:
    kfree(pFile_Path);
    return length;
}

static int dev_release(struct inode *inodep, struct file *filep)
{
    g_device_open--;
    module_put(THIS_MODULE);
    return SUCCESS_FLAG;
}

struct dentry *g_parent_dentry;
struct nameidata g_root_nd;

unsigned long *g_inode_numbers;
int g_inode_count = 0;

void **g_old_inode_pointer;
void **g_old_fop_pointer;
void **g_old_iop_pointer;

void **g_old_parent_inode_pointer;
void **g_old_parent_fop_pointer;

filldir_t real_filldir;

static int new_filldir(void *buf, const char *name, int namelen, loff_t offset, u64 ux64, unsigned ino)
{
    unsigned int i = 0;
    struct dentry *pDentry;
    struct qstr Current_Name;

    Current_Name.name = name;
    Current_Name.len = namelen;
    Current_Name.hash = full_name_hash(name, namelen);

    pDentry = d_lookup(g_parent_dentry, &Current_Name);
    if (pDentry != NULL)
    {
        for (i = 0; i <= g_inode_count - 1; i++)
        {
            if (g_inode_numbers[i] == pDentry->d_inode->i_ino)
            {
                return 0;
            }
        }
    }
    return real_filldir(buf, name, namelen, offset, ux64, ino);
}

int parent_readdir(struct file *file, void *dirent, filldir_t filldir)
{
    g_parent_dentry = file->f_dentry;
    real_filldir = filldir;
    return g_root_nd.path.dentry->d_inode->i_fop->readdir(file, dirent, new_filldir);
}

static struct file_operations new_parent_fop =
{
        .owner = THIS_MODULE,
        .readdir = parent_readdir,
};

int new_mmap(struct file *file, struct vm_area_struct *area)
{
    printk(KERN_ALERT "Entered in new_mmap\n");
    return -2;
}

ssize_t new_read(struct file *file1, char __user *u, size_t t, loff_t *ll)
{
    printk(KERN_ALERT "Entered in new_read\n");
    return -2;
}

ssize_t new_write(struct file *file1, const char __user *u, size_t t, loff_t *ll)
{
    printk(KERN_ALERT "Entered in new_write\n");
    return -2;
}

int new_release(struct inode *new_inode, struct file *file)
{
    printk(KERN_ALERT "Entered in new_release \n");
    return -2;
}

int new_flush(struct file *file, fl_owner_t id)
{
    printk(KERN_ALERT "Entered in new_flush \n");
    return -2;
}

int new_readdir(struct file *file, void *dirent, filldir_t filldir)
{
    printk(KERN_ALERT "Entered in new_readdir \n");
    return -2;
}

int new_open(struct inode *old_inode, struct file *old_file)
{
    printk(KERN_ALERT "Entered in new_open \n");
    return -2;
}

static struct file_operations new_fop =
{
        .owner = THIS_MODULE,
        .readdir = new_readdir,
        .release = new_release,
        .open = new_open,
        .read = new_read,
        .write = new_write,
        .mmap = new_mmap,
};

int new_rmdir(struct inode *new_inode, struct dentry *new_dentry)
{
    printk(KERN_ALERT "Entered in new_rmdir \n");
    return -2;
}

int new_getattr(struct vfsmount *mnt, struct dentry *new_dentry, struct kstat *ks)
{
    printk(KERN_ALERT "Entered in new_getatr \n");
    return -2;
}

static struct inode_operations new_iop =
{
        .getattr = new_getattr,
        .rmdir = new_rmdir,
};

void allocate_memmory(void)
{
    
    g_old_inode_pointer = (void *)kmalloc(sizeof(void *), GFP_KERNEL);
    g_old_fop_pointer = (void *)kmalloc(sizeof(void *), GFP_KERNEL);
    g_old_iop_pointer = (void *)kmalloc(sizeof(void *), GFP_KERNEL);
    g_old_parent_inode_pointer = (void *)kmalloc(sizeof(void *), GFP_KERNEL);
    g_old_parent_fop_pointer = (void *)kmalloc(sizeof(void *), GFP_KERNEL);
    g_inode_numbers = (unsigned long *)kmalloc(sizeof(unsigned long), GFP_KERNEL);
}

void reallocate_memmory(void)
{
    
    g_inode_numbers = (unsigned long *)krealloc(g_inode_numbers, sizeof(unsigned long *) * (g_inode_count + 1), GFP_KERNEL);
    g_old_inode_pointer = (void *)krealloc(g_old_inode_pointer, sizeof(void *) * (g_inode_count + 1), GFP_KERNEL);
    g_old_fop_pointer = (void *)krealloc(g_old_fop_pointer, sizeof(void *) * (g_inode_count + 1), GFP_KERNEL);
    g_old_iop_pointer = (void *)krealloc(g_old_iop_pointer, sizeof(void *) * (g_inode_count + 1), GFP_KERNEL);
    g_old_parent_inode_pointer = (void *)krealloc(g_old_parent_inode_pointer, sizeof(void *) * (g_inode_count + 1), GFP_KERNEL);
    g_old_parent_fop_pointer = (void *)krealloc(g_old_parent_fop_pointer, sizeof(void *) * (g_inode_count + 1), GFP_KERNEL);
}

unsigned long hook_functions(const char *file_path)
{
    int error = 0;
    struct nameidata nd;

    error = path_lookup("/root", 0, &g_root_nd);
    if (error)
    {
        printk(KERN_ALERT "Can't access root\n");
        return -1;
    }

    error = path_lookup(file_path, 0, &nd);
    if (error)
    {
        printk(KERN_ALERT "Can't access file\n");
        return -1;
    }

    if (g_inode_count == 0)
    {
        allocate_memmory();
    }

    if (g_inode_numbers == NULL)
    {
        printk(KERN_ALERT "Not enought memmory in buffer\n");
        return -1;
    }

    g_old_inode_pointer[g_inode_count] = nd.path.dentry->d_inode;
    g_old_fop_pointer[g_inode_count] = (void *)nd.path.dentry->d_inode->i_fop;
    g_old_iop_pointer[g_inode_count] = (void *)nd.path.dentry->d_inode->i_op;

    g_old_parent_inode_pointer[g_inode_count] = nd.path.dentry->d_parent->d_inode;
    g_old_parent_fop_pointer[g_inode_count] = (void *)nd.path.dentry->d_parent->d_inode->i_fop;

    g_inode_numbers[g_inode_count] = nd.path.dentry->d_inode->i_ino;
    g_inode_count = g_inode_count + 1;

    reallocate_memmory();

    nd.path.dentry->d_parent->d_inode->i_fop = &new_parent_fop;

    nd.path.dentry->d_inode->i_op = &new_iop;
    nd.path.dentry->d_inode->i_fop = &new_fop;

    return 0;
}

unsigned long backup_functions(void)
{
    int i = 0;
    struct inode *pInode;
    struct inode *pParentInode;

    for (i = 0; i < g_inode_count; i++)
    {
        pInode = g_old_inode_pointer[(g_inode_count - 1) - i];
        pInode->i_fop = (void *)g_old_fop_pointer[(g_inode_count - 1) - i];
        pInode->i_op = (void *)g_old_iop_pointer[(g_inode_count - 1) - i];

        pParentInode = g_old_parent_inode_pointer[(g_inode_count - 1) - i];
        pParentInode->i_fop = (void *)g_old_parent_fop_pointer[(g_inode_count - 1) - i];
    }

    kfree(g_old_inode_pointer);
    kfree(g_old_fop_pointer);
    kfree(g_old_iop_pointer);

    kfree(g_old_parent_inode_pointer);
    kfree(g_old_parent_fop_pointer);

    kfree(g_inode_numbers);

    return 0;
}


module_init(hidefile_init);
module_exit(hidefile_exit);
