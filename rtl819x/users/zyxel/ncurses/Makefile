PACKAGE_NAME=ncurses-6.0
TAR_FILE=$(PACKAGE_NAME).tar.gz
PACKAGE_PATH=$(ZYXEL_BUILD_DIR)

all: prepare build

prepare:
ifneq ("$(ZYXEL_DL)/$(TAR_FILE)","$(wildcard $(ZYXEL_DL)/$(TAR_FILE))")
	# download source code
	wget -P $(ZYXEL_DL) $(ZYXEL_SITE)/$(TAR_FILE)
endif
ifneq ("$(PACKAGE_PATH)/$(PACKAGE_NAME)","$(wildcard $(PACKAGE_PATH)/$(PACKAGE_NAME))")
	# prepare build_dir source code
	cd $(PACKAGE_PATH) && \
	tar xzvf $(ZYXEL_DL)/$(TAR_FILE)
endif


build:
	cd $(PACKAGE_PATH)/$(PACKAGE_NAME) && \
	./configure --prefix=${DIR_ROOT}/users/lib --host=mips-linux --with-shared && \
	make && \
	make install

# make one package clean can't use users/Makefile export value
clean:
	rm -rf $(DIR_USERS)/build_dir/$(PACKAGE_NAME)

romfs:
	install -d -m0755 $(DIR_ROMFS)/usr/share/terminfo/v
	install -m0644 ${DIR_ROOT}/users/lib/share/terminfo/v/vt102 $(DIR_ROMFS)/usr/share/terminfo/v/vt102
	install -d -m0755 $(DIR_ROMFS)/usr/share/terminfo/x
	install -m0644 ${DIR_ROOT}/users/lib/share/terminfo/x/xterm $(DIR_ROMFS)/usr/share/terminfo/x/xterm
	install -m755 ${DIR_ROOT}/users/lib/lib/libncurses.so.6.0 $(DIR_ROMFS)/lib/libncurses.so.6.0
	ln -snf libncurses.so.6.0 $(DIR_ROMFS)/lib/libncurses.so.6
	ln -snf libncurses.so.6 $(DIR_ROMFS)/lib/libncurses.so
	echo "ncurses do romfs done."
