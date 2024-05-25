PACKAGE_NAME=at
PKG_SOURCE_VERSION:=3.1.12
PACKAGE_PATH=$(ZYXEL_BUILD_DIR)
TAR_FILE=$(PACKAGE_NAME)_$(PKG_SOURCE_VERSION).orig.tar.gz
#PKG_MD5SUM:=1e67991776148fb319fd77a2e599a765

all: prepare build


prepare:
ifneq ("$(ZYXEL_DL)/$(TAR_FILE)","$(wildcard $(ZYXEL_DL)/$(TAR_FILE))")
	# download source code
	wget -P $(ZYXEL_DL) $(ZYXEL_SITE)/$(TAR_FILE)
endif
ifneq ("$(PACKAGE_PATH)/$(PACKAGE_NAME)-$(PKG_SOURCE_VERSION)","$(wildcard $(PACKAGE_PATH)/$(PACKAGE_NAME)-$(PKG_SOURCE_VERSION) )")
	# prepare build_dir source code
	cd $(PACKAGE_PATH) && \
	tar -xvf $(ZYXEL_DL)/$(TAR_FILE)
	cd $(PACKAGE_PATH)/$(PACKAGE_NAME)-$(PKG_SOURCE_VERSION) && \
	patch -p1 < $(DIR_USERS)/zyxel/$(PACKAGE_NAME)/patches/100-cross-compile.patch
	cd $(PACKAGE_PATH)/$(PACKAGE_NAME)-$(PKG_SOURCE_VERSION) && \
	patch -p1 < $(DIR_USERS)/zyxel/$(PACKAGE_NAME)/patches/110-getloadavg.patch
endif
	export SENDMAIL=/bin/true && \
	cd $(PACKAGE_PATH)/$(PACKAGE_NAME)-$(PKG_SOURCE_VERSION) && \
	LD="$(CROSS_COMPILE)ld" \
	CC="$(CROSS_COMPILE)gcc" \
	GCC="$(CROSS_COMPILE)gcc" \
	CFLAGS="-I$(DIR_USERS)/lib/include/ -DNEED_YYWRAP " LIBS="-L$(DIR_USERS)/lib/lib/" \
	./configure  --target=mips-linux --host=mips-linux --build=i686-linux-gnu \
		--with-daemon_username=nobody \
		--with-daemon_groupname=nobody \
		--with-jobdir=/var/spool/cron/atjobs \
		--with-atspool=/var/spool/cron/atspool ; \


build:
	cd $(PACKAGE_PATH)/$(PACKAGE_NAME)-$(PKG_SOURCE_VERSION) && \
	make


# make one package clean can't use users/Makefile export value
clean:
	rm -rf $(DIR_USERS)/build_dir/$(PACKAGE_NAME)-$(PKG_SOURCE_VERSION)


romfs:
	cp -d ./files/atd.init $(DIR_ROMFS)/etc/init.d/atd
	cp -d $(PACKAGE_PATH)/$(PACKAGE_NAME)-$(PKG_SOURCE_VERSION)/at $(DIR_ROMFS)/usr/bin/
	cp -d $(PACKAGE_PATH)/$(PACKAGE_NAME)-$(PKG_SOURCE_VERSION)/atd $(DIR_ROMFS)/usr/bin/
	cp -d $(PACKAGE_PATH)/$(PACKAGE_NAME)-$(PKG_SOURCE_VERSION)/atq $(DIR_ROMFS)/usr/bin/
	cp -d $(PACKAGE_PATH)/$(PACKAGE_NAME)-$(PKG_SOURCE_VERSION)/atrm $(DIR_ROMFS)/usr/bin/
	echo "at do romfs done."

