PACKAGE_NAME=pcre
PACKAGE_VERSION=8.11
TAR_FILE=$(PACKAGE_NAME)-$(PACKAGE_VERSION).tar.bz2

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

ifneq ("$(PACKAGE_PATH)/$(PACKAGE_NAME)","$(wildcard $(PACKAGE_PATH)/$(PACKAGE_NAME))")
	# prepare build_dir source code
	cd $(PACKAGE_PATH) && \
	tar jxvf $(ZYXEL_DL)/$(TAR_FILE)
endif

build:
	cd $(ZYXEL_BUILD_DIR)/$(PACKAGE_NAME)-$(PACKAGE_VERSION); \
	LD="$(CROSS_COMPILE)ld" \
	CC="$(CROSS_COMPILE)gcc" \
	GCC="$(CROSS_COMPILE)gcc" \
	./configure --target=mips-linux --host=mips-linux --build=i686-linux-gnu \
	--enable-utf8 --enable-unicode-properties --disable-cpp; \
	make; \
	cp pcre.h $(DIR_USERS)/lib/include/; \
	cp -d .libs/*.so* $(DIR_USERS)/lib/lib/; \
	cp -d .libs/*.a $(DIR_USERS)/lib/lib/; \
	cp pcre-config $(DIR_USERS)/lib/bin/;

# make one package clean can't use users/Makefile export value
clean:
	rm -rf $(DIR_USERS)/build_dir/$(PACKAGE_NAME)-$(PACKAGE_VERSION)

romfs:
	cp -d $(ZYXEL_BUILD_DIR)/$(PACKAGE_NAME)-$(PACKAGE_VERSION)/.libs/*.so* $(DIR_ROMFS)/lib/ ; \
	cp -d $(ZYXEL_BUILD_DIR)/$(PACKAGE_NAME)-$(PACKAGE_VERSION)/pcre-config $(DIR_ROMFS)/bin/ ; \
	echo "pcre do romfs done."
