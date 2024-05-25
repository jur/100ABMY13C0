cmd_arch/mips/bsp/prom.o := msdk-linux-gcc -Wp,-MD,arch/mips/bsp/.prom.o.d  -nostdinc -isystem /workspace/wsr30/20240206_1102/marble/rtl819x/toolchain/msdk-4.4.7-mips-EB-3.10-0.9.33-m32t-131227b/bin/../lib/gcc/mips-linux-uclibc/4.4.7/include -I/workspace/wsr30/20240206_1102/marble/rtl819x/linux-3.10/arch/mips/include -Iarch/mips/include/generated  -Iinclude -I/workspace/wsr30/20240206_1102/marble/rtl819x/linux-3.10/arch/mips/include/uapi -Iarch/mips/include/generated/uapi -I/workspace/wsr30/20240206_1102/marble/rtl819x/linux-3.10/include/uapi -Iinclude/generated/uapi -include /workspace/wsr30/20240206_1102/marble/rtl819x/linux-3.10/include/linux/kconfig.h -D__KERNEL__ -DVMLINUX_LOAD_ADDRESS=0x80000000 -DDATAOFFSET=0 -Wall -Wundef -Wstrict-prototypes -Wno-trigraphs -fno-strict-aliasing -fno-common -Werror-implicit-function-declaration -Wno-format-security -fno-delete-null-pointer-checks -std=gnu89 -O1 -mno-check-zero-division -mabi=32 -G 0 -mno-abicalls -fno-pic -pipe -msoft-float -ffreestanding -EB -UMIPSEB -U_MIPSEB -U__MIPSEB -U__MIPSEB__ -UMIPSEL -U_MIPSEL -U__MIPSEL -U__MIPSEL__ -DMIPSEB -D_MIPSEB -D__MIPSEB -D__MIPSEB__ -U_MIPS_ISA -D_MIPS_ISA=_MIPS_ISA_MIPS32 -Wa,-mips32r2 -Wa,--trap -Iinclude/asm-mips -Iarch/mips/bsp/ -I/workspace/wsr30/20240206_1102/marble/rtl819x/linux-3.10/arch/mips/include/asm/mach-generic -Wframe-larger-than=1024 -fno-stack-protector -fomit-frame-pointer -Wdeclaration-after-statement -Wno-pointer-sign -fno-strict-overflow -fconserve-stack   -ffunction-sections -fdata-sections  -D"KBUILD_STR(s)=\#s" -D"KBUILD_BASENAME=KBUILD_STR(prom)"  -D"KBUILD_MODNAME=KBUILD_STR(prom)" -c -o arch/mips/bsp/prom.o arch/mips/bsp/prom.c

source_arch/mips/bsp/prom.o := arch/mips/bsp/prom.c

deps_arch/mips/bsp/prom.o := \
    $(wildcard include/config/early/printk.h) \
    $(wildcard include/config/rtk/819x/auto/zone/planning.h) \
    $(wildcard include/config/page/size/4kb.h) \
    $(wildcard include/config/page/size/16kb.h) \
    $(wildcard include/config/page/size/64kb.h) \
    $(wildcard include/config/rtl8198c/over/256mb.h) \
    $(wildcard include/config/rtk/voip.h) \
  include/linux/init.h \
    $(wildcard include/config/broken/rodata.h) \
    $(wildcard include/config/modules.h) \
  include/linux/compiler.h \
    $(wildcard include/config/sparse/rcu/pointer.h) \
    $(wildcard include/config/trace/branch/profiling.h) \
    $(wildcard include/config/profile/all/branches.h) \
    $(wildcard include/config/enable/must/check.h) \
    $(wildcard include/config/enable/warn/deprecated.h) \
    $(wildcard include/config/kprobes.h) \
  include/linux/compiler-gcc.h \
    $(wildcard include/config/arch/supports/optimized/inlining.h) \
    $(wildcard include/config/optimize/inlining.h) \
  include/linux/compiler-gcc4.h \
    $(wildcard include/config/arch/use/builtin/bswap.h) \
  include/linux/types.h \
    $(wildcard include/config/uid16.h) \
    $(wildcard include/config/lbdaf.h) \
    $(wildcard include/config/arch/dma/addr/t/64bit.h) \
    $(wildcard include/config/phys/addr/t/64bit.h) \
    $(wildcard include/config/64bit.h) \
  include/uapi/linux/types.h \
  /workspace/wsr30/20240206_1102/marble/rtl819x/linux-3.10/arch/mips/include/asm/types.h \
    $(wildcard include/config/64bit/phys/addr.h) \
  include/asm-generic/int-ll64.h \
  include/uapi/asm-generic/int-ll64.h \
  /workspace/wsr30/20240206_1102/marble/rtl819x/linux-3.10/arch/mips/include/uapi/asm/bitsperlong.h \
  include/asm-generic/bitsperlong.h \
  include/uapi/asm-generic/bitsperlong.h \
  /workspace/wsr30/20240206_1102/marble/rtl819x/linux-3.10/arch/mips/include/uapi/asm/types.h \
  /workspace/wsr30/20240206_1102/marble/rtl819x/linux-3.10/include/uapi/linux/posix_types.h \
  include/linux/stddef.h \
  include/uapi/linux/stddef.h \
  /workspace/wsr30/20240206_1102/marble/rtl819x/linux-3.10/arch/mips/include/uapi/asm/posix_types.h \
  /workspace/wsr30/20240206_1102/marble/rtl819x/linux-3.10/arch/mips/include/uapi/asm/sgidefs.h \
  /workspace/wsr30/20240206_1102/marble/rtl819x/linux-3.10/include/uapi/asm-generic/posix_types.h \
  include/linux/kernel.h \
    $(wildcard include/config/preempt/voluntary.h) \
    $(wildcard include/config/debug/atomic/sleep.h) \
    $(wildcard include/config/prove/locking.h) \
    $(wildcard include/config/ring/buffer.h) \
    $(wildcard include/config/tracing.h) \
    $(wildcard include/config/ftrace/mcount/record.h) \
  /workspace/wsr30/20240206_1102/marble/rtl819x/toolchain/msdk-4.4.7-mips-EB-3.10-0.9.33-m32t-131227b/bin/../lib/gcc/mips-linux-uclibc/4.4.7/include/stdarg.h \
  include/linux/linkage.h \
  include/linux/stringify.h \
  include/linux/export.h \
    $(wildcard include/config/have/underscore/symbol/prefix.h) \
    $(wildcard include/config/modversions.h) \
    $(wildcard include/config/unused/symbols.h) \
  /workspace/wsr30/20240206_1102/marble/rtl819x/linux-3.10/arch/mips/include/asm/linkage.h \
  include/linux/bitops.h \
  /workspace/wsr30/20240206_1102/marble/rtl819x/linux-3.10/arch/mips/include/asm/bitops.h \
    $(wildcard include/config/cpu/mipsr2.h) \
  /workspace/wsr30/20240206_1102/marble/rtl819x/linux-3.10/arch/mips/include/asm/barrier.h \
    $(wildcard include/config/cpu/has/sync.h) \
    $(wildcard include/config/cpu/cavium/octeon.h) \
    $(wildcard include/config/sgi/ip28.h) \
    $(wildcard include/config/cpu/has/wb.h) \
    $(wildcard include/config/weak/ordering.h) \
    $(wildcard include/config/smp.h) \
    $(wildcard include/config/weak/reordering/beyond/llsc.h) \
  /workspace/wsr30/20240206_1102/marble/rtl819x/linux-3.10/arch/mips/include/asm/addrspace.h \
    $(wildcard include/config/cpu/r8000.h) \
  /workspace/wsr30/20240206_1102/marble/rtl819x/linux-3.10/arch/mips/include/asm/mach-generic/spaces.h \
    $(wildcard include/config/32bit.h) \
    $(wildcard include/config/kvm/guest.h) \
    $(wildcard include/config/dma/noncoherent.h) \
  /workspace/wsr30/20240206_1102/marble/rtl819x/linux-3.10/include/uapi/linux/const.h \
  /workspace/wsr30/20240206_1102/marble/rtl819x/linux-3.10/arch/mips/include/uapi/asm/byteorder.h \
  include/linux/byteorder/big_endian.h \
  include/uapi/linux/byteorder/big_endian.h \
  include/linux/swab.h \
  include/uapi/linux/swab.h \
  /workspace/wsr30/20240206_1102/marble/rtl819x/linux-3.10/arch/mips/include/uapi/asm/swab.h \
  include/linux/byteorder/generic.h \
  /workspace/wsr30/20240206_1102/marble/rtl819x/linux-3.10/arch/mips/include/asm/cpu-features.h \
    $(wildcard include/config/cpu/mips4k.h) \
    $(wildcard include/config/cpu/mips24k.h) \
    $(wildcard include/config/cpu/mips34k.h) \
    $(wildcard include/config/cpu/mips74k.h) \
    $(wildcard include/config/cpu/mips1004k.h) \
    $(wildcard include/config/cpu/mips1074k.h) \
    $(wildcard include/config/cpu/has/fpu.h) \
    $(wildcard include/config/cpu/has/watch.h) \
    $(wildcard include/config/cpu/has/ar7.h) \
    $(wildcard include/config/cpu/mips14k.h) \
    $(wildcard include/config/cpu/has/dsp.h) \
    $(wildcard include/config/cpu/has/mipsmt.h) \
    $(wildcard include/config/cpu/has/tls.h) \
    $(wildcard include/config/cpu/mipsr2/irq/vi.h) \
    $(wildcard include/config/cpu/mipsr2/irq/ei.h) \
  /workspace/wsr30/20240206_1102/marble/rtl819x/linux-3.10/arch/mips/include/asm/cpu.h \
  /workspace/wsr30/20240206_1102/marble/rtl819x/linux-3.10/arch/mips/include/asm/cpu-info.h \
    $(wildcard include/config/mips/mt/smp.h) \
    $(wildcard include/config/mips/mt/smtc.h) \
  /workspace/wsr30/20240206_1102/marble/rtl819x/linux-3.10/arch/mips/include/asm/cache.h \
    $(wildcard include/config/mips/l1/cache/shift.h) \
  /workspace/wsr30/20240206_1102/marble/rtl819x/linux-3.10/arch/mips/include/asm/mach-generic/kmalloc.h \
    $(wildcard include/config/dma/coherent.h) \
  arch/mips/bsp/bspcpu.h \
  /workspace/wsr30/20240206_1102/marble/rtl819x/linux-3.10/arch/mips/include/asm/war.h \
    $(wildcard include/config/cpu/r4000/workarounds.h) \
    $(wildcard include/config/cpu/r4400/workarounds.h) \
    $(wildcard include/config/cpu/daddi/workarounds.h) \
  include/asm-generic/bitops/non-atomic.h \
  include/asm-generic/bitops/fls64.h \
  include/asm-generic/bitops/ffz.h \
  include/asm-generic/bitops/find.h \
    $(wildcard include/config/generic/find/first/bit.h) \
  include/asm-generic/bitops/sched.h \
  /workspace/wsr30/20240206_1102/marble/rtl819x/linux-3.10/arch/mips/include/asm/arch_hweight.h \
  include/asm-generic/bitops/arch_hweight.h \
  include/asm-generic/bitops/const_hweight.h \
  include/asm-generic/bitops/le.h \
  include/asm-generic/bitops/ext2-atomic.h \
  include/linux/log2.h \
    $(wildcard include/config/arch/has/ilog2/u32.h) \
    $(wildcard include/config/arch/has/ilog2/u64.h) \
  include/linux/typecheck.h \
  include/linux/printk.h \
    $(wildcard include/config/printk.h) \
    $(wildcard include/config/printk/func.h) \
    $(wildcard include/config/dynamic/debug.h) \
  include/linux/kern_levels.h \
  include/linux/dynamic_debug.h \
  include/linux/string.h \
    $(wildcard include/config/binary/printf.h) \
  include/uapi/linux/string.h \
  /workspace/wsr30/20240206_1102/marble/rtl819x/linux-3.10/arch/mips/include/asm/string.h \
    $(wildcard include/config/cpu/r3000.h) \
  include/linux/errno.h \
  include/uapi/linux/errno.h \
  /workspace/wsr30/20240206_1102/marble/rtl819x/linux-3.10/arch/mips/include/asm/errno.h \
  /workspace/wsr30/20240206_1102/marble/rtl819x/linux-3.10/arch/mips/include/uapi/asm/errno.h \
  /workspace/wsr30/20240206_1102/marble/rtl819x/linux-3.10/include/uapi/asm-generic/errno-base.h \
  include/uapi/linux/kernel.h \
    $(wildcard include/config/rlx.h) \
  /workspace/wsr30/20240206_1102/marble/rtl819x/linux-3.10/include/uapi/linux/sysinfo.h \
    $(wildcard include/config/rtl/819x.h) \
  /workspace/wsr30/20240206_1102/marble/rtl819x/linux-3.10/arch/mips/include/asm/div64.h \
  include/asm-generic/div64.h \
  /workspace/wsr30/20240206_1102/marble/rtl819x/linux-3.10/arch/mips/include/asm/bootinfo.h \
    $(wildcard include/config/swiotlb.h) \
  /workspace/wsr30/20240206_1102/marble/rtl819x/linux-3.10/arch/mips/include/asm/setup.h \
  /workspace/wsr30/20240206_1102/marble/rtl819x/linux-3.10/arch/mips/include/uapi/asm/setup.h \
  arch/mips/bsp/bspcpu.h \

arch/mips/bsp/prom.o: $(deps_arch/mips/bsp/prom.o)

$(deps_arch/mips/bsp/prom.o):
