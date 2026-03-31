#include <linux/sched.h>
#include <linux/tty.h>
#include <linux/io.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/version.h>
#if(LINUX_VERSION_CODE >= KERNEL_VERSION(4,14,83))
#include <linux/sched/mm.h>
#endif
#include <asm/cpu.h>
#include <asm/io.h>
#include <asm/page.h>
#include <asm/pgtable.h>
#include "process.h"
#undef pgd_offset
#if LINUX_VERSION_CODE <= KERNEL_VERSION(3,10,84)
//来源：#define pgd_offset(mm, addr)	((mm)->pgd+pgd_index(addr))
#define my_pgd_offset(pgd, addr)	(pgd+pgd_index(addr))
#define my_pud_offset(dir, addr) ((pud_t *)__va(pud_offset_phys((dir), (addr))))
#endif
#if LINUX_VERSION_CODE < KERNEL_VERSION(5,10,43)
//来源：#define pgd_offset(mm, addr)	((mm)->pgd+pgd_index(addr))
#define my_pgd_offset(pgd, addr)	(pgd+pgd_index(addr))
#define my_pud_offset(dir, addr) ((pud_t *)__va(pud_offset_phys((dir), (addr))))
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,10,43)
//来源：#define pgd_offset(mm, address)	pgd_offset_pgd((mm)->pgd, (address))
#define my_pgd_offset(pgd, address)	pgd_offset_pgd(pgd, address)
#endif
static ssize_t g_pgd_offset_mm_struct = 0;
static bool g_init_pgd_offset_success = false;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,1,75)

static int init_pgd_offset(struct mm_struct *mm) {
	int is_find_pgd_offset = 0;
	g_init_pgd_offset_success = false;
	for (g_pgd_offset_mm_struct = -40; g_pgd_offset_mm_struct <= 80; g_pgd_offset_mm_struct += 1) {
		char *rp;
		size_t val;
		ssize_t accurate_offset = (ssize_t)((size_t)&mm->pgd - (size_t)mm + g_pgd_offset_mm_struct);
		if (accurate_offset >= sizeof(struct mm_struct) - sizeof(ssize_t)) {
			return -EFAULT;
		}
		rp = (char*)((size_t)mm + (size_t)accurate_offset);
		val = *(size_t*)(rp);
		//printk_debug(KERN_EMERG "init_pgd_offset %zd:%zd:%p:%ld\n", g_pgd_offset_mm_struct, accurate_offset, rp, val);

		if (val == TASK_SIZE) {
			g_pgd_offset_mm_struct += sizeof(unsigned long);
			//printk_debug(KERN_EMERG "found g_init_pgd_offset_success:%zd\n", g_pgd_offset_mm_struct);
			is_find_pgd_offset = 1;
			break;
		}
	}
	if (!is_find_pgd_offset) {
		//printk_debug(KERN_INFO "find pgd offset failed\n");
		return -ESPIPE;
	}
	g_init_pgd_offset_success = true;
	//printk_debug(KERN_INFO "g_pgd_offset_mm_struct:%zu\n", g_pgd_offset_mm_struct);
	return 0;
}
#else
static int init_pgd_offset(struct mm_struct *mm) {
	int is_find_pgd_offset = 0;
	g_init_pgd_offset_success = false;
	for (g_pgd_offset_mm_struct = -40; g_pgd_offset_mm_struct <= 80; g_pgd_offset_mm_struct += 1) {
		char *rp;
		size_t val;
		ssize_t accurate_offset = (ssize_t)((size_t)&mm->pgd - (size_t)mm + g_pgd_offset_mm_struct);
		if (accurate_offset >= sizeof(struct mm_struct) - sizeof(ssize_t)) {
			return -EFAULT;
		}
		rp = (char*)((size_t)mm + (size_t)accurate_offset);
		val = *(size_t*)(rp);
		//printk_debug(KERN_EMERG "init_pgd_offset %zd:%zd:%p:%ld\n", g_pgd_offset_mm_struct, accurate_offset, rp, val);

		if (val == TASK_SIZE) {
			g_pgd_offset_mm_struct += sizeof(unsigned long);
			g_pgd_offset_mm_struct += sizeof(unsigned long);
			printk_debug(KERN_EMERG "found g_init_pgd_offset_success:%zd\n", g_pgd_offset_mm_struct);
			is_find_pgd_offset = 1;
			break;
		}
	}
	if (!is_find_pgd_offset) {
		//printk_debug(KERN_INFO "find pgd offset failed\n");
		return -ESPIPE;
	}
	g_init_pgd_offset_success = true;
	//printk_debug(KERN_INFO "g_pgd_offset_mm_struct:%zu\n", g_pgd_offset_mm_struct);
	return 0;
}
#endif

static inline pgd_t *x_pgd_offset(struct mm_struct *mm, size_t addr) {
	size_t pgd;
	ssize_t accurate_offset;
	if (g_init_pgd_offset_success == false) {
		if (init_pgd_offset(mm) != 0) {
			return NULL;
		}
	}
	//精确偏移
	accurate_offset = (ssize_t)((size_t)&mm->pgd - (size_t)mm + g_pgd_offset_mm_struct);
	//printk_debug(KERN_INFO "x_pgd_offset accurate_offset:%zd\n", accurate_offset);
	if (accurate_offset >= sizeof(struct mm_struct) - sizeof(ssize_t)) {
		return NULL;
	}

	//拷贝到我自己的pgd指针变量里去
	//写法一（可读性强）
	//void * rv = (size_t*)((size_t)mm + (size_t)accurate_offset);
	//pgd_t *pgd;
	//memcpy(&pgd, rv, sizeof(pgd_t *));

	//写法二（快些）
	pgd = *(size_t*)((size_t)mm + (size_t)accurate_offset);

	return my_pgd_offset((pgd_t*)pgd, addr);
}

//走页表获取物理地址
phys_addr_t translate_linear_address(struct mm_struct* mm, uintptr_t va) {

    pgd_t *pgd;
    p4d_t *p4d;
    pmd_t *pmd;
    pte_t *pte;
    pud_t *pud;
	
        unsigned long paddr = 0;
	unsigned long page_addr = 0;
	unsigned long page_offset = 0;
    
    pgd = x_pgd_offset(mm, va);
    if(pgd_none(*pgd)) {
        return 0;
    }
    p4d = p4d_offset(pgd, va);
    if (p4d_none(*p4d)) {
    	return 0;
    }
	pud = pud_offset(p4d,va);
	if(pud_none(*pud)) {
        return 0;
    }
	pmd = pmd_offset(pud,va);
	if(pmd_none(*pmd)) {
        return 0;
    }
	pte = pte_offset_kernel(pmd,va);
	if(pte_none(*pte)) {
        return 0;
    }
	if(!pte_present(*pte)) {
        return 0;
    }
	//页物理地址
	page_addr = page_to_phys(pte_page(*pte));

	page_offset = va & ~PAGE_MASK;
	paddr = page_addr | page_offset;
	return paddr;
}

#ifdef ARCH_HAS_VALID_PHYS_ADDR_RANGE
static size_t g_phy_total_memory_size = 0;
//初始化物理内存大小
static int init_phy_total_memory_size(void) {
    struct sysinfo si;
    si_meminfo(&si);
    g_phy_total_memory_size = __pa(si.totalram * si.mem_unit);
    return 0;
}
//检查物理地址范围
static inline int check_phys_addr_range(phys_addr_t addr, size_t count) {
    if (g_phy_total_memory_size == 0)
        init_phy_total_memory_size();
    return (addr + count) <= g_phy_total_memory_size;
}
#else
static inline int check_phys_addr_range(phys_addr_t addr, size_t count) {
    return 1;
}
#endif
//读写物理地址
static size_t access_physical_address(phys_addr_t pa, void* buffer, size_t size, bool is_write) {
    void __iomem *mapped = NULL;
    size_t ret = 0;

    if (!check_phys_addr_range(pa, size)) // 新增范围检查
        return 0;

    if (!pfn_valid(__phys_to_pfn(pa)))
        return 0;

    mapped = ioremap_cache(pa, size);
    if (!mapped)
        return 0;

    if (is_write)
        ret = x_copy_from_user(mapped, buffer, size) ? 0 : size;
    else
        ret = x_copy_to_user(buffer, mapped, size) ? 0 : size;

    iounmap(mapped);
    return ret;
}
/*对外接口实现 读写进程内存*/
bool read_process_memory(pid_t pid, uintptr_t addr, void *buffer, size_t size)
{
    struct task_struct *task = NULL;
    struct mm_struct *mm = NULL;
    size_t remaining = size;
    size_t chunk, copied = 0;
    phys_addr_t pa;
    char __user *buf = buffer;
    bool ret = false;

    /* 1. 获取任务结构 */
    rcu_read_lock();
    task = pid_task(find_vpid(pid), PIDTYPE_PID);
    if (!task) {
       // pr_debug("Task not found for pid %d\n", pid);
        rcu_read_unlock();
        return false;
    }
    rcu_read_unlock();

    /* 2. 获取内存管理结构 */
    mm = get_task_mm(task);
    if (!mm) {
        //pr_debug("Failed to get mm_struct for pid %d\n", pid);
        goto out;
    }

    /* 3. 分页读取内存 */
    while (remaining > 0) {
        chunk = min_t(size_t, PAGE_SIZE - (addr & ~PAGE_MASK), remaining);
        pa = translate_linear_address(mm, addr);
        
        if (!pa) {
           // pr_debug("Failed to translate VA 0x%lx in pid %d\n", addr, pid);
            goto out_mm;
        }

        if (!access_physical_address(pa, buf + copied, chunk, false)) {
            //pr_debug("Failed to read %zu bytes at PA 0x%llx\n", chunk, (u64)pa);
            goto out_mm;
        }

        copied += chunk;
        addr += chunk;
        remaining -= chunk;
    }

    ret = (copied == size);

out_mm:
    mmput(mm);
out:
    return ret;
}

bool write_process_memory(pid_t pid, uintptr_t addr, void *buffer, size_t size)
{
    struct task_struct *task = NULL;
    struct mm_struct *mm = NULL;
    size_t remaining = size;
    size_t chunk, written = 0;
    phys_addr_t pa;
    const char __user *buf = buffer;
    bool ret = false;

    /* 1. 获取任务结构 */
    rcu_read_lock();
    task = pid_task(find_vpid(pid), PIDTYPE_PID);
    if (!task) {
        //pr_debug("Task not found for pid %d\n", pid);
        rcu_read_unlock();
        return false;
    }
    rcu_read_unlock();

    /* 2. 获取内存管理结构 */
    mm = get_task_mm(task);
    if (!mm) {
        //pr_debug("Failed to get mm_struct for pid %d\n", pid);
        goto out;
    }

    /* 3. 分页写入内存 */
    while (remaining > 0) {
        chunk = min_t(size_t, PAGE_SIZE - (addr & ~PAGE_MASK), remaining);
        pa = translate_linear_address(mm, addr);
        
        if (!pa) {
            //pr_debug("Failed to translate VA 0x%lx in pid %d\n", addr, pid);
            goto out_mm;
        }

        if (!access_physical_address(pa, (void *)(buf + written), chunk, true)) {
           // pr_debug("Failed to write %zu bytes at PA 0x%llx\n", chunk, (u64)pa);
            goto out_mm;
        }

        written += chunk;
        addr += chunk;
        remaining -= chunk;
    }

    ret = (written == size);

out_mm:
    mmput(mm);
out:
    return ret;
}
