
PACKAGE_NAME=uci-2012-11-09.1
TAR_FILE=$(PACKAGE_NAME).tar.gz
# SHAREDLIB=libzy_common_util.so
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
	tar zxvf $(ZYXEL_DL)/$(TAR_FILE) && \
	cd $(PACKAGE_NAME) && \
	patch -p1 < $(DIR_USERS)/zyxel/uci/patches/100-modify-config-path.patch && \
	patch -p1 < $(DIR_USERS)/zyxel/uci/patches/160-zyxel_patch_all.patch && \
	patch -p1 < $(DIR_USERS)/zyxel/uci/patches/190-close_uci_changed_get_zy.patch && \
	patch -p1 < $(DIR_USERS)/zyxel/uci/patches/200-remove_uci_changed_get_zy.patch
endif


build:
	cd $(PACKAGE_PATH)/$(PACKAGE_NAME) && \
	mkdir -p build && \
	cd build && \
	cmake .. && \
	make cli && \
	cp -d libuci.so $(DIR_USERS)/lib/lib/ && \
	cp ../uci.h $(DIR_USERS)/lib/include/ && \
	cp ../uci_config.h $(DIR_USERS)/lib/include/

# make one package clean can't use users/Makefile export value
clean:
	rm -rf $(DIR_USERS)/build_dir/$(PACKAGE_NAME)

romfs:

	cp -d $(PACKAGE_PATH)/$(PACKAGE_NAME)/build/libuci.so $(DIR_ROMFS)/lib/
	cp -d $(PACKAGE_PATH)/$(PACKAGE_NAME)/build/uci $(DIR_ROMFS)/bin/
	cp -d ./files/uci.sh $(DIR_ROMFS)/sbin/
	echo "uci do romfs done."
