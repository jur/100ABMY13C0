
PACKAGE_NAME=curl-7.34.0
PACKAGE_PATH=$(DIR_USERS)/$(PACKAGE_NAME)
PACKAGE_BUILD_DIR_PATH=$(DIR_USERS)/build_dir/$(PACKAGE_NAME)

all: prepare build

prepare:
	if [ ! -d "$(PACKAGE_BUILD_DIR_PATH)"]; then \
		mkdir -p $(PACKAGE_BUILD_DIR_PATH); \
	fi; \
	cp -r $(PACKAGE_PATH) $(DIR_USERS)/build_dir; \
	cd $(PACKAGE_BUILD_DIR_PATH); \
	CC="$(CROSS_COMPILE)gcc" \
	LD="$(CROSS_COMPILE)ld" \
	GCC="$(CROSS_COMPILE)gcc" \
	CPPFLAGS="-I$(DIR_USERS)/lib/include/" CFLAGS="" LDFLAGS="-L$(DIR_USERS)/lib/lib" LIBS="-lssl -lcrypto" \
	./configure --host=mips-linux --enable-shared --enable-static --prefix="/usr" \
	--without-libidn --with-ssl  \
	--without-librtmp --without-gnutls --without-nss --without-libssh2 --with-zlib --without-winidn --disable-rtsp --disable-ldap --disable-ldaps --disable-ipv6;

build:
	cd $(PACKAGE_BUILD_DIR_PATH); \
	make; \
	cp -d lib/.libs/libcurl.so* $(DIR_USERS)/lib/lib/; \
	mkdir $(DIR_USERS)/lib/include/curl/; \
	cp include/curl/*.h $(DIR_USERS)/lib/include/curl/;

clean:
	if [ -d "$(PACKAGE_BUILD_DIR_PATH)" ]; then \
		rm -r $(PACKAGE_BUILD_DIR_PATH);\
	fi

romfs:
	# copy zlib dependency
	@cp -d $(DIR_USERS)/zlib-1.2.8/libz.so*  $(DIR_ROMFS)/lib/

	# copy curl binary & library
	@cp -d $(PACKAGE_BUILD_DIR_PATH)/src/.libs/curl $(DIR_ROMFS)/bin/
	@cp -d $(PACKAGE_BUILD_DIR_PATH)/lib/.libs/libcurl.so* $(DIR_ROMFS)/lib/
	@echo "curl do romfs done."
