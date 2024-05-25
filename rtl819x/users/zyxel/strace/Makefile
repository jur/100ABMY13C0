PACKAGE_NAME=strace-4.20
TAR_FILE=$(PACKAGE_NAME).tar.xz
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
	tar xJvf $(ZYXEL_DL)/$(TAR_FILE)
endif


build:
	cd $(PACKAGE_PATH)/$(PACKAGE_NAME) && \
	./configure --host=mips-linux && \
	make

# make one package clean can't use users/Makefile export value
clean:
	rm -rf $(DIR_USERS)/build_dir/$(PACKAGE_NAME)

romfs:
	cp $(PACKAGE_PATH)/$(PACKAGE_NAME)/strace $(DIR_ROMFS)/usr/bin/strace
	echo "strace do romfs done."
