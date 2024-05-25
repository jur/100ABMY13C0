# 6rd
PACKAGE_NAME = 6rd
PKG_VERSION = 2
PACKAGE_PATH = $(ZYXEL_BUILD_DIR)/$(PACKAGE_NAME)-$(PKG_VERSION)

all: prepare build

prepare:
ifneq ("$(PACKAGE_PATH)","$(wildcard $(PACKAGE_PATH))")
	mkdir -p $(PACKAGE_PATH)
	cp -rf ./src/* $(PACKAGE_PATH)
	cd $(PACKAGE_PATH) && \
	patch -p1 < $(DIR_USERS)/zyxel/$(PACKAGE_NAME)/patches/010-ZYXEL_break_while_loop_v6it.patch
endif

build:
	cd $(PACKAGE_PATH) && \
	make

# make one package clean can't use users/Makefile export value
clean:
	rm -rf $(DIR_USERS)/build_dir/$(PACKAGE_NAME)

romfs:
	cp -d $(PACKAGE_PATH)/6rdcalc $(DIR_ROMFS)/usr/bin/
	cp -d ./files/6rd_wsr30.sh $(DIR_ROMFS)/usr/sbin
	cp -d ./files/ipcalc.sh $(DIR_ROMFS)/usr/sbin
