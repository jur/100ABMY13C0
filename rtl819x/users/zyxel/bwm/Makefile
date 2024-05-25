PACKAGE_NAME=bwm
PACKAGE_VERSION=1.1.0
TAR_FILE=$(PACKAGE_NAME)_$(PACKAGE_VERSION).orig.tar.gz
PACKAGE_PATH=$(ZYXEL_BUILD_DIR)

all: prepare build

prepare:
ifneq ("$(ZYXEL_DL)/$(TAR_FILE)","$(wildcard $(ZYXEL_DL)/$(TAR_FILE))")
	# download source code
	wget -P $(ZYXEL_DL) $(ZYXEL_SITE)/$(TAR_FILE)
endif
ifneq ("$(PACKAGE_PATH)/$(PACKAGE_NAME)","$(wildcard $(PACKAGE_PATH)/$(PACKAGE_NAME))")
	# prepare build_dir source code
	cd $(PACKAGE_PATH) && \
	tar xzvf $(ZYXEL_DL)/$(TAR_FILE) && \
	cd $(PACKAGE_NAME)-$(PACKAGE_VERSION).orig && \
	patch -p1 < $(DIR_USERS)/zyxel/$(PACKAGE_NAME)/patches/001-debian.patch && \
	patch -p1 < $(DIR_USERS)/zyxel/$(PACKAGE_NAME)/patches/002-remove_unused_func.patch && \
	patch -p1 < $(DIR_USERS)/zyxel/$(PACKAGE_NAME)/patches/003-bugfix_bytes_count_overflow.patch && \
	patch -p1 < $(DIR_USERS)/zyxel/$(PACKAGE_NAME)/patches/004-variable_interface_table.patch && \
	patch -p1 < $(DIR_USERS)/zyxel/$(PACKAGE_NAME)/patches/005-match_exact_interface_name.patch
endif


build:
	cd $(PACKAGE_PATH)/$(PACKAGE_NAME)-$(PACKAGE_VERSION).orig && \
	CC=$(CROSS_COMPILE)gcc && \
	$(CC) -Wall -O2 bwm.c -o bwm

# make one package clean can't use users/Makefile export value
clean:
	rm -rf $(DIR_USERS)/build_dir/$(PACKAGE_NAME)-$(PACKAGE_VERSION).orig

romfs:
	cp $(PACKAGE_PATH)/$(PACKAGE_NAME)-$(PACKAGE_VERSION).orig/bwm $(DIR_ROMFS)/usr/bin/bwm
	cp -d $(DIR_USERS)/zyxel/$(PACKAGE_NAME)/files/bwm.init $(DIR_ROMFS)/etc/init.d/bwm
	echo "bwm do romfs done."

