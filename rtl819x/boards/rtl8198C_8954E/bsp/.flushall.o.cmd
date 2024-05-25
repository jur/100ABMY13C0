cmd_arch/mips/bsp/flushall.o := msdk-linux-gcc -Wp,-MD,arch/mips/bsp/.flushall.o.d  -nostdinc -isystem /workspace/wsr30/20240206_1102/marble/rtl819x/toolchain/msdk-4.4.7-mips-EB-3.10-0.9.33-m32t-131227b/bin/../lib/gcc/mips-linux-uclibc/4.4.7/include -I/workspace/wsr30/20240206_1102/marble/rtl819x/linux-3.10/arch/mips/include -Iarch/mips/include/generated  -Iinclude -I/workspace/wsr30/20240206_1102/marble/rtl819x/linux-3.10/arch/mips/include/uapi -Iarch/mips/include/generated/uapi -I/workspace/wsr30/20240206_1102/marble/rtl819x/linux-3.10/include/uapi -Iinclude/generated/uapi -include /workspace/wsr30/20240206_1102/marble/rtl819x/linux-3.10/include/linux/kconfig.h -D__KERNEL__ -DVMLINUX_LOAD_ADDRESS=0x80000000 -DDATAOFFSET=0 -Wall -Wundef -Wstrict-prototypes -Wno-trigraphs -fno-strict-aliasing -fno-common -Werror-implicit-function-declaration -Wno-format-security -fno-delete-null-pointer-checks -std=gnu89 -O1 -mno-check-zero-division -mabi=32 -G 0 -mno-abicalls -fno-pic -pipe -msoft-float -ffreestanding -EB -UMIPSEB -U_MIPSEB -U__MIPSEB -U__MIPSEB__ -UMIPSEL -U_MIPSEL -U__MIPSEL -U__MIPSEL__ -DMIPSEB -D_MIPSEB -D__MIPSEB -D__MIPSEB__ -U_MIPS_ISA -D_MIPS_ISA=_MIPS_ISA_MIPS32 -Wa,-mips32r2 -Wa,--trap -Iinclude/asm-mips -Iarch/mips/bsp/ -I/workspace/wsr30/20240206_1102/marble/rtl819x/linux-3.10/arch/mips/include/asm/mach-generic -Wframe-larger-than=1024 -fno-stack-protector -fomit-frame-pointer -Wdeclaration-after-statement -Wno-pointer-sign -fno-strict-overflow -fconserve-stack   -ffunction-sections -fdata-sections  -D"KBUILD_STR(s)=\#s" -D"KBUILD_BASENAME=KBUILD_STR(flushall)"  -D"KBUILD_MODNAME=KBUILD_STR(flushall)" -c -o arch/mips/bsp/flushall.o arch/mips/bsp/flushall.c

source_arch/mips/bsp/flushall.o := arch/mips/bsp/flushall.c

deps_arch/mips/bsp/flushall.o := \
  /workspace/wsr30/20240206_1102/marble/rtl819x/linux-3.10/arch/mips/include/asm/flushall.h \

arch/mips/bsp/flushall.o: $(deps_arch/mips/bsp/flushall.o)

$(deps_arch/mips/bsp/flushall.o):
