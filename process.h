#ifndef PROCESS_H
#define PROCESS_H
#include <linux/sched.h>
#include <linux/module.h>
#include <linux/tty.h>
#include <linux/mm.h>
#include <linux/version.h>
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4,14,83))
#include <linux/sched/mm.h>
#include <linux/sched/task.h>
#include <linux/sched/signal.h>
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 8, 0))
#include <linux/mmap_lock.h>
#define MM_READ_LOCK(mm) mmap_read_lock(mm);
#define MM_READ_UNLOCK(mm) mmap_read_unlock(mm);
#else
#include <linux/rwsem.h>
#define MM_READ_LOCK(mm) down_read(&(mm)->mmap_sem);
#define MM_READ_UNLOCK(mm) up_read(&(mm)->mmap_sem);
#endif
#define CONFIG_DIRECT_API_USER_COPY
static inline unsigned long x_copy_from_user(void *to, const void __user *from, unsigned long n);
static inline unsigned long x_copy_to_user(void __user *to, const void *from, unsigned long n);
static inline uintptr_t get_module_base(pid_t pid, char* name);

static inline unsigned long x_copy_from_user(void *to, const void __user *from, unsigned long n) {
#ifdef CONFIG_DIRECT_API_USER_COPY
	unsigned long __arch_copy_from_user(void* to, const void __user * from, unsigned long n);
	return __arch_copy_from_user(to, from, n);
#else
	return copy_from_user(to, from, n);
#endif
}
static inline unsigned long x_copy_to_user(void __user *to, const void *from, unsigned long n) {
#ifdef CONFIG_DIRECT_API_USER_COPY
	unsigned long __arch_copy_to_user(void __user * to, const void* from, unsigned long n);
	return __arch_copy_to_user(to, from, n);
#else
	return copy_to_user(to, from, n);
#endif
}
#define ARC_PATH_MAX 256


//获取模块基址
uintptr_t get_module_base(pid_t pid, char* name) {
    struct pid* pid_struct;
    struct task_struct* task;
    struct mm_struct* mm;
    struct vm_area_struct* vma;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0))
    struct vma_iterator vmi;
#endif
    uintptr_t result;
    struct dentry* dentry;
    size_t name_len, dname_len;

    result = 0;

    name_len = strlen(name);
    if (name_len == 0) {
        //dream_debug("module name is empty\n");
        return 0;
    }

    pid_struct = find_get_pid(pid);
    if (!pid_struct) {
        //dream_debug("failed to find pid_struct\n");
        return 0;
    }

    task = get_pid_task(pid_struct, PIDTYPE_PID);
    put_pid(pid_struct);
    if (!task) {
        //dream_debug("failed to get task from pid_struct\n");
        return 0;
    }

    mm = get_task_mm(task);
    put_task_struct(task);
    if (!mm) {
        //dream_debug("failed to get mm from task\n");
        return 0;
    }

    MM_READ_LOCK(mm)

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0))
    vma_iter_init(&vmi, mm, 0);
    for_each_vma(vmi, vma)
#else
    for (vma = mm->mmap; vma; vma = vma->vm_next)
#endif
    {
        if (vma->vm_file) {
            dentry = vma->vm_file->f_path.dentry;
            dname_len = dentry->d_name.len;
            if (!memcmp(dentry->d_name.name, name, min(name_len, dname_len))) {
                result = vma->vm_start;
                goto ret;
            }
        }
    }

    ret:
        MM_READ_UNLOCK(mm)

        mmput(mm);
    return result;
}



#endif

