#include <cstdio>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <memory>
#include <dirent.h>
#include <thread>
#include <sstream>
#include <cinttypes>
#include <chrono>
#include "kernel.h"

void benchmark_read(int iterations) {
    pid_t pid= kernel->get_name_pid("com.tencent.tmgp.pubgmhd");
    std::cout << pid << std::endl;
    kernel->initialize(pid);
    auto libbase = kernel->get_module_base("libUE4.so");
    printf("libbase %lx :\n", libbase);
    float value;

    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; ++i) {
       value = kernel->read<float>(libbase);

    }
    auto end = std::chrono::high_resolution_clock::now();

    auto duration_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    double total_us = duration_ns.count() / 1000.0;
    double avg_us = total_us / iterations;
    printf("读取浮点数 %f :\n", value);
    printf("读取 %d  万次:\n", iterations / 10000);
    printf("  总耗时: %.3f 微秒\n", total_us);
    printf("  平均每次读取: %.3f 微秒\n", avg_us);
    printf("  平均每次读取: %.3f 纳秒\n", avg_us * 1000.0);
    printf("\n");
}

int main(int argc, char* argv[]) {
    benchmark_read(100000);
    // benchmark_read(1000000);
    // benchmark_read(10000000);
    // benchmark_read(100000000);

    return 0;
}