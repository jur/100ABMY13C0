PACKAGE_NAME=libnfnetlink-1.0.0
TAR_FILE=$(PACKAGE_NAME).tar.bz2
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
	tar xvf $(ZYXEL_DL)/$(TAR_FILE)
endif


build:
	cd $(PACKAGE_PATH)/$(PACKAGE_NAME) && \
	mkdir -p  $(PACKAGE_PATH)/$(PACKAGE_NAME)/install && \
	LD="$(CROSS_COMPILE)ld" && \
	./configure --host=mips-gnu-linux --target=mips-gnu-linux --enable-static --enable-shared CFLAGS=-Os --prefix=${DIR_ROOT}/users/lib && \
	make && \
	make install
	
# make one package clean can't use users/Makefile export value
clean:
	rm -rf $(DIR_USERS)/build_dir/$(PACKAGE_NAME)

romfs:
	cp -d $(PACKAGE_PATH)/$(PACKAGE_NAME)/src/.libs/libnfnetlink.so.* $(DIR_ROMFS)/lib/
	cp -d $(PACKAGE_PATH)/$(PACKAGE_NAME)/src/.libs/libnfnetlink.so $(DIR_ROMFS)/lib/
	echo "libnfnetlink-1.0.0  do romfs done."
