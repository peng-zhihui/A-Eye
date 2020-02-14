cd "/home/imliubo/Kendryte/KendryteIDE/LocalPackage/cmake/bin"
export PATH=/home/imliubo/Kendryte/KendryteIDE/bin:/home/imliubo/Kendryte/KendryteIDE/LocalPackage/toolchain/bin:/home/imliubo/Kendryte/KendryteIDE/LocalPackage/cmake/bin:/home/imliubo/Kendryte/KendryteIDE/LocalPackage/jlink:/home/imliubo/Kendryte/KendryteIDE/LocalPackage/clang-format/bin:/bin:/usr/bin:/usr/local/bin:/home/imliubo/.bin:/home/imliubo/.local/bin
export PYTHONPATH=:/usr/lib/python2.7/:/usr/lib/python2/:/lib/python2.7/:/lib/python2/:/home/imliubo/Kendryte/KendryteIDE/LocalPackage/toolchain/share/gdb/python
export PYTHONDONTWRITEBYTECODE=yes
export CMAKE_MAKE_PROGRAM=make
export KENDRYTE_IDE=yes
export LANG=en_US.utf-8
"/home/imliubo/Kendryte/KendryteIDE/LocalPackage/cmake/bin/cmake" -E server --experimental --pipe=/dev/shm/kide-sock/cmake_server_pipe.1581153099061.sock