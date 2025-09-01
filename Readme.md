gcc -Wall -Wextra -std=c11 -O2 -c -Isrc -Iinclude -o build/cpu_branch.o src/cpu_branch.c
gcc -Wall -Wextra -std=c11 -O2 -c -Isrc -Iinclude -o build/disk.o src/disk.c
gcc -Wall -Wextra -std=c11 -O2 -c -Isrc -Iinclude -o build/wincrt.o src/wincrt.c
gcc -Wall -Wextra -std=c11 -O2 -c -Isrc -Iinclude -o build/log.o src/log.c
gcc -Wall -Wextra -std=c11 -O2 -c -Isrc -Iinclude -o build/cpu.o src/cpu.c


Yes Windows has autoconf:

C:\cygwin64\setup-x86_64.exe -q -P autoconf

cygcheck -c autoconf

But you should probably plan on installing the full suit:
setup-x86_64.exe -q -P autoconf,automake,libtool,m4,make

mintty.exe -

You might need to run CYGWIN64 bash environment in order for the path variables to line up right so that "autoconf" can be run

You can use:  ls -l /usr/bin/autoconf to verify the file is there

cd /cygdrive/o
