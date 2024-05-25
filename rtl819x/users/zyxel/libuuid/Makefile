PACKAGE_NAME=libuuid-1.0.3
TAR_FILE=$(PACKAGE_NAME).tar.gz
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
	tar xzvf $(ZYXEL_DL)/$(TAR_FILE)
endif


build:
	cd $(PACKAGE_PATH)/$(PACKAGE_NAME) && \
	CC=$(CROSS_COMPILE)gcc \
	LD=$(CROSS_COMPILE)ld \
	./configure --prefix=${DIR_ROOT}/users/lib --host=mips-linux --enable-shared && \
	make && \
	make install

# make one package clean can't use users/Makefile export value
clean:
	rm -rf $(DIR_USERS)/build_dir/$(PACKAGE_NAME)

romfs:
	cp -d $(ZYXEL_BUILD_DIR)/$(PACKAGE_NAME)/.libs/*.so* $(DIR_ROMFS)/lib/ ;
	echo "libuuid do romfs done."
