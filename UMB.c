#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kprobes.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/mm.h>
#include <linux/proc_fs.h>
#include "comm.h"
#include "memory.h"
#include "process.h"
//static struct kprobe kp;
#define OP_DRIVER_PING 0x400010
#define OP_READ_MEM 0x400011
#define OP_WRITE_MEM 0x400012
#define OP_MODULE_BASE 0x400013

//获取内核符号地址
static unsigned long my_kallsyms_lookup_name(const char *symbol_name) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 7, 0)
    typedef unsigned long (*kallsyms_lookup_name_t)(const char *name);

    static kallsyms_lookup_name_t lookup_name = NULL;
    if (lookup_name == NULL) {
        struct kprobe kp = {
                .symbol_name = "kallsyms_lookup_name"
        };

        if(register_kprobe(&kp) < 0) {
            return 0;
        }

        // 高版本一些地址符号不再导出，需要通过kallsyms_lookup_name获取
        // 但是kallsyms_lookup_name也是一个不导出的内核符号，需要通过kprobe获取
        lookup_name = (kallsyms_lookup_name_t) kp.addr;
        unregister_kprobe(&kp);
    }
    return lookup_name(symbol_name);
#else
    return kallsyms_lookup_name(symbol_name);
#endif
}
//打开文件
 int my_flip_open(const char *filename, int flags, umode_t mode, struct file **f) {
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0))
    *f = filp_open(filename, flags, mode);
    return *f == NULL ? -2 : 0;
#else
    static struct file* (*reserve_flip_open)(const char *filename, int flags, umode_t mode) = NULL;

    if (reserve_flip_open == NULL) {
        reserve_flip_open = (struct file* (*)(const char *filename, int flags, umode_t mode))my_kallsyms_lookup_name("filp_open");
        if (reserve_flip_open == NULL) {
            return 0;
        }
    }

    *f = reserve_flip_open(filename, flags, mode);
    return *f == NULL ? -2 : 0;
#endif
}
//关闭文件
int my_flip_close(struct file **f, fl_owner_t id) {
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0))
    filp_close(*f, id);
    return 0;
#else
    static struct file* (*reserve_flip_close)(struct file **f, fl_owner_t id) = NULL;

    if (reserve_flip_close == NULL) {
        reserve_flip_close = (struct file* (*)(struct file **f, fl_owner_t id))my_kallsyms_lookup_name("filp_close");
        if (reserve_flip_close == NULL) {
            return 0;
        }
    }

    reserve_flip_close(f, id);
    return 0;
#endif
}
//检查文件是否存在
bool is_file_exist(const char *filename) {
    struct file* fp;

    if(my_flip_open(filename, O_RDONLY, 0, &fp) == 0) {
        if (!IS_ERR(fp)) {
            my_flip_close(&fp, NULL);
            return true;
        }
        return false;
    }
    return false;
}
//移除proc文件
void cuteBabyPleaseDontCry(void) {
    if (is_file_exist("/proc/sched_debug")) {
        remove_proc_entry("sched_debug", NULL);
    }

    if (is_file_exist("/proc/uevents_records")) {
        remove_proc_entry("uevents_records", NULL);
    }

}
static struct list_head *mod_list;
//隐藏模块
void hide_module(void)
{
   
        mod_list = THIS_MODULE->list.prev;
        list_del(&THIS_MODULE->list);
        kfree(THIS_MODULE->sect_attrs);
        THIS_MODULE->sect_attrs = NULL;
}
//用户层调用接口封装 更方便维护
ssize_t Get_Module_Base(void *arg) {
    MODULE_BASE mb;
    char name[0x100] = {0};
    if (x_copy_from_user(&mb, arg, sizeof(mb)) ||
        x_copy_from_user(name, (void __user *)mb.name, sizeof(name) - 1))
    {
        return -EACCES;
    }
    mb.base = get_module_base(mb.pid, name);
    if (x_copy_to_user(arg, &mb, sizeof(mb)))
    {
        return -EACCES;
    }
    return 1;
}
ssize_t Read_Mem(void *arg) {
    COPY_MEMORY cm;
    if (x_copy_from_user(&cm, arg, sizeof(cm)))
    {
        return -EACCES;
    }
    if (!read_process_memory(cm.pid, cm.addr, cm.buffer, cm.size))
    {
        return  -EACCES;
    }
    return 1;
}
ssize_t Write_Mem(void *arg) {
    COPY_MEMORY cm;
    if (x_copy_from_user(&cm, arg, sizeof(cm)))
    {
        return -EACCES;
    }
    if (!write_process_memory(cm.pid, cm.addr, cm.buffer, cm.size))
    {
        return  -EACCES;
    }
    return 1;
}
//处理ioctl调用 返回值1表示成功 否则为错误码
static long dispatch_ioctl(unsigned int cmd , unsigned long arg)
{
	ssize_t ret;
	switch (cmd) {
	case OP_DRIVER_PING: //解释一下这里 这里是直接返回1 因为驱动加载成功后 会返回1 用来判断驱动是否加载成功
		ret = 1;
		break;
	case OP_READ_MEM:
		ret = Read_Mem((void *)arg);
		break;
	case OP_WRITE_MEM:
		ret = Write_Mem((void *)arg);
		break;
	case OP_MODULE_BASE:
		ret = Get_Module_Base((void *)arg);
		break;
	default:
		ret = -EACCES;
		break;
	}
	return ret;
}
//处理random_ioctl 使用kretprobe 为什么不使用kprobe呢?因为kprobe只在入口探针 无法修改返回值 即使修改了寄存器
//函数也会在返回之前覆盖原本的返回值
struct my_data {
    unsigned int cmd;
    unsigned long arg;
};
static int handler_ioctl_pre(struct kretprobe_instance *ri, struct pt_regs *regs)
{
    struct my_data *data = (struct my_data *)ri->data;
    data->cmd = regs->regs[1];
    data->arg = regs->regs[2];
	int ret =0;
    //
    if (data->cmd  >= OP_DRIVER_PING && data->cmd  <= OP_MODULE_BASE)
    {
    	//printk("inet_ioctl: %d, %lx\n", cmd, arg);
        ret = dispatch_ioctl(data->cmd ,data->arg);
		regs->regs[0] = ret; //修改返回值为ret
		//printk("random_ioctl: %d %d\n", ret,(int)regs->regs[0]);
		return 0;
    }
    return 0;
}

static struct kretprobe  krp = {
    .kp.symbol_name = "random_ioctl",
    .handler  = handler_ioctl_pre,
    .data_size = sizeof(struct my_data),

};

static int __init my_module_init(void) {
    hide_module();
    cuteBabyPleaseDontCry();
    if (register_kretprobe(&krp) < 0) {
        printk(KERN_ERR "Failed to register kprobe.\n");
        return 0;
    }

    //printk(KERN_INFO "Custom syscall module loaded.\n");

    return 0;
}

static void __exit my_module_exit(void) {
    unregister_kretprobe(&krp);
    //printk(KERN_INFO "Custom syscall module unloaded.\n");
}

module_init(my_module_init);
module_exit(my_module_exit);
MODULE_LICENSE("GPL");
