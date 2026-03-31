#ifndef KERNEL_H
#define KERNEL_H
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <iostream>
#include <sys/ioctl.h>
#include <string.h>

//操作码定义
#define OP_DRIVER_PING 0x400010
#define OP_READ_MEM 0x400011
#define OP_WRITE_MEM 0x400012
#define OP_MODULE_BASE 0x400013
//驱动类实现
class Kernel {
private:
    int fd;
    pid_t pid;

    typedef struct _COPY_MEMORY {
        pid_t pid;
        uintptr_t addr;
        void* buffer;
        size_t size;
    } COPY_MEMORY, *PCOPY_MEMORY;
    typedef struct _MODULE_BASE {
        pid_t pid;
        char* name;
        uintptr_t base;
    } MODULE_BASE, *PMODULE_BASE;

bool check_driver_load(int fd) {
    if (ioctl(fd,OP_DRIVER_PING,0) !=1) {
        return false;
    }
    return true;
}

public:
    //构造函数 打开random
    Kernel() {
        fd =  open("/dev/random", O_WRONLY);
        if (fd == -1) {
            std::cout << "无法打开/dev/random" << std::endl;
        }
        else {
            std::cout << "打开random成功"<<std::endl;
        }
        if (check_driver_load(fd)) {
            std::cout<<"驱动已加载"<<std::endl;
        }
        else {
            std::cout<<"驱动未载入 请刷入驱动"<<std::endl;
            close(fd);
            exit(1);
        }
    }
    //析构函数 关闭文件
    ~Kernel() {
        if (fd > 0)
            close(fd);
    }
    //初始化pid
    void initialize(pid_t pid) {
        this->pid = pid;
    }
    //获取进程pid
    pid_t get_name_pid(char* name) {
        FILE* fp;
        pid_t pid;
        char cmd[0x100] = "pidof ";

        strcat(cmd, name);
        fp = popen(cmd, "r");
        fscanf(fp, "%d", &pid);
        pclose(fp);
        return pid;
    }
    //读内存
    bool read(uintptr_t addr, void* buffer, size_t size) {
        COPY_MEMORY cm;

        cm.pid = this->pid;
        cm.addr = addr;
        cm.buffer = buffer;
        cm.size = size;

        if (ioctl(fd,OP_READ_MEM, &cm) != 1) {
            return false;
        }
        return true;
    }
    //写内存
    bool write(uintptr_t addr, void* buffer, size_t size) {
        COPY_MEMORY cm;

        cm.pid = this->pid;
        cm.addr = addr;
        cm.buffer = buffer;
        cm.size = size;

        if (ioctl(fd,OP_WRITE_MEM, &cm) != 1) {
            return false;
        }
        return true;
    }


    template <typename T>
    T read(uintptr_t addr) {
        T res;
        if (this->read(addr, &res, sizeof(T)))
            return res;
        return {};
    }

    template <typename T>
    bool write(uintptr_t addr, T value) {
        return this->write(addr, &value, sizeof(T));
    }

    //获取模块基质
    uintptr_t get_module_base(char* name) {
        MODULE_BASE mb;
        char buf[0x100];
        strcpy(buf, name);
        mb.pid = this->pid;
        mb.name = buf;
        if (ioctl(fd,OP_MODULE_BASE, &mb) != 1) {
            return 0;
        }
        return mb.base;
    }

};

static Kernel* kernel = new Kernel();


#endif
