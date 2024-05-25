PACKAGE_NAME=c-ares
PACKAGE_VERSION=1.14.0
TAR_FILE=$(PACKAGE_NAME)-$(PACKAGE_VERSION).tar.gz
PACKAGE_PATH=$(ZYXEL_BUILD_DIR)

all: prepare build

prepare:
ifneq ("$(ZYXEL_DL)/$(TAR_FILE)","$(wildcard $(ZYXEL_DL)/$(TAR_FILE))")
	# download source code
	wget -P $(ZYXEL_DL) $(ZYXEL_SITE)/$(TAR_FILE)
	#cp $(DIR_USERS)/zyxel/$(PACKAGE_NAME)/$(TAR_FILE) $(ZYXEL_DL)
endif
ifneq ("$(PACKAGE_PATH)/$(PACKAGE_NAME)-$(PACKAGE_VERSION)","$(wildcard $(PACKAGE_PATH)/$(PACKAGE_NAME)-$(PACKAGE_VERSION))")
	# prepare build_dir source code
	cd $(PACKAGE_PATH) && \
	tar xzvf $(ZYXEL_DL)/$(TAR_FILE)
endif


build:
ifeq ("$(wildcard $(ZYXEL_BUILD_DIR)/$(PACKAGE_NAME)-$(PACKAGE_VERSION)/install)", "")
	cd $(PACKAGE_PATH)/$(PACKAGE_NAME)-$(PACKAGE_VERSION) ; \
	CC="$(CROSS_COMPILE)gcc" \
	LD="$(CROSS_COMPILE)ld" \
	GCC="$(CROSS_COMPILE)gcc" \
	CFLAGS="-fPIC" \
	CPPFLAGS="-I$(DIR_USERS)/lib/include/" LDFLAGS="-L$(DIR_USERS)/lib/lib/" \
	./configure --host=mips-linux --target=mips-linux --prefix=$(ZYXEL_BUILD_DIR)/$(PACKAGE_NAME)-$(PACKAGE_VERSION)/install --disable-shared && \
	make ; make install; \
	cp -d .libs/libcares.a $(DIR_USERS)/lib/lib/; \
	cp ares.h $(DIR_USERS)/lib/include/;
endif

# make one package clean can't use users/Makefile export value
clean:
	rm -rf $(DIR_USERS)/build_dir/$(PACKAGE_NAME)-$(PACKAGE_VERSION)

romfs:
	@#cp -d $(PACKAGE_PATH)/$(PACKAGE_NAME)-$(PACKAGE_VERSION)/.libs/libcares.so.* $(DIR_ROMFS)/lib/
	echo "$(PACKAGE_NAME)-$(PACKAGE_VERSION) do romfs done."
