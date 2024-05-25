PACKAGE_NAME=dnsmasq-2.78
TAR_FILE=$(PACKAGE_NAME).tar.xz
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
	patch -p1 < $(DIR_USERS)/zyxel/$(PACKAGE_NAME)/patches/100-create_lease2.patch && \
	patch -p1 < $(DIR_USERS)/zyxel/$(PACKAGE_NAME)/patches/110-ipset-remove-old-kernel-support.patch && \
	patch -p1 < $(DIR_USERS)/zyxel/$(PACKAGE_NAME)/patches/210-dnssec-improve-timestamp-heuristic.patch && \
	patch -p1 < $(DIR_USERS)/zyxel/$(PACKAGE_NAME)/patches/230-fix-poll-h-include-warning-on-musl.patch && \
	patch -p1 < $(DIR_USERS)/zyxel/$(PACKAGE_NAME)/patches/231-change-ETHERSFILE-path.patch
endif


build:
	cd $(PACKAGE_PATH)/$(PACKAGE_NAME) && \
	make

# make one package clean can't use users/Makefile export value
clean:
	rm -rf $(DIR_USERS)/build_dir/$(PACKAGE_NAME)

romfs:
	cp -d $(PACKAGE_PATH)/$(PACKAGE_NAME)/src/dnsmasq $(DIR_ROMFS)/bin/
	cp -d $(DIR_USERS)/zyxel/$(PACKAGE_NAME)/files/etc/*  $(DIR_ROMFS)/etc/
	cp -d $(DIR_USERS)/zyxel/$(PACKAGE_NAME)/files/usr/sbin/*  $(DIR_ROMFS)/usr/sbin/
	echo "dnsmasq-2.78  do romfs done."

