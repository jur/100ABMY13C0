
PACKAGE_NAME=curl-7.59.0
TAR_FILE=$(PACKAGE_NAME).tar.bz2
PACKAGE_PATH=$(ZYXEL_BUILD_DIR)

C_ARES_PACKAGE=c-ares-1.14.0
all: prepare build

prepare:
ifneq ("$(ZYXEL_DL)/$(TAR_FILE)","$(wildcard $(ZYXEL_DL)/$(TAR_FILE))")
	# download source code
	wget -P $(ZYXEL_DL) $(ZYXEL_SITE)/$(TAR_FILE)
endif

ifneq ("$(PACKAGE_PATH)/$(PACKAGE_NAME)","$(wildcard $(PACKAGE_PATH)/$(PACKAGE_NAME))")
	@echo "PACKAGE_PATH=" $(PACKAGE_PATH) 
	@echo "PACKAGE_NAME=" $(PACKAGE_NAME)
	# prepare build_dir source code
	cd $(PACKAGE_PATH) && \
	tar jxvf $(ZYXEL_DL)/$(TAR_FILE) && \
	cd $(PACKAGE_NAME) && \
	patch -p1 < $(DIR_USERS)/zyxel/$(PACKAGE_NAME)/patches/200-no_docs_tests.patch && \
	patch -p1 < $(DIR_USERS)/zyxel/$(PACKAGE_NAME)/patches/310-mbedtls-disable-runtime-version-check.patch && \
	patch -p1 < $(DIR_USERS)/zyxel/$(PACKAGE_NAME)/patches/320-mbedtls_dont_use_deprecated_sha256_function.patch
endif

	cd $(PACKAGE_PATH)/$(PACKAGE_NAME)
	cd $(PACKAGE_PATH)/$(PACKAGE_NAME); \
	CC="$(CROSS_COMPILE)gcc" \
	LD="$(CROSS_COMPILE)ld" \
	GCC="$(CROSS_COMPILE)gcc" \
	CPPFLAGS="-I$(DIR_USERS)/lib/include/" CFLAGS="" LDFLAGS="-L$(DIR_USERS)/lib/lib" LIBS="-lssl -lcrypto -lcares" \
	./configure --host=mips-linux --enable-shared --enable-static --prefix="/usr" \
	--without-libidn --with-ssl  --enable-ares=$(ZYXEL_BUILD_DIR)/$(C_ARES_PACKAGE)/install/ \
	--without-librtmp --without-gnutls --without-nss --without-libssh2 --with-zlib --without-winidn --disable-rtsp --disable-ldap --disable-ldaps --disable-ipv6;

build:
	cd $(PACKAGE_PATH)/$(PACKAGE_NAME); \
	make; \
	cp -d lib/.libs/libcurl.so* $(DIR_USERS)/lib/lib/; \
	mkdir $(DIR_USERS)/lib/include/curl/; \
	cp include/curl/*.h $(DIR_USERS)/lib/include/curl/;

clean:
	if [ -d "$(DIR_USERS)/build_dir/$(PACKAGE_NAME)" ]; then \
		rm -r $(DIR_USERS)/build_dir/$(PACKAGE_NAME); \
	fi

romfs:
	# copy zlib dependency
	@cp -d $(DIR_USERS)/zlib-1.2.8/libz.so*  $(DIR_ROMFS)/lib/

	# copy curl binary & library
	@cp -d $(DIR_USERS)/build_dir/$(PACKAGE_NAME)/src/.libs/curl $(DIR_ROMFS)/bin/
	@cp -d $(DIR_USERS)/build_dir/$(PACKAGE_NAME)/lib/.libs/libcurl.so* $(DIR_ROMFS)/lib/
	@echo "curl do romfs done."
