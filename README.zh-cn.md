# AFLNW: AFL network wrapper

AFL当前的测试目标大多数是文件处理类程序，在测试网络程序的时候，存在多个许多需要解决的问题。

本工具提供了一种方案，可以几乎无修改的测试网络服务程序。

## 目录

- [方案背景](#%E6%96%B9%E6%A1%88%E8%83%8C%E6%99%AF)
- [解决方案](#%E8%A7%A3%E5%86%B3%E6%96%B9%E6%A1%88)
- [工具安装](#%E5%B7%A5%E5%85%B7%E5%AE%89%E8%A3%85)
- [使用说明](#%E4%BD%BF%E7%94%A8%E8%AF%B4%E6%98%8E)
- [测试样例](#%E6%B5%8B%E8%AF%95%E6%A0%B7%E4%BE%8B)
- [注意事项](#%E6%B3%A8%E6%84%8F%E4%BA%8B%E9%A1%B9)
- [参考链接](#%E5%8F%82%E8%80%83%E9%93%BE%E6%8E%A5)

## 方案背景

对于网络服务程序的模糊测试，当前的解决方案主要有：hook libc socket调用、修改afl、修改网络程序等，其中无论哪种方式，几乎都需要修改网络服务程序。

AFLplusplus的开发者在仓库中推荐使用[hook socket](https://github.com/AFLplusplus/AFLplusplus/tree/stable/utils/socket_fuzzing) 的方案，这种方案灵感来源于[preeny](https://github.com/zardus/preeny)。基于 ``LD_PRELOAD`` hook libc中socket相关的函数将网络流转换为标准输入的IO，但是这种方案在一些复杂的网络程序中并不一定通用。

以nginx为例，在nginx启动后，程序会持续运行并监听对应的网络端口，如果没有异常并不会主动退出。而AFL需要每次重新启动程序，在这种场景下进行测试时就需要修改nginx的源代码。

另一个较为自然的方案是直接修改AFL传递输入的方式。这种方式中比较有代表性的是Monash University的研究者提出的[aflnet](https://github.com/aflnet/aflnet)。aflnet在AFL的基础上，将标准输入修改为网络发包的方式，并加入了网络传输的功能，可以较为高效的测试网络服务程序。

但是afl的分支众多，有着不同的优劣势，也有着不同的应用场景。基于修改AFL的方案在移植其它优化策略时需要较大的工作量，另外aflnet同样需要对待测程序进行一定的修改以适应测试。

效率最高的方案是直接修改网络程序，调用对应的解析函数来进行测试，以bind9为例，其代码中就专门提供了用于Fuzz的[部分](https://github.com/isc-projects/bind9/tree/main/fuzz)。这种方式直接获取标准输入，传入核心函数中进行测试。这种方式的缺陷在于需要较为了解程序，且需要对目标程序进行定制开发。

## 解决方案

那么能不能找到一种相对简单的方案，能够在不对AFL或者目标程序进行修改的基础上，较为简单的测试网络服务程序呢？

考虑到AFL读取覆盖率是通过共享内存的方式，一个解决思路是，并不直接通过AFL启动程序，而是AFL启动辅助程序，AFL将标准输入传输辅助程序，辅助程序和网络程序进行交互。

具体来说，AFL启动辅助程序，辅助程序检查网络程序是否启动，若未启动，则启动待测的网络服务程序。此时 ``__AFL_SHM_ID`` 环境变量将传输待测网络服务程序中，基于AFL插桩的网络服务程序在测试时同样会记录覆盖率信息到当前的共享内存中。

每次进行新的测试时，辅助程序重复读取输入、将输入通过网络发送至目标网络服务程序的流程。而网络服务程序则不需要在启动流程中浪费运行时间，达到类似 ``persistent mode`` 的效果。

另外当网络服务程序失去响应时，辅助程序主动crash，使得AFL记录对应的crash输入。

## 工具安装

```bash
git clone --depth=1 https://github.com/LyleMi/aflnw.git
cd aflnw
export CC=/path/to/afl/afl-clang-fast
mkdir build && cd build && cmake .. && make
```

## 使用说明

主要使用以下参数：

- ``-a addr`` 网络监听地址，建议设置为 ``127.0.0.1``
- ``-p port`` 网络监听端口
- ``-- <remains>`` 待测程序启动参数

## 测试样例

以nginx为例，测试的流程如下：

首先安装nginx:

```bash
git clone --depth=1 https://github.com/nginx/nginx.git
cd nginx
mkdir `pwd`/logs
export CC=/path/to/afl/afl-clang-fast
./auto/configure --prefix=`pwd` --with-select_module
make -j `nproc`
```

为了方便测试，修改 ``nginx/conf/nginx.conf`` 中部分配置为：

```
master_process off;
daemon on;
# 关闭日志
error_log /dev/null;

events {
        worker_connections  1024;
        multi_accept off;
}
server {
    # 监听在非 80 端口防止冲突等情况
    listen 8980;
    access_log off;
}
```

最后启动程序，开始测试

```bash
/path/to/afl/afl-fuzz -i ./input -o ./aflout -- ./aflnw -a 127.0.0.1 -p 8980 -- /data/targets/nginx/objs/nginx
```

## 注意事项

该项目仅供验证想法，还有许多可能未处理好的问题，如果在测试过程中遇到问题，请以Issue或者Pr的形式提出，万分感谢！

## 参考链接

- [AFLplusplus](https://github.com/AFLplusplus/AFLplusplus)
- [preeny](https://github.com/zardus/preeny)
- [aflnet](https://github.com/aflnet/aflnet)
