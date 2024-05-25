PACKAGE_NAME = openssl-1.0.2h
SHAREDLIB_DIR = $(PACKAGE_NAME).shared
PACKAGE_PATH=$(ZYXEL_BUILD_DIR)/$(PACKAGE_NAME)

all: prepare build

prepare:
	# download openssl-1.0.2h source package
ifeq ("$(wildcard $(ZYXEL_DL)/$(PACKAGE_NAME).tar.gz)", "")
	wget -P $(ZYXEL_DL)/ $(ZYXEL_SITE)/$(PACKAGE_NAME).tar.gz;
endif

ifeq ("$(wildcard $(ZYXEL_BUILD_DIR))", "")
	mkdir -p $(ZYXEL_BUILD_DIR);
endif

ifeq ("$(wildcard $(ZYXEL_BUILD_DIR)/$(SHAREDLIB_DIR))", "")
	tar zxvf $(ZYXEL_DL)/$(PACKAGE_NAME).tar.gz -C $(ZYXEL_BUILD_DIR)/;
	mv $(ZYXEL_BUILD_DIR)/$(PACKAGE_NAME) $(ZYXEL_BUILD_DIR)/$(SHAREDLIB_DIR);
	mv $(ZYXEL_BUILD_DIR)/$(PACKAGE_NAME) $(ZYXEL_BUILD_DIR)/$(SHAREDLIB_DIR); \
	cd $(ZYXEL_BUILD_DIR)/$(SHAREDLIB_DIR) && \
	patch -p1 < $(DIR_USERS)/zyxel/$(PACKAGE_NAME)/patches/z01-Fix-possible-infinite-loop-in-BN_mod_sqrt.patch
endif

build:
ifeq ("$(wildcard $(ZYXEL_BUILD_DIR)/$(SHAREDLIB_DIR))", "")
# Now we build shared openssl library for others
	cd $(ZYXEL_BUILD_DIR)/$(SHAREDLIB_DIR);\
	CC='gcc' \
	./Configure linux-mips32 --cross-compile-prefix=$(CROSS_COMPILE) --with-zlib-include=$(DIR_USERS)/lib/include --with-zlib-lib=$(DIR_USERS)/lib/lib --openssldir=/etc/ssl shared -DOPENSSL_SMALL_FOOTPRINT no-idea no-md2 no-mdc2 no-rc5 no-sha0 no-camellia no-krb5 no-err no-hw no-zlib no-zlib-dynamic no-sse2 no-ssl2 no-engines no-ec; \
	make depend; \
	make; 
	mkdir -p $(DIR_USERS)/lib/include/openssl/;
	cp -d $(ZYXEL_BUILD_DIR)/$(SHAREDLIB_DIR)/libssl.so* $(DIR_USERS)/lib/lib/;
	cp -d $(ZYXEL_BUILD_DIR)/$(SHAREDLIB_DIR)/libcrypto.so* $(DIR_USERS)/lib/lib/;
	cp -p $(ZYXEL_BUILD_DIR)/$(SHAREDLIB_DIR)/include/openssl/* $(DIR_USERS)/lib/include/openssl/;
endif


clean:
	rm -rf $(DIR_USERS)/build_dir/$(SHAREDLIB_DIR)

romfs:
	$(ROMFSINST) $(ZYXEL_BUILD_DIR)/$(SHAREDLIB_DIR)/libssl.so.* /lib/
	$(ROMFSINST) $(ZYXEL_BUILD_DIR)/$(SHAREDLIB_DIR)/libcrypto.so.* /lib/
	mkdir -p $(DIR_ROMFS)/etc/ssl/
	$(ROMFSINST) $(ZYXEL_BUILD_DIR)/$(SHAREDLIB_DIR)/apps/openssl.cnf /etc/ssl/
	$(ROMFSINST) $(ZYXEL_BUILD_DIR)/$(SHAREDLIB_DIR)/apps/demoCA/private /etc/ssl/ 
	$(ROMFSINST) $(ZYXEL_BUILD_DIR)/$(SHAREDLIB_DIR)/certs /etc/ssl 
	$(ROMFSINST) $(ZYXEL_BUILD_DIR)/$(SHAREDLIB_DIR)/apps/openssl /bin/
	$(ROMFSINST) $(DIR_USERS)/zlib-1.2.8/libz.so /lib/
