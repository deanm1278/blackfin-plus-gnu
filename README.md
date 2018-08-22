# blackfin-plus-gnu
gnu toolchain for blackfin plus

This is a fork of the now unsupported [ADI GNU Toolchain for Blackfin](https://sourceforge.net/projects/adi-toolchain/)
I have updated it to support the newer BF70x Blackfin+ parts

Note that not all Blackfin+ features are supported yet

## Building

after cloning the repo
```
$ cd blackfin-plus-gnu/gcc
$ ./contrib/download_prerequisites
$ cd ../buildscript/
$ ./build.sh
```

I have built this on cygwin64 on Windows 10, WSL Bash on windows 10, and on Ubuntu.
