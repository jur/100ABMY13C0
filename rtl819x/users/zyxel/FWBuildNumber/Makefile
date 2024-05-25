# zyFWupgrade
PACKAGE_NAME = FWBuildNumber
PACKAGE_PATH = $(ZYXEL_BUILD_DIR)/$(PACKAGE_NAME)

all: prepare build

prepare:
ifneq ("$(PACKAGE_PATH)","$(wildcard $(PACKAGE_PATH))")
	mkdir -p $(PACKAGE_PATH)
	cp -rf ./source_code/* $(PACKAGE_PATH)
endif

build:
	cd $(PACKAGE_PATH) && \
	rm -f BuildNumber && \
	make

# make one package clean can't use users/Makefile export value
clean:
	rm -rf $(DIR_USERS)/build_dir/$(PACKAGE_NAME)

romfs:
	cp -d $(PACKAGE_PATH)/BuildNumber $(DIR_ROMFS)/etc/