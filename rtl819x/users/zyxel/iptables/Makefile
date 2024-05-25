PACKAGE_NAME=iptables-1.4.21
TAR_FILE=$(PACKAGE_NAME).tar.bz2
# SHAREDLIB=libzy_common_util.so
PACKAGE_PATH=$(ZYXEL_BUILD_DIR)

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
	tar xvf $(ZYXEL_DL)/$(TAR_FILE) && \
	cd $(PACKAGE_NAME) && \
	patch -p1 < $(DIR_USERS)/zyxel/iptables/patches/002-layer7_2.22.patch && \
	patch -p1 < $(DIR_USERS)/zyxel/iptables/patches/020-iptables-disable-modprobe.patch && \
	patch -p1 < $(DIR_USERS)/zyxel/iptables/patches/030-no-libnfnetlink.patch && \
	patch -p1 < $(DIR_USERS)/zyxel/iptables/patches/100-bash-location.patch && \
	patch -p1 < $(DIR_USERS)/zyxel/iptables/patches/200-configurable_builtin.patch && \
	patch -p1 < $(DIR_USERS)/zyxel/iptables/patches/300-musl_fixes.patch && \
	patch -p1 < $(DIR_USERS)/zyxel/iptables/patches/400-lenient-restore.patch && \
	patch -p1 < $(DIR_USERS)/zyxel/iptables/patches/500-ZYXEL_add_xtables_check_inverse.patch && \
	patch -p1 < $(DIR_USERS)/zyxel/iptables/patches/501-ZYXEL_iptables_webstr_PORTTRIGGER.patch
endif


build:
	cd $(PACKAGE_PATH)/$(PACKAGE_NAME) && \
	if [ $(CONFIG_APP_IP6TABLES_SUPPORT) = 1 ]; then \
		if [ ! -f ip6tables_config_done ]; then \
			./configure --host=mips-linux --with-ksource=$(DIR_LINUX_KERNEL) --enable-static --with-pic --disable-shared CFLAGS=-Os; \
			make depend; \
			make clean; \
			rm -f iptables_config_done; touch ip6tables_config_done; \
		fi; \
	else\
		if [ ! -f iptables_config_done ]; then \
			./configure --host=mips-linux --with-ksource=$(DIR_LINUX_KERNEL) --enable-static --with-pic --disable-shared --disable-ipv6 CFLAGS=-Os; \
			make depend; \
			make clean; \
			rm -f ip6tables_config_done; touch iptables_config_done; \
		fi; \
	fi; \
	make && \
	cp -d libiptc/.libs/*.a $(DIR_USERS)/lib/lib/;

# make one package clean can't use users/Makefile export value
clean:
	rm -rf $(DIR_USERS)/build_dir/$(PACKAGE_NAME)

romfs:
	cp -d $(PACKAGE_PATH)/$(PACKAGE_NAME)/iptables/xtables-multi $(DIR_ROMFS)/bin/iptables
	echo "iptables-1.4.21  do romfs done."
