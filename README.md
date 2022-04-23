# Mock Kernel Driver
Simple example project of a kernel driver doing basic operations. A server handling the kernel driver and communicating the data to the client via unix domain sockets. Built on 5.4.189-1.el7.elrepo.x86_64 (CentOS 7), gcc 9.3.1.

## Build

Should be easy enough to build if everything is working.

```bash
mkdir build
cd build
cmake3 ..
make
```

## Usage

Open two terminals in the build directory. 

On terminal 1:
```bash
.\load_driver #expect to output nothing (check dmesg for loaded driver)
.\server
```

On terminal 2 connect the client:
```
.\client #follow instructions from the server
```

