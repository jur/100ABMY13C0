PACKAGE_NAME = sqlite-autoconf-3071201
TAR_FILE=$(PACKAGE_NAME).tar.gz
PACKAGE_PATH=$(ZYXEL_BUILD_DIR)/$(PACKAGE_NAME)

all: prepare build

prepare:
	
ifeq ("$(wildcard $(ZYXEL_DL)/$(PACKAGE_NAME).tar.gz)", "")
	wget -P $(ZYXEL_DL) $(ZYXEL_SITE)/$(TAR_FILE)
endif

ifeq ("$(wildcard $(ZYXEL_BUILD_DIR)/$(PACKAGE_NAME))", "")
	tar zxvf $(ZYXEL_DL)/$(PACKAGE_NAME).tar.gz -C $(ZYXEL_BUILD_DIR)
endif

build:
	cd $(PACKAGE_PATH) && \
	if [ ! -e Makefile ]; then \
		patch -p1 < $(DIR_USERS)/zyxel/sqlite-autoconf-3071201/patches/100-sqlite-romfs.patch; \
		LD=$(CROSS_COMPILE)ld\
		CC="$(CROSS_COMPILE)gcc" \
		GCC="$(CROSS_COMPILE)gcc" \
		CFLAGS="-I$(DIR_USERS)/lib/include/" LIBS="-L$(DIR_USERS)/lib/lib/" \
		./configure --target=mips-linux --host=mips-linux --build=i686-linux-gnu \
		#--enable-readline --enable-threadsafe --enable-dynamic-extensions \
	fi; \
	make && \
	mkdir -p $(DIR_USERS)/lib/include/sqlite/;
	cd $(PACKAGE_PATH) && \
	cp sqlite3.h $(DIR_USERS)/lib/include/sqlite && \
	cp .libs/*.so $(DIR_USERS)/lib/lib/ && \
	cp .libs/*.so.* $(DIR_USERS)/lib/lib/

clean:
	rm -rf $(DIR_USERS)/build_dir/$(PACKAGE_NAME)

romfs:
	cp -d $(PACKAGE_PATH)/.libs/*.so $(DIR_ROMFS)/lib/
	cp -d $(PACKAGE_PATH)/.libs/*.so* $(DIR_ROMFS)/lib/
