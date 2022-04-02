geekos/idt.o: ../src/geekos/idt.c /usr/include/stdc-predef.h \
 ../include/geekos/kassert.h ../include/geekos/screen.h \
 ../include/geekos/ktypes.h \
 /usr/lib/gcc/x86_64-linux-gnu/5/include/stdbool.h \
 ../include/geekos/defs.h ../include/geekos/idt.h ../include/geekos/int.h
geekos/int.o: ../src/geekos/int.c /usr/include/stdc-predef.h \
 ../include/geekos/idt.h ../include/geekos/int.h \
 ../include/geekos/kassert.h ../include/geekos/screen.h \
 ../include/geekos/ktypes.h \
 /usr/lib/gcc/x86_64-linux-gnu/5/include/stdbool.h \
 ../include/geekos/defs.h ../include/geekos/paging.h \
 ../include/geekos/bootinfo.h ../include/geekos/list.h
geekos/trap.o: ../src/geekos/trap.c /usr/include/stdc-predef.h \
 ../include/geekos/idt.h ../include/geekos/int.h \
 ../include/geekos/kassert.h ../include/geekos/screen.h \
 ../include/geekos/ktypes.h \
 /usr/lib/gcc/x86_64-linux-gnu/5/include/stdbool.h \
 ../include/geekos/defs.h ../include/geekos/kthread.h \
 ../include/geekos/list.h ../include/geekos/syscall.h \
 ../include/geekos/trap.h
geekos/irq.o: ../src/geekos/irq.c /usr/include/stdc-predef.h \
 ../include/geekos/kassert.h ../include/geekos/screen.h \
 ../include/geekos/ktypes.h \
 /usr/lib/gcc/x86_64-linux-gnu/5/include/stdbool.h \
 ../include/geekos/idt.h ../include/geekos/int.h ../include/geekos/defs.h \
 ../include/geekos/io.h ../include/geekos/irq.h
geekos/io.o: ../src/geekos/io.c /usr/include/stdc-predef.h \
 ../include/geekos/io.h ../include/geekos/ktypes.h \
 /usr/lib/gcc/x86_64-linux-gnu/5/include/stdbool.h
geekos/keyboard.o: ../src/geekos/keyboard.c /usr/include/stdc-predef.h \
 ../include/geekos/kthread.h ../include/geekos/ktypes.h \
 /usr/lib/gcc/x86_64-linux-gnu/5/include/stdbool.h \
 ../include/geekos/list.h ../include/geekos/kassert.h \
 ../include/geekos/screen.h ../include/geekos/irq.h \
 ../include/geekos/int.h ../include/geekos/defs.h ../include/geekos/io.h \
 ../include/geekos/keyboard.h
geekos/screen.o: ../src/geekos/screen.c /usr/include/stdc-predef.h \
 /usr/lib/gcc/x86_64-linux-gnu/5/include/stdarg.h \
 ../include/geekos/kassert.h ../include/geekos/screen.h \
 ../include/geekos/ktypes.h \
 /usr/lib/gcc/x86_64-linux-gnu/5/include/stdbool.h ../include/geekos/io.h \
 ../include/geekos/int.h ../include/geekos/defs.h \
 ../include/geekos/fmtout.h ../include/geekos/../libc/fmtout.h
geekos/timer.o: ../src/geekos/timer.c /usr/include/stdc-predef.h \
 /usr/lib/gcc/x86_64-linux-gnu/5/include-fixed/limits.h \
 /usr/lib/gcc/x86_64-linux-gnu/5/include-fixed/syslimits.h \
 /usr/include/limits.h /usr/include/features.h /usr/include/sys/cdefs.h \
 /usr/include/bits/wordsize.h /usr/include/gnu/stubs.h \
 /usr/include/gnu/stubs-32.h /usr/include/bits/posix1_lim.h \
 /usr/include/bits/local_lim.h /usr/include/linux/limits.h \
 /usr/include/bits/posix2_lim.h ../include/geekos/io.h \
 ../include/geekos/ktypes.h \
 /usr/lib/gcc/x86_64-linux-gnu/5/include/stdbool.h \
 ../include/geekos/int.h ../include/geekos/kassert.h \
 ../include/geekos/screen.h ../include/geekos/defs.h \
 ../include/geekos/irq.h ../include/geekos/kthread.h \
 ../include/geekos/list.h ../include/geekos/timer.h \
 ../include/geekos/scheduler.h ../include/geekos/paging.h \
 ../include/geekos/bootinfo.h
geekos/mem.o: ../src/geekos/mem.c /usr/include/stdc-predef.h \
 ../include/geekos/defs.h ../include/geekos/ktypes.h \
 /usr/lib/gcc/x86_64-linux-gnu/5/include/stdbool.h \
 ../include/geekos/kassert.h ../include/geekos/screen.h \
 ../include/geekos/bootinfo.h ../include/geekos/gdt.h \
 ../include/geekos/int.h ../include/geekos/malloc.h \
 ../include/geekos/string.h ../include/geekos/../libc/string.h \
 /usr/lib/gcc/x86_64-linux-gnu/5/include/stddef.h \
 ../include/geekos/paging.h ../include/geekos/list.h \
 ../include/geekos/mem.h ../include/geekos/timer.h
geekos/crc32.o: ../src/geekos/crc32.c /usr/include/stdc-predef.h \
 ../include/geekos/crc32.h \
 /usr/lib/gcc/x86_64-linux-gnu/5/include/stddef.h \
 ../include/geekos/ktypes.h \
 /usr/lib/gcc/x86_64-linux-gnu/5/include/stdbool.h \
 ../include/geekos/kassert.h ../include/geekos/screen.h
geekos/gdt.o: ../src/geekos/gdt.c /usr/include/stdc-predef.h \
 ../include/geekos/kassert.h ../include/geekos/screen.h \
 ../include/geekos/ktypes.h \
 /usr/lib/gcc/x86_64-linux-gnu/5/include/stdbool.h \
 ../include/geekos/segment.h ../include/geekos/int.h \
 ../include/geekos/defs.h ../include/geekos/tss.h ../include/geekos/gdt.h
geekos/tss.o: ../src/geekos/tss.c /usr/include/stdc-predef.h \
 ../include/geekos/kassert.h ../include/geekos/screen.h \
 ../include/geekos/ktypes.h \
 /usr/lib/gcc/x86_64-linux-gnu/5/include/stdbool.h \
 ../include/geekos/defs.h ../include/geekos/gdt.h \
 ../include/geekos/segment.h ../include/geekos/string.h \
 ../include/geekos/../libc/string.h \
 /usr/lib/gcc/x86_64-linux-gnu/5/include/stddef.h ../include/geekos/tss.h
geekos/segment.o: ../src/geekos/segment.c /usr/include/stdc-predef.h \
 ../include/geekos/kassert.h ../include/geekos/screen.h \
 ../include/geekos/ktypes.h \
 /usr/lib/gcc/x86_64-linux-gnu/5/include/stdbool.h \
 ../include/geekos/string.h ../include/geekos/../libc/string.h \
 /usr/lib/gcc/x86_64-linux-gnu/5/include/stddef.h ../include/geekos/tss.h \
 ../include/geekos/segment.h
geekos/bget.o: ../src/geekos/bget.c /usr/include/stdc-predef.h \
 ../include/geekos/string.h ../include/geekos/../libc/string.h \
 /usr/lib/gcc/x86_64-linux-gnu/5/include/stddef.h \
 ../include/geekos/kassert.h ../include/geekos/screen.h \
 ../include/geekos/ktypes.h \
 /usr/lib/gcc/x86_64-linux-gnu/5/include/stdbool.h \
 ../include/geekos/bget.h
geekos/malloc.o: ../src/geekos/malloc.c /usr/include/stdc-predef.h \
 ../include/geekos/screen.h ../include/geekos/ktypes.h \
 /usr/lib/gcc/x86_64-linux-gnu/5/include/stdbool.h \
 ../include/geekos/int.h ../include/geekos/kassert.h \
 ../include/geekos/defs.h ../include/geekos/bget.h \
 ../include/geekos/malloc.h
geekos/synch.o: ../src/geekos/synch.c /usr/include/stdc-predef.h \
 ../include/geekos/kthread.h ../include/geekos/ktypes.h \
 /usr/lib/gcc/x86_64-linux-gnu/5/include/stdbool.h \
 ../include/geekos/list.h ../include/geekos/kassert.h \
 ../include/geekos/screen.h ../include/geekos/int.h \
 ../include/geekos/defs.h ../include/geekos/synch.h
geekos/kthread.o: ../src/geekos/kthread.c /usr/include/stdc-predef.h \
 ../include/geekos/kassert.h ../include/geekos/screen.h \
 ../include/geekos/ktypes.h \
 /usr/lib/gcc/x86_64-linux-gnu/5/include/stdbool.h \
 ../include/geekos/defs.h ../include/geekos/int.h ../include/geekos/mem.h \
 ../include/geekos/list.h ../include/geekos/paging.h \
 ../include/geekos/bootinfo.h ../include/geekos/symbol.h \
 ../include/geekos/string.h ../include/geekos/../libc/string.h \
 /usr/lib/gcc/x86_64-linux-gnu/5/include/stddef.h \
 ../include/geekos/kthread.h ../include/geekos/malloc.h \
 ../include/geekos/user.h ../include/geekos/segment.h \
 ../include/geekos/elf.h ../include/geekos/scheduler.h \
 ../include/geekos/argblock.h ../include/geekos/syscall.h
geekos/user.o: ../src/geekos/user.c /usr/include/stdc-predef.h \
 ../include/geekos/errno.h ../include/geekos/ktypes.h \
 /usr/lib/gcc/x86_64-linux-gnu/5/include/stdbool.h \
 ../include/geekos/kassert.h ../include/geekos/screen.h \
 ../include/geekos/int.h ../include/geekos/defs.h ../include/geekos/mem.h \
 ../include/geekos/list.h ../include/geekos/paging.h \
 ../include/geekos/bootinfo.h ../include/geekos/malloc.h \
 ../include/geekos/kthread.h ../include/geekos/vfs.h \
 ../include/geekos/fileio.h \
 /usr/lib/gcc/x86_64-linux-gnu/5/include/stddef.h \
 ../include/geekos/blockdev.h ../include/geekos/tss.h \
 ../include/geekos/user.h ../include/geekos/segment.h \
 ../include/geekos/elf.h
geekos/uservm.o: ../src/geekos/uservm.c /usr/include/stdc-predef.h \
 ../include/geekos/int.h ../include/geekos/kassert.h \
 ../include/geekos/screen.h ../include/geekos/ktypes.h \
 /usr/lib/gcc/x86_64-linux-gnu/5/include/stdbool.h \
 ../include/geekos/defs.h ../include/geekos/mem.h \
 ../include/geekos/list.h ../include/geekos/paging.h \
 ../include/geekos/bootinfo.h ../include/geekos/malloc.h \
 ../include/geekos/string.h ../include/geekos/../libc/string.h \
 /usr/lib/gcc/x86_64-linux-gnu/5/include/stddef.h \
 ../include/geekos/argblock.h ../include/geekos/kthread.h \
 ../include/geekos/range.h ../include/geekos/vfs.h \
 ../include/geekos/fileio.h ../include/geekos/blockdev.h \
 ../include/geekos/user.h ../include/geekos/segment.h \
 ../include/geekos/elf.h ../include/geekos/gdt.h \
 ../include/geekos/errno.h
geekos/argblock.o: ../src/geekos/argblock.c /usr/include/stdc-predef.h \
 ../include/geekos/ktypes.h \
 /usr/lib/gcc/x86_64-linux-gnu/5/include/stdbool.h \
 ../include/geekos/string.h ../include/geekos/../libc/string.h \
 /usr/lib/gcc/x86_64-linux-gnu/5/include/stddef.h \
 ../include/geekos/argblock.h
geekos/syscall.o: ../src/geekos/syscall.c /usr/include/stdc-predef.h \
 ../include/geekos/syscall.h ../include/geekos/kthread.h \
 ../include/geekos/ktypes.h \
 /usr/lib/gcc/x86_64-linux-gnu/5/include/stdbool.h \
 ../include/geekos/list.h ../include/geekos/kassert.h \
 ../include/geekos/screen.h ../include/geekos/errno.h \
 ../include/geekos/int.h ../include/geekos/defs.h ../include/geekos/elf.h \
 ../include/geekos/malloc.h ../include/geekos/keyboard.h \
 ../include/geekos/string.h ../include/geekos/../libc/string.h \
 /usr/lib/gcc/x86_64-linux-gnu/5/include/stddef.h \
 ../include/geekos/user.h ../include/geekos/segment.h \
 ../include/geekos/paging.h ../include/geekos/bootinfo.h \
 ../include/geekos/timer.h ../include/geekos/vfs.h \
 ../include/geekos/fileio.h ../include/geekos/blockdev.h \
 ../include/geekos/scheduler.h ../include/geekos/sysinfo.h \
 ../include/geekos/mqueue.h ../include/geekos/pipefs.h
geekos/dma.o: ../src/geekos/dma.c /usr/include/stdc-predef.h \
 ../include/geekos/screen.h ../include/geekos/ktypes.h \
 /usr/lib/gcc/x86_64-linux-gnu/5/include/stdbool.h \
 ../include/geekos/range.h ../include/geekos/int.h \
 ../include/geekos/kassert.h ../include/geekos/defs.h \
 ../include/geekos/io.h ../include/geekos/dma.h
geekos/floppy.o: ../src/geekos/floppy.c /usr/include/stdc-predef.h \
 ../include/geekos/screen.h ../include/geekos/ktypes.h \
 /usr/lib/gcc/x86_64-linux-gnu/5/include/stdbool.h \
 ../include/geekos/string.h ../include/geekos/../libc/string.h \
 /usr/lib/gcc/x86_64-linux-gnu/5/include/stddef.h ../include/geekos/mem.h \
 ../include/geekos/defs.h ../include/geekos/list.h \
 ../include/geekos/kassert.h ../include/geekos/paging.h \
 ../include/geekos/bootinfo.h ../include/geekos/malloc.h \
 ../include/geekos/int.h ../include/geekos/irq.h ../include/geekos/dma.h \
 ../include/geekos/io.h ../include/geekos/timer.h \
 ../include/geekos/kthread.h ../include/geekos/blockdev.h \
 ../include/geekos/fileio.h ../include/geekos/floppy.h
geekos/elf.o: ../src/geekos/elf.c /usr/include/stdc-predef.h \
 ../include/geekos/errno.h ../include/geekos/kassert.h \
 ../include/geekos/screen.h ../include/geekos/ktypes.h \
 /usr/lib/gcc/x86_64-linux-gnu/5/include/stdbool.h \
 ../include/geekos/pfat.h ../include/geekos/malloc.h \
 ../include/geekos/string.h ../include/geekos/../libc/string.h \
 /usr/lib/gcc/x86_64-linux-gnu/5/include/stddef.h \
 ../include/geekos/user.h ../include/geekos/segment.h \
 ../include/geekos/elf.h ../include/geekos/paging.h \
 ../include/geekos/defs.h ../include/geekos/bootinfo.h \
 ../include/geekos/list.h ../include/geekos/fileio.h
geekos/blockdev.o: ../src/geekos/blockdev.c /usr/include/stdc-predef.h \
 ../include/geekos/errno.h ../include/geekos/screen.h \
 ../include/geekos/ktypes.h \
 /usr/lib/gcc/x86_64-linux-gnu/5/include/stdbool.h \
 ../include/geekos/string.h ../include/geekos/../libc/string.h \
 /usr/lib/gcc/x86_64-linux-gnu/5/include/stddef.h \
 ../include/geekos/malloc.h ../include/geekos/int.h \
 ../include/geekos/kassert.h ../include/geekos/defs.h \
 ../include/geekos/kthread.h ../include/geekos/list.h \
 ../include/geekos/synch.h ../include/geekos/blockdev.h \
 ../include/geekos/fileio.h
geekos/ide.o: ../src/geekos/ide.c /usr/include/stdc-predef.h \
 ../include/geekos/ktypes.h \
 /usr/lib/gcc/x86_64-linux-gnu/5/include/stdbool.h \
 ../include/geekos/kassert.h ../include/geekos/screen.h \
 ../include/geekos/errno.h ../include/geekos/malloc.h \
 ../include/geekos/string.h ../include/geekos/../libc/string.h \
 /usr/lib/gcc/x86_64-linux-gnu/5/include/stddef.h ../include/geekos/io.h \
 ../include/geekos/int.h ../include/geekos/defs.h \
 ../include/geekos/timer.h ../include/geekos/kthread.h \
 ../include/geekos/list.h ../include/geekos/blockdev.h \
 ../include/geekos/fileio.h ../include/geekos/ide.h
geekos/vfs.o: ../src/geekos/vfs.c /usr/include/stdc-predef.h \
 ../include/geekos/errno.h ../include/geekos/list.h \
 ../include/geekos/ktypes.h \
 /usr/lib/gcc/x86_64-linux-gnu/5/include/stdbool.h \
 ../include/geekos/kassert.h ../include/geekos/screen.h \
 ../include/geekos/string.h ../include/geekos/../libc/string.h \
 /usr/lib/gcc/x86_64-linux-gnu/5/include/stddef.h \
 ../include/geekos/malloc.h ../include/geekos/synch.h \
 ../include/geekos/kthread.h ../include/geekos/vfs.h \
 ../include/geekos/fileio.h ../include/geekos/blockdev.h
geekos/pfat.o: ../src/geekos/pfat.c /usr/include/stdc-predef.h \
 /usr/lib/gcc/x86_64-linux-gnu/5/include-fixed/limits.h \
 /usr/lib/gcc/x86_64-linux-gnu/5/include-fixed/syslimits.h \
 /usr/include/limits.h /usr/include/features.h /usr/include/sys/cdefs.h \
 /usr/include/bits/wordsize.h /usr/include/gnu/stubs.h \
 /usr/include/gnu/stubs-32.h /usr/include/bits/posix1_lim.h \
 /usr/include/bits/local_lim.h /usr/include/linux/limits.h \
 /usr/include/bits/posix2_lim.h ../include/geekos/errno.h \
 ../include/geekos/screen.h ../include/geekos/ktypes.h \
 /usr/lib/gcc/x86_64-linux-gnu/5/include/stdbool.h \
 ../include/geekos/string.h ../include/geekos/../libc/string.h \
 /usr/lib/gcc/x86_64-linux-gnu/5/include/stddef.h \
 ../include/geekos/malloc.h ../include/geekos/ide.h \
 ../include/geekos/blockdev.h ../include/geekos/kthread.h \
 ../include/geekos/list.h ../include/geekos/kassert.h \
 ../include/geekos/fileio.h ../include/geekos/bitset.h \
 ../include/geekos/vfs.h ../include/geekos/synch.h \
 ../include/geekos/pfat.h
geekos/bitset.o: ../src/geekos/bitset.c /usr/include/stdc-predef.h \
 ../include/geekos/kassert.h ../include/geekos/screen.h \
 ../include/geekos/ktypes.h \
 /usr/lib/gcc/x86_64-linux-gnu/5/include/stdbool.h \
 ../include/geekos/malloc.h ../include/geekos/bitset.h \
 ../include/geekos/string.h ../include/geekos/../libc/string.h \
 /usr/lib/gcc/x86_64-linux-gnu/5/include/stddef.h
geekos/paging.o: ../src/geekos/paging.c /usr/include/stdc-predef.h \
 ../include/geekos/string.h ../include/geekos/../libc/string.h \
 /usr/lib/gcc/x86_64-linux-gnu/5/include/stddef.h ../include/geekos/int.h \
 ../include/geekos/kassert.h ../include/geekos/screen.h \
 ../include/geekos/ktypes.h \
 /usr/lib/gcc/x86_64-linux-gnu/5/include/stdbool.h \
 ../include/geekos/defs.h ../include/geekos/idt.h ../include/geekos/irq.h \
 ../include/geekos/kthread.h ../include/geekos/list.h \
 ../include/geekos/mem.h ../include/geekos/paging.h \
 ../include/geekos/bootinfo.h ../include/geekos/malloc.h \
 ../include/geekos/gdt.h ../include/geekos/segment.h \
 ../include/geekos/user.h ../include/geekos/elf.h ../include/geekos/vfs.h \
 ../include/geekos/fileio.h ../include/geekos/blockdev.h \
 ../include/geekos/crc32.h ../include/geekos/keyboard.h \
 ../include/geekos/errno.h ../include/geekos/timer.h
geekos/bufcache.o: ../src/geekos/bufcache.c /usr/include/stdc-predef.h \
 ../include/geekos/errno.h ../include/geekos/kassert.h \
 ../include/geekos/screen.h ../include/geekos/ktypes.h \
 /usr/lib/gcc/x86_64-linux-gnu/5/include/stdbool.h \
 ../include/geekos/mem.h ../include/geekos/defs.h \
 ../include/geekos/list.h ../include/geekos/paging.h \
 ../include/geekos/bootinfo.h ../include/geekos/malloc.h \
 ../include/geekos/blockdev.h ../include/geekos/kthread.h \
 ../include/geekos/fileio.h \
 /usr/lib/gcc/x86_64-linux-gnu/5/include/stddef.h \
 ../include/geekos/bufcache.h ../include/geekos/synch.h
geekos/gosfs.o: ../src/geekos/gosfs.c /usr/include/stdc-predef.h \
 /usr/lib/gcc/x86_64-linux-gnu/5/include-fixed/limits.h \
 /usr/lib/gcc/x86_64-linux-gnu/5/include-fixed/syslimits.h \
 /usr/include/limits.h /usr/include/features.h /usr/include/sys/cdefs.h \
 /usr/include/bits/wordsize.h /usr/include/gnu/stubs.h \
 /usr/include/gnu/stubs-32.h /usr/include/bits/posix1_lim.h \
 /usr/include/bits/local_lim.h /usr/include/linux/limits.h \
 /usr/include/bits/posix2_lim.h ../include/geekos/errno.h \
 ../include/geekos/kassert.h ../include/geekos/screen.h \
 ../include/geekos/ktypes.h \
 /usr/lib/gcc/x86_64-linux-gnu/5/include/stdbool.h \
 ../include/geekos/malloc.h ../include/geekos/string.h \
 ../include/geekos/../libc/string.h \
 /usr/lib/gcc/x86_64-linux-gnu/5/include/stddef.h \
 ../include/geekos/bitset.h ../include/geekos/synch.h \
 ../include/geekos/kthread.h ../include/geekos/list.h \
 ../include/geekos/int.h ../include/geekos/defs.h \
 ../include/geekos/bufcache.h ../include/geekos/gosfs.h \
 ../include/geekos/blockdev.h ../include/geekos/fileio.h \
 ../include/geekos/vfs.h ../include/geekos/user.h \
 ../include/geekos/segment.h ../include/geekos/elf.h \
 ../include/geekos/paging.h ../include/geekos/bootinfo.h \
 ../include/libc/sched.h
geekos/consfs.o: ../src/geekos/consfs.c /usr/include/stdc-predef.h \
 ../include/geekos/errno.h ../include/geekos/kassert.h \
 ../include/geekos/screen.h ../include/geekos/ktypes.h \
 /usr/lib/gcc/x86_64-linux-gnu/5/include/stdbool.h \
 ../include/geekos/vfs.h ../include/geekos/list.h \
 ../include/geekos/fileio.h \
 /usr/lib/gcc/x86_64-linux-gnu/5/include/stddef.h \
 ../include/geekos/blockdev.h ../include/geekos/kthread.h \
 ../include/geekos/malloc.h ../include/geekos/keyboard.h \
 ../include/geekos/consfs.h
geekos/pipefs.o: ../src/geekos/pipefs.c /usr/include/stdc-predef.h \
 ../include/geekos/kassert.h ../include/geekos/screen.h \
 ../include/geekos/ktypes.h \
 /usr/lib/gcc/x86_64-linux-gnu/5/include/stdbool.h \
 ../include/geekos/errno.h ../include/geekos/vfs.h \
 ../include/geekos/list.h ../include/geekos/fileio.h \
 /usr/lib/gcc/x86_64-linux-gnu/5/include/stddef.h \
 ../include/geekos/blockdev.h ../include/geekos/kthread.h \
 ../include/geekos/malloc.h ../include/geekos/string.h \
 ../include/geekos/../libc/string.h ../include/geekos/pipefs.h \
 ../include/geekos/int.h ../include/geekos/defs.h
geekos/main.o: ../src/geekos/main.c /usr/include/stdc-predef.h \
 ../include/geekos/bootinfo.h ../include/geekos/string.h \
 ../include/geekos/../libc/string.h \
 /usr/lib/gcc/x86_64-linux-gnu/5/include/stddef.h \
 ../include/geekos/screen.h ../include/geekos/ktypes.h \
 /usr/lib/gcc/x86_64-linux-gnu/5/include/stdbool.h \
 ../include/geekos/mem.h ../include/geekos/defs.h \
 ../include/geekos/list.h ../include/geekos/kassert.h \
 ../include/geekos/paging.h ../include/geekos/crc32.h \
 ../include/geekos/tss.h ../include/geekos/int.h \
 ../include/geekos/kthread.h ../include/geekos/trap.h \
 ../include/geekos/timer.h ../include/geekos/keyboard.h \
 ../include/geekos/dma.h ../include/geekos/ide.h \
 ../include/geekos/floppy.h ../include/geekos/pfat.h \
 ../include/geekos/vfs.h ../include/geekos/fileio.h \
 ../include/geekos/blockdev.h ../include/geekos/user.h \
 ../include/geekos/segment.h ../include/geekos/elf.h \
 ../include/geekos/gosfs.h ../include/geekos/synch.h \
 ../include/geekos/consfs.h ../include/geekos/mqueue.h
geekos/scheduler.o: ../src/geekos/scheduler.c /usr/include/stdc-predef.h \
 ../include/geekos/kassert.h ../include/geekos/screen.h \
 ../include/geekos/ktypes.h \
 /usr/lib/gcc/x86_64-linux-gnu/5/include/stdbool.h \
 ../include/geekos/defs.h ../include/geekos/int.h ../include/geekos/mem.h \
 ../include/geekos/list.h ../include/geekos/paging.h \
 ../include/geekos/bootinfo.h ../include/geekos/symbol.h \
 ../include/geekos/string.h ../include/geekos/../libc/string.h \
 /usr/lib/gcc/x86_64-linux-gnu/5/include/stddef.h \
 ../include/geekos/malloc.h ../include/geekos/user.h \
 ../include/geekos/segment.h ../include/geekos/elf.h \
 ../include/geekos/timer.h ../include/geekos/scheduler.h \
 ../include/geekos/kthread.h ../include/geekos/errno.h
geekos/sysinfo.o: ../src/geekos/sysinfo.c /usr/include/stdc-predef.h \
 ../include/geekos/kassert.h ../include/geekos/screen.h \
 ../include/geekos/ktypes.h \
 /usr/lib/gcc/x86_64-linux-gnu/5/include/stdbool.h \
 ../include/geekos/defs.h ../include/geekos/sysinfo.h \
 ../include/geekos/string.h ../include/geekos/../libc/string.h \
 /usr/lib/gcc/x86_64-linux-gnu/5/include/stddef.h \
 ../include/geekos/paging.h ../include/geekos/bootinfo.h \
 ../include/geekos/list.h ../include/geekos/scheduler.h \
 ../include/geekos/kthread.h ../include/libc/kernel.h
geekos/mqueue.o: ../src/geekos/mqueue.c /usr/include/stdc-predef.h \
 ../include/geekos/ktypes.h \
 /usr/lib/gcc/x86_64-linux-gnu/5/include/stdbool.h \
 ../include/geekos/screen.h ../include/geekos/int.h \
 ../include/geekos/kassert.h ../include/geekos/defs.h \
 ../include/geekos/mqueue.h ../include/geekos/kthread.h \
 ../include/geekos/list.h ../include/geekos/string.h \
 ../include/geekos/../libc/string.h \
 /usr/lib/gcc/x86_64-linux-gnu/5/include/stddef.h \
 ../include/geekos/malloc.h ../include/geekos/errno.h
libc/bget.o: ../src/libc/bget.c /usr/include/stdc-predef.h \
 ../include/libc/string.h \
 /usr/lib/gcc/x86_64-linux-gnu/5/include/stddef.h ../include/libc/conio.h \
 ../include/geekos/ktypes.h \
 /usr/lib/gcc/x86_64-linux-gnu/5/include/stdbool.h \
 ../include/geekos/keyboard.h ../include/geekos/screen.h \
 ../include/geekos/bget.h
libc/sched.o: ../src/libc/sched.c /usr/include/stdc-predef.h \
 ../include/geekos/syscall.h ../include/libc/string.h \
 /usr/lib/gcc/x86_64-linux-gnu/5/include/stddef.h
libc/sema.o: ../src/libc/sema.c /usr/include/stdc-predef.h \
 ../include/geekos/syscall.h ../include/libc/string.h \
 /usr/lib/gcc/x86_64-linux-gnu/5/include/stddef.h ../include/libc/sema.h
libc/fileio.o: ../src/libc/fileio.c /usr/include/stdc-predef.h \
 ../include/geekos/errno.h ../include/geekos/syscall.h \
 ../include/libc/fileio.h ../include/geekos/fileio.h \
 /usr/lib/gcc/x86_64-linux-gnu/5/include/stddef.h \
 ../include/geekos/ktypes.h \
 /usr/lib/gcc/x86_64-linux-gnu/5/include/stdbool.h \
 ../include/geekos/unistd.h ../include/libc/string.h
libc/mq.o: ../src/libc/mq.c /usr/include/stdc-predef.h ../include/libc/conio.h \
 /usr/lib/gcc/x86_64-linux-gnu/5/include/stddef.h \
 ../include/geekos/ktypes.h \
 /usr/lib/gcc/x86_64-linux-gnu/5/include/stdbool.h \
 ../include/geekos/keyboard.h ../include/geekos/screen.h \
 ../include/geekos/syscall.h ../include/libc/string.h \
 ../include/libc/mq.h ../include/geekos/mqueue.h
libc/unix.o: ../src/libc/unix.c /usr/include/stdc-predef.h \
 ../include/libc/conio.h /usr/lib/gcc/x86_64-linux-gnu/5/include/stddef.h \
 ../include/geekos/ktypes.h \
 /usr/lib/gcc/x86_64-linux-gnu/5/include/stdbool.h \
 ../include/geekos/keyboard.h ../include/geekos/screen.h \
 ../include/libc/process.h ../include/libc/fileio.h \
 ../include/geekos/fileio.h ../include/geekos/unistd.h \
 ../include/libc/unix.h
libc/curses.o: ../src/libc/curses.c /usr/include/stdc-predef.h \
 /usr/lib/gcc/x86_64-linux-gnu/5/include-fixed/limits.h \
 /usr/lib/gcc/x86_64-linux-gnu/5/include-fixed/syslimits.h \
 /usr/include/limits.h /usr/include/features.h /usr/include/sys/cdefs.h \
 /usr/include/bits/wordsize.h /usr/include/gnu/stubs.h \
 /usr/include/gnu/stubs-32.h /usr/include/bits/posix1_lim.h \
 /usr/include/bits/local_lim.h /usr/include/linux/limits.h \
 /usr/include/bits/posix2_lim.h ../include/libc/conio.h \
 /usr/lib/gcc/x86_64-linux-gnu/5/include/stddef.h \
 ../include/geekos/ktypes.h \
 /usr/lib/gcc/x86_64-linux-gnu/5/include/stdbool.h \
 ../include/geekos/keyboard.h ../include/geekos/screen.h \
 ../include/libc/process.h ../include/libc/fileio.h \
 ../include/geekos/fileio.h ../include/geekos/unistd.h \
 ../include/libc/curses.h
libc/compat.o: ../src/libc/compat.c /usr/include/stdc-predef.h \
 ../include/libc/conio.h /usr/lib/gcc/x86_64-linux-gnu/5/include/stddef.h \
 ../include/geekos/ktypes.h \
 /usr/lib/gcc/x86_64-linux-gnu/5/include/stdbool.h \
 ../include/geekos/keyboard.h ../include/geekos/screen.h \
 ../include/libc/kernel.h ../include/geekos/paging.h \
 ../include/geekos/defs.h ../include/geekos/bootinfo.h \
 ../include/geekos/list.h ../include/geekos/kassert.h \
 ../include/geekos/bget.h
libc/process.o: ../src/libc/process.c /usr/include/stdc-predef.h \
 /usr/lib/gcc/x86_64-linux-gnu/5/include/stddef.h \
 ../include/geekos/ktypes.h \
 /usr/lib/gcc/x86_64-linux-gnu/5/include/stdbool.h \
 ../include/geekos/syscall.h ../include/geekos/errno.h \
 ../include/libc/string.h ../include/libc/process.h
libc/conio.o: ../src/libc/conio.c /usr/include/stdc-predef.h \
 ../include/geekos/syscall.h ../include/libc/fmtout.h \
 /usr/lib/gcc/x86_64-linux-gnu/5/include/stdarg.h \
 ../include/libc/string.h \
 /usr/lib/gcc/x86_64-linux-gnu/5/include/stddef.h \
 ../include/libc/fileio.h ../include/geekos/fileio.h \
 ../include/geekos/ktypes.h \
 /usr/lib/gcc/x86_64-linux-gnu/5/include/stdbool.h \
 ../include/geekos/unistd.h ../include/libc/conio.h \
 ../include/geekos/keyboard.h ../include/geekos/screen.h
libc/kernel.o: ../src/libc/kernel.c /usr/include/stdc-predef.h \
 ../include/geekos/syscall.h ../include/libc/kernel.h \
 ../include/geekos/paging.h ../include/geekos/ktypes.h \
 /usr/lib/gcc/x86_64-linux-gnu/5/include/stdbool.h \
 ../include/geekos/defs.h ../include/geekos/bootinfo.h \
 ../include/geekos/list.h ../include/geekos/kassert.h \
 ../include/geekos/screen.h
libc/acl.o: ../src/libc/acl.c /usr/include/stdc-predef.h \
 ../include/geekos/syscall.h ../include/libc/string.h \
 /usr/lib/gcc/x86_64-linux-gnu/5/include/stddef.h ../include/libc/acl.h \
 ../include/geekos/ktypes.h \
 /usr/lib/gcc/x86_64-linux-gnu/5/include/stdbool.h
libc/errno.o: libc/errno.c /usr/include/stdc-predef.h
common/fmtout.o: ../src/common/fmtout.c /usr/include/stdc-predef.h \
 /usr/lib/gcc/x86_64-linux-gnu/5/include/stdarg.h \
 /usr/lib/gcc/x86_64-linux-gnu/5/include/stddef.h \
 ../include/geekos/string.h ../include/geekos/../libc/string.h \
 /usr/lib/gcc/x86_64-linux-gnu/5/include-fixed/limits.h \
 /usr/lib/gcc/x86_64-linux-gnu/5/include-fixed/syslimits.h \
 /usr/include/limits.h /usr/include/features.h /usr/include/sys/cdefs.h \
 /usr/include/bits/wordsize.h /usr/include/gnu/stubs.h \
 /usr/include/gnu/stubs-32.h /usr/include/bits/posix1_lim.h \
 /usr/include/bits/local_lim.h /usr/include/linux/limits.h \
 /usr/include/bits/posix2_lim.h ../include/geekos/fmtout.h \
 ../include/geekos/../libc/fmtout.h
common/string.o: ../src/common/string.c /usr/include/stdc-predef.h \
 ../include/libc/fmtout.h \
 /usr/lib/gcc/x86_64-linux-gnu/5/include/stdarg.h \
 ../include/libc/string.h \
 /usr/lib/gcc/x86_64-linux-gnu/5/include/stddef.h
common/memmove.o: ../src/common/memmove.c /usr/include/stdc-predef.h \
 ../include/libc/string.h \
 /usr/lib/gcc/x86_64-linux-gnu/5/include/stddef.h
