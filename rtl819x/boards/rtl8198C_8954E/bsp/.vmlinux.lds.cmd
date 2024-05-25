cmd_arch/mips/bsp/vmlinux.lds := msdk-linux-gcc -E -Wp,-MD,arch/mips/bsp/.vmlinux.lds.d  -nostdinc -isystem /workspace/wsr30/20240206_1102/marble/rtl819x/toolchain/msdk-4.4.7-mips-EB-3.10-0.9.33-m32t-131227b/bin/../lib/gcc/mips-linux-uclibc/4.4.7/include -I/workspace/wsr30/20240206_1102/marble/rtl819x/linux-3.10/arch/mips/include -Iarch/mips/include/generated  -Iinclude -I/workspace/wsr30/20240206_1102/marble/rtl819x/linux-3.10/arch/mips/include/uapi -Iarch/mips/include/generated/uapi -I/workspace/wsr30/20240206_1102/marble/rtl819x/linux-3.10/include/uapi -Iinclude/generated/uapi -include /workspace/wsr30/20240206_1102/marble/rtl819x/linux-3.10/include/linux/kconfig.h -D__KERNEL__ -DVMLINUX_LOAD_ADDRESS=0x80000000 -DDATAOFFSET=0    -Wall -Wundef -Wstrict-prototypes -Wno-trigraphs -fno-strict-aliasing -fno-common -Werror-implicit-function-declaration -Wno-format-security -fno-delete-null-pointer-checks -std=gnu89 -O1  -mno-check-zero-division -mabi=32 -G 0 -mno-abicalls -fno-pic -pipe -msoft-float -ffreestanding -EB -UMIPSEB -U_MIPSEB -U__MIPSEB -U__MIPSEB__ -UMIPSEL -U_MIPSEL -U__MIPSEL -U__MIPSEL__ -DMIPSEB -D_MIPSEB -D__MIPSEB -D__MIPSEB__ -U_MIPS_ISA -D_MIPS_ISA=_MIPS_ISA_MIPS32 -Wa,-mips32r2 -Wa,--trap -Iinclude/asm-mips -Iarch/mips/bsp/ -I/workspace/wsr30/20240206_1102/marble/rtl819x/linux-3.10/arch/mips/include/asm/mach-generic -Wframe-larger-than=1024  -fno-stack-protector  -fomit-frame-pointer  -Wdeclaration-after-statement -Wno-pointer-sign -fno-strict-overflow -fconserve-stack  -P -C -Umips -D__ASSEMBLY__ -DLINKER_SCRIPT -o arch/mips/bsp/vmlinux.lds arch/mips/bsp/vmlinux.lds.S

source_arch/mips/bsp/vmlinux.lds := arch/mips/bsp/vmlinux.lds.S

deps_arch/mips/bsp/vmlinux.lds := \
    $(wildcard include/config/use/uapi.h) \
    $(wildcard include/config/mips/l1/cache/shift.h) \
    $(wildcard include/config/32bit.h) \
    $(wildcard include/config/cpu/little/endian.h) \
    $(wildcard include/config/boot/elf64.h) \
    $(wildcard include/config/mapped/kernel.h) \
  /workspace/wsr30/20240206_1102/marble/rtl819x/linux-3.10/arch/mips/include/asm/asm-offsets.h \
  include/generated/asm-offsets.h \
  /workspace/wsr30/20240206_1102/marble/rtl819x/linux-3.10/arch/mips/include/asm/thread_info.h \
    $(wildcard include/config/page/size/4kb.h) \
    $(wildcard include/config/64bit.h) \
    $(wildcard include/config/page/size/8kb.h) \
    $(wildcard include/config/page/size/16kb.h) \
    $(wildcard include/config/page/size/32kb.h) \
    $(wildcard include/config/page/size/64kb.h) \
  include/generated/uapi/linux/version.h \
  include/asm-generic/vmlinux.lds.h \
    $(wildcard include/config/hotplug.h) \
    $(wildcard include/config/hotplug/cpu.h) \
    $(wildcard include/config/memory/hotplug.h) \
    $(wildcard include/config/ftrace/mcount/record.h) \
    $(wildcard include/config/trace/branch/profiling.h) \
    $(wildcard include/config/profile/all/branches.h) \
    $(wildcard include/config/event/tracing.h) \
    $(wildcard include/config/tracing.h) \
    $(wildcard include/config/ftrace/syscalls.h) \
    $(wildcard include/config/clksrc/of.h) \
    $(wildcard include/config/irqchip.h) \
    $(wildcard include/config/common/clk.h) \
    $(wildcard include/config/function/graph/tracer.h) \
    $(wildcard include/config/constructors.h) \
    $(wildcard include/config/rtk/voip.h) \
    $(wildcard include/config/rtl/89xxd.h) \
    $(wildcard include/config/rtl/8881a.h) \
    $(wildcard include/config/generic/bug.h) \
    $(wildcard include/config/pm/trace.h) \
    $(wildcard include/config/blk/dev/initrd.h) \
  include/linux/export.h \
    $(wildcard include/config/have/underscore/symbol/prefix.h) \
    $(wildcard include/config/modules.h) \
    $(wildcard include/config/modversions.h) \
    $(wildcard include/config/unused/symbols.h) \

arch/mips/bsp/vmlinux.lds: $(deps_arch/mips/bsp/vmlinux.lds)

$(deps_arch/mips/bsp/vmlinux.lds):
