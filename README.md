# Linux Arm64 Kernel Hack
## 项目介绍
本项目是一个基于Linux Arm64内核的驱动项目，用于学习和实验。
## 项目结构
1. UMB.c 驱动主文件 使用kretprobe 挂钩random_ioctl 实现通信 隐藏模块实现
2. memory.h 读内存实现 简单的ioremap映射 加手动走页实现获取物理地址
3. process.h 获取模块地址
4. common.h 通信结构体
## 驱动接口
1. 检测驱动是否加载 OP_DRIVER_PING 0x400010
2. 读内存 OP_READ_MEM 0x400011
3. 写内存 OP_WRITE_MEM 0x400012 
4. 获取模块基址 OP_MODULE_BASE 0x400013

## 关于
### 本项目是本人初学时的项目，用于学习和实验Linux Arm64内核的驱动开发。
### 可能有点简陋 但基本功能已经实现
### 只支持5.10及以上版本内核
# 编译教程
## 使用ddk进行编译

### 安装 DDK

```bash
sudo curl -fsSL https://raw.githubusercontent.com/Ylarod/ddk/main/scripts/ddk -o /usr/local/bin/ddk
sudo chmod +x /usr/local/bin/ddk
```

### 用ddk构建
```bash
sudo ddk pull andriod12-5.10
sudo ddk build andriod12-5.10
```

### 用户层通信方法
1. 刷入驱动
2. open /dev/random 成功后即可通信
