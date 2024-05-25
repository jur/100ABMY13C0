PACKAGE_NAME=lighttpd
PKG_VERSION:=1.4.37
TAR_FILE=$(PACKAGE_NAME)-$(PKG_VERSION).tar.xz
PACKAGE_PATH=$(ZYXEL_BUILD_DIR)

$(info "")
$(info "====================")
$(info "PACKAGE_NAME=[$(PACKAGE_NAME)]")
$(info "PKG_VERSION=[$(PKG_VERSION)]")
$(info "TAR_FILE=[$(TAR_FILE)]")
$(info "PACKAGE_PATH=[$(PACKAGE_PATH)]")
$(info "ZYXEL_BUILD_DIR=[$(ZYXEL_BUILD_DIR)]")
$(info "ZYXEL_DL=[$(ZYXEL_DL)]")
$(info "ZYXEL_SITE=[$(ZYXEL_SITE)]")
$(info "DIR_ROOT=[$(DIR_ROOT)]")
$(info "DIR_USERS=[$(DIR_USERS)]")
$(info "PATH=[$(PATH)]")
$(info "====================")
$(info "")

all: prepare build

prepare:
ifneq ("$(ZYXEL_DL)/$(TAR_FILE)","$(wildcard $(ZYXEL_DL)/$(TAR_FILE))")
	# download source code
	wget -P $(ZYXEL_DL) $(ZYXEL_SITE)/$(TAR_FILE)
endif

ifneq ("$(PACKAGE_PATH)/$(PACKAGE_NAME)","$(wildcard $(PACKAGE_PATH)/$(PACKAGE_NAME))")
	# prepare build_dir source code
	cd $(PACKAGE_PATH) && \
	tar Jxvf $(ZYXEL_DL)/$(TAR_FILE) && \
	cd -
endif

# prepare
# echo "#define HAVE_LIBPCRE 1" >> $(PACKAGE_PATH)/$(PACKAGE_NAME)-$(PKG_VERSION)/config.h && \
# echo "#define HAVE_PCRE_H 1" >> $(PACKAGE_PATH)/$(PACKAGE_NAME)-$(PKG_VERSION)/config.h && \

CONFIGURE_ARGS+= \
	--libdir=/usr/lib/lighttpd \
	--sysconfdir=/etc/lighttpd \
	--enable-shared \
	--enable-static \
	--disable-rpath \
	--without-attr \
	--without-bzip2 \
	--without-fam \
	--without-gdbm \
	--without-ldap \
	--without-lua \
	--without-memcache \
	--without-mysql \
	--with-pcre \
	--without-valgrind \
	--with-openssl


build:
	cd $(PACKAGE_PATH)/$(PACKAGE_NAME)-$(PKG_VERSION) && \
	LD="$(CROSS_COMPILE)ld" \
	CC="$(CROSS_COMPILE)gcc" \
	GCC="$(CROSS_COMPILE)gcc" \
	PATH="$(PATH):$(DIR_USERS)/lib/bin/" \
	./configure --target=mips-linux --host=mips-linux --build=i686-linux-gnu \
	$(CONFIGURE_ARGS) \
	CPPFLAGS=" -I$(DIR_ROOT)/users/lib/include/ -I$(DIR_USERS)/lib/bin/ -I$(DIR_ROOT)/users/lib/lib/ -I$(DIR_ROOT)/users/lib/include/openssl/" \
	LDFLAGS=" -I$(DIR_ROOT)/users/lib/include/ -I$(DIR_USERS)/lib/bin/ -L$(DIR_USERS)/lib/bin/ -L$(DIR_ROOT)/users/lib/lib/ -I$(DIR_ROOT)/users/lib/include/openssl/" \
	LIBS=" -lpthread -lpcre -lcrypto -lssl" && \
	make


# make one package clean can't use users/Makefile export value
clean:
	rm -rf $(DIR_USERS)/build_dir/$(PACKAGE_NAME)-$(PKG_VERSION)


romfs:
	mkdir -p $(DIR_ROMFS)/etc/lighttpd/files_for_symlink/etc/lighttpd/conf.d/
	ln -s /tmp/ApplicationData/lighttpd/etc/lighttpd/conf.d $(DIR_ROMFS)/etc/lighttpd/conf.d
	cp -d ./files/lighttpd.conf $(DIR_ROMFS)/etc/lighttpd/

	mkdir -p $(DIR_ROMFS)/etc/default/
	cp -d ./files/lighttpd.default  $(DIR_ROMFS)/etc/default/lighttpd
	mkdir -p $(DIR_ROMFS)/etc/init.d/
	cp -d ./files/lighttpd.init  $(DIR_ROMFS)/etc/init.d/lighttpd_init
	cp -d ./files/uhttpd  $(DIR_ROMFS)/etc/init.d/uhttpd
	mkdir -p $(DIR_ROMFS)/usr/lib/lighttpd
	for m in dirlisting indexfile staticfile; do \
		cp $(ZYXEL_BUILD_DIR)/$(PACKAGE_NAME)-$(PKG_VERSION)/src/.libs/mod_$$m.so $(DIR_ROMFS)/usr/lib/lighttpd/ ; \
	done
	cp $(ZYXEL_BUILD_DIR)/$(PACKAGE_NAME)-$(PKG_VERSION)/src/.libs/mod_cgi.so $(DIR_ROMFS)/usr/lib/lighttpd/
	cp $(ZYXEL_BUILD_DIR)/$(PACKAGE_NAME)-$(PKG_VERSION)/src/.libs/mod_fastcgi.so $(DIR_ROMFS)/usr/lib/lighttpd/
	cp $(ZYXEL_BUILD_DIR)/$(PACKAGE_NAME)-$(PKG_VERSION)/src/.libs/mod_rewrite.so $(DIR_ROMFS)/usr/lib/lighttpd/
	cp $(ZYXEL_BUILD_DIR)/$(PACKAGE_NAME)-$(PKG_VERSION)/src/.libs/mod_redirect.so $(DIR_ROMFS)/usr/lib/lighttpd/
	cp $(ZYXEL_BUILD_DIR)/$(PACKAGE_NAME)-$(PKG_VERSION)/src/.libs/mod_setenv.so $(DIR_ROMFS)/usr/lib/lighttpd/
	mkdir -p $(DIR_ROMFS)/usr/sbin
	cp -d $(ZYXEL_BUILD_DIR)/$(PACKAGE_NAME)-$(PKG_VERSION)/src/lighttpd $(DIR_ROMFS)/usr/sbin/
	cp -d ./files/lighttpd-port  $(DIR_ROMFS)/usr/sbin/
	mkdir -p $(DIR_ROMFS)/etc/uci_defconfig/
	cp -d ./files/uhttpd.config  $(DIR_ROMFS)/etc/uci_defconfig/uhttpd
	echo "lighttpd do romfs done."

