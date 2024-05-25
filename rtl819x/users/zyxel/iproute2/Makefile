PACKAGE_NAME := iproute2-3.3.0
PACKAGE_PATH := $(ZYXEL_BUILD_DIR)
TAR_FILE := $(PACKAGE_NAME).tar.gz

all: prepare build

prepare:
ifneq ("$(ZYXEL_DL)/$(TAR_FILE)", "$(wildcard $(ZYXEL_DL)/$(TAR_FILE))")
	wget -P  $(ZYXEL_DL) $(ZYXEL_SITE)/$(TAR_FILE)
endif
ifneq ("$(PACKAGE_PATH)/$(PACKAGE_NAME)", "$(wildcard $(PACKAGE_PATH)/$(PACKAGE_NAME))")
	cd $(PACKAGE_PATH) && tar -xzf $(ZYXEL_DL)/$(TAR_FILE)
	cd $(PACKAGE_PATH)/$(PACKAGE_NAME) && patch -p0 < $(DIR_USERS)/zyxel/iproute2/patches/000-iproute2-3.3.0_Makefile.patch
	cd $(PACKAGE_PATH)/$(PACKAGE_NAME) && patch -p0 < $(DIR_USERS)/zyxel/iproute2/patches/001-iprote2_fix_netns_build_error.patch

endif

build:
	cd $(PACKAGE_PATH)/$(PACKAGE_NAME) && $(MAKE) all

romfs:
	cd $(PACKAGE_PATH)/$(PACKAGE_NAME) && $(MAKE) romfs

