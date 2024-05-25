PACKAGE_NAME=libpcap
PACKAGE_VERSION=1.7.4
TAR_FILE=$(PACKAGE_NAME)-$(PACKAGE_VERSION).tar.gz

PACKAGE_PATH=$(ZYXEL_BUILD_DIR)

all: prepare build

prepare:
ifneq ("$(ZYXEL_DL)/$(TAR_FILE)","$(wildcard $(ZYXEL_DL)/$(TAR_FILE))")
	# download source code
	wget -P $(ZYXEL_DL) $(ZYXEL_SITE)/$(TAR_FILE)
endif

ifeq ("$(wildcard $(ZYXEL_BUILD_DIR))", "")
	mkdir -p $(ZYXEL_BUILD_DIR);
endif

ifneq ("$(PACKAGE_PATH)/$(PACKAGE_NAME)-$(PACKAGE_VERSION)","$(wildcard $(PACKAGE_PATH)/$(PACKAGE_NAME)-$(PACKAGE_VERSION))")
	# prepare build_dir source code
	cd $(PACKAGE_PATH) && \
	tar zxvf $(ZYXEL_DL)/$(PACKAGE_NAME)-$(PACKAGE_VERSION).tar.gz -C $(ZYXEL_BUILD_DIR)
endif

build:
ifeq ("$(wildcard, $(PACKAGE_PATH)/$(PACKAGE_NAME)-$(PACKAGE_VERSION)/install)", "")
	cd $(PACKAGE_PATH)/$(PACKAGE_NAME)-$(PACKAGE_VERSION) ; \
	LD="$(CROSS_COMPILE)ld" \
	CC="$(CROSS_COMPILE)gcc" \
	GCC="$(CROSS_COMPILE)gcc" \
	CFLAGS="-I $(DIR_USERS)/lib/include" \
	LDFLAGS="" \
	./configure --host=mips-gun-linux --prefix="$(PACKAGE_PATH)/$(PACKAGE_NAME)-$(PACKAGE_VERSION)/install" \
	--enable-shared --disable-yydebug --enable-ipv6 --disable-canusb \
	--with-pcap=linux --without-libnl && \
	make -j 4 && \
	make install && \
	cp -rf ./install/include/* $(DIR_USERS)/lib/include/ ; \
	cp -d ./install/lib/libpcap.so* $(DIR_USERS)/lib/lib/ ; \
	cp -d ./install/lib/libpcap.a $(DIR_USERS)/lib/lib/ ;
endif

# make one package clean can't use users/Makefile export value
clean:
	rm -rf $(DIR_USERS)/build_dir/$(PACKAGE_NAME)-$(PACKAGE_VERSION)

romfs:
	echo "$(PACKAGE_NAME) do romfs done."
