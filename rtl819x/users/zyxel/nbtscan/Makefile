PACKAGE_NAME =nbtscan
PKG_VERSION:=1.5.1
PKG_SOURCE:=$(PACKAGE_NAME)_$(PKG_VERSION).orig.tar.gz
PACKAGE_PATH=$(ZYXEL_BUILD_DIR)/$(PACKAGE_NAME)-$(PKG_VERSION)a

all: prepare build

prepare:

ifeq ("$(wildcard $(ZYXEL_DL)/$(PKG_SOURCE))", "")
	wget -P $(ZYXEL_DL) $(ZYXEL_SITE)/$(PKG_SOURCE)
endif

ifeq ("$(wildcard $(ZYXEL_BUILD_DIR)/$(PACKAGE_NAME))", "")
	tar zxvf $(ZYXEL_DL)/$(PKG_SOURCE) -C $(ZYXEL_BUILD_DIR)
endif

build:
	cd $(PACKAGE_PATH) && \
	if [ ! -e Makefile ]; then \
		./configure --target=mips-linux --host=mips-linux --build=i686-linux-gnu \
		#--enable-readline --enable-threadsafe --enable-dynamic-extensions \
	fi; \
	make 

clean:
	rm -rf $(DIR_USERS)/build_dir/$(PACKAGE_NAME)

romfs:
	cp -d $(PACKAGE_PATH)/$(PACKAGE_NAME) $(DIR_ROMFS)/bin/
	echo "nbtscan do romfs done."
