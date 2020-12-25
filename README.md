# AFLNW: AFL network wrapper

The current fuzz targets of AFL are mainly file processing programs. When fuzzing the network program, there are many problems to be solved.

This tool provides a solution to fuzz the network service program almost without modification.

[中文版本(Chinese version)](https://github.com/LyleMi/aflnw/blob/main/README.zh-cn.md)

## Contents

- [Background](#Background)
- [Approach](#Approach)
- [Installation](#Installation)
- [Usage](#Usage)
- [Example](#Example)
- [Issues](#Issues)
- [Reference](#Reference)

## Background

For network service fuzzing, the current solutions mainly include: hook libc socket call, modify AFL, modify network programs, etc.

Among them, the developers of AFLplusplus recommend using [hook sockets](https://github.com/AFLplusplus/AFLplusplus/tree/stable/utils/socket_fuzzing) in their repo. The idea is inspired by [preeny](https://github.com/zardus/preeny). This solution is based on hooking the socket-related functions in libc through ``LD_PRELOAD`` , but this solution is not necessarily universal in some complex network programs.

Take nginx as an example. After nginx is started, the program will continue to run and monitor the corresponding network port. If there is no exception, it will not exit anyway. However, AFL needs to restart the program every time. If you are fuzzing nginx in this scenario, you need to modify the source code of nginx.

Another solution is to directly modify the way AFL transmits input. The most representative of this method is [AFLnet](https://github.com/aflnet/aflnet) proposed by researchers at Monash University. Based on AFL, AFLnet modifies the standard input to network packet and adds the function of network transmission, which can fuzz network service programs more efficiently.

However, there are many branches of AFL, with different advantages and disadvantages, and different application scenarios. When migrating other optimization strategies, the scheme based on modifying AFL requires a large amount of work. Besides, AFLnet also requires certain modifications to the target program to adapt to the fuzz.

The most efficient solution is to directly modify the network program and call the corresponding analytic function for fuzzing. Take bind9 as an example, its code specifically provides a [part](https://github.com/isc-projects/bind9/tree/main/fuzz) for fuzz. In this way, standard input is directly obtained and passed into the function for testing. The disadvantage of this approach is that it requires a better understanding of the program and the need for customized development of the target program.

## Approach

Can we find a relatively simple solution that can fuzz the network service without modifying the AFL or target program?

The idea of this tool is that instead of starting the program directly through AFL, AFL starts the auxiliary program. AFL transfers the standard input to the auxiliary program, and the auxiliary program interacts with the network program.

Specifically, AFL starts the auxiliary program, and the auxiliary program checks whether the network program is started, if not, it starts the network service program to be fuzzed. At this time, the ``__AFL_SHM_ID`` environment variable will transmit the network service to be fuzzed, and the network service based on AFL instrumentation will also record the coverage information to the current shared memory during the fuzz.

Each time a new test is performed, the auxiliary program repeats the process of reading the input and sending the input to the target network service through the network. The network service program does not need to waste running time in the startup process, achieving an effect similar to ``persistent mode``.

In addition, when the network service loses response, the auxiliary program actively crashes, so that AFL records the corresponding crash input.

## Installation

```bash
git clone --depth=1 https://github.com/LyleMi/aflnw.git
cd aflnw
export CC=/path/to/afl/afl-clang-fast
mkdir build && cd build && cmake .. && make
```

## Usage

- ``-a addr`` network listening address, it is recommended to set to ``127.0.0.1``
- ``-p port`` network listening port
- ``-- <remains>`` parameters of the program to be fuzzed

## Example

Taking nginx as an example, the fuzz process is as follows:

First install nginx:

```bash
git clone --depth=1 https://github.com/nginx/nginx.git
cd nginx
mkdir `pwd`/logs
export CC=/path/to/afl/afl-clang-fast
./auto/configure --prefix=`pwd` --with-select_module
make -j `nproc`
```

In order to facilitate the fuzzing, modify the part of the configuration in ``nginx/conf/nginx.conf`` to:


```
master_process off;
daemon on;
# close log
error_log /dev/null;

events {
        worker_connections  1024;
        multi_accept off;
}
server {
    # monitor non-80 ports to prevent conflicts and other situations
    listen 8980;
    access_log off;
}
```

Finally, start the program and begin the test

```bash
/path/to/afl/afl-fuzz -i ./input -o ./aflout -- ./aflnw -a 127.0.0.1 -p 8980 -- /data/targets/nginx/objs/nginx
```

## Issues

This project is only for verifying ideas, and many problems may not be handled well. If you encounter problems during the test, please raise them in the form of [Issue](https://github.com/lylemi/aflnw/issues) or [PR](https://github.com/lylemi/aflnw/pulls). Thank you very much!

## Reference

- [AFLplusplus](https://github.com/AFLplusplus/AFLplusplus)
- [preeny](https://github.com/zardus/preeny)
- [aflnet](https://github.com/aflnet/aflnet)
