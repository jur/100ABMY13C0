ifeq ($(wildcard $(DIR_LINUX)/.config),)
include ../../linux-2.6.30/.config
DIR_LINUX_KERNEL = ../../linux-2.6.30
else
KERNEL_VER=$(patsubst $(DIR_ROOT)/%,%,$(DIR_LINUX))
include $(DIR_LINUX)/.config
DIR_LINUX_KERNEL = $(DIR_LINUX)
endif

ifeq ($(wildcard $(DIR_USERS)/.config),)
include ../.config
else
include $(DIR_USERS)/.config
endif

ifeq ($(CONFIG_RTL_8197F),y)
CFLAGS += -DCONFIG_RTL_8197F
endif

ifeq ($(CONFIG_ZYXEL_WSR30),y)
CFLAGS += -DCONFIG_ZYXEL_WSR30
endif

CFLAGS += -Os
 
EXEC = watchdog
OBJS = watchdog.o
all: $(EXEC)
 
$(EXEC): $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS)  -o $@ $(OBJS) $(LDLIBS)  
 
romfs:
	$(ROMFSINST) /bin/$(EXEC)
 
clean:
	rm -f $(EXEC) *.o

