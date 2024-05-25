PACKAGE_NAME=htop-2.0.2
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
	CFLAGS=-I$(DIR_ROOT)/users/lib/include LDFLAGS=-L$(DIR_ROOT)/users/lib/lib \
	./configure --host=mips-linux --disable-unicode && \
	make

# make one package clean can't use users/Makefile export value
clean:
	rm -rf $(DIR_USERS)/build_dir/$(PACKAGE_NAME)

romfs:
	install -m0755 $(PACKAGE_PATH)/$(PACKAGE_NAME)/htop $(DIR_ROMFS)/usr/bin/htop
	echo "htop do romfs done."
