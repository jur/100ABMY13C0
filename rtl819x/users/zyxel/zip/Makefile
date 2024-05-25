PACKAGE_NAME=zip
PACKAGE_VERSION=2.32
TAR_FILE=$(PACKAGE_NAME)232.zip

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
	unzip -d $(PACKAGE_PATH)/$(PACKAGE_NAME)-$(PACKAGE_VERSION) $(ZYXEL_DL)/$(TAR_FILE)
endif

build:
ifeq ("$(wildcard, $(PACKAGE_PATH)/$(PACKAGE_NAME)-$(PACKAGE_VERSION)/usr)", "")
	$(MAKE) -C $(PACKAGE_PATH)/$(PACKAGE_NAME)-$(PACKAGE_VERSION) -f unix/Makefile generic \
	LD="$(CROSS_COMPILE)ld" \
	CC="$(CROSS_COMPILE)gcc" \
	GCC="$(CROSS_COMPILE)gcc" \
	prefix="$(PACKAGE_PATH)/$(PACKAGE_NAME)-$(PACKAGE_VERSION)/usr" \
	CFLAGS="-I. -DUNIX" \
	LDFLAGS="" \
	install
endif

# make one package clean can't use users/Makefile export value
clean:
	rm -rf $(DIR_USERS)/build_dir/$(PACKAGE_NAME)-$(PACKAGE_VERSION)

romfs:
	cp -d $(ZYXEL_BUILD_DIR)/$(PACKAGE_NAME)-$(PACKAGE_VERSION)/usr/bin/* $(DIR_ROMFS)/usr/bin/ ; \
	echo "$(PACKAGE_NAME) do romfs done."
