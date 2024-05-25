
PACKAGE_NAME=libmesode-master
TAR_FILE=libmesode-master.tar.gz
# SHAREDLIB=libzy_common_util.so
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

ifneq ("$(PACKAGE_PATH)/$(PACKAGE_NAME)","$(wildcard $(PACKAGE_PATH)/$(PACKAGE_NAME))")
	# prepare build_dir source code
	cd $(PACKAGE_PATH) && \
	tar zxvf $(ZYXEL_DL)/$(TAR_FILE)
endif

build:
	cd $(PACKAGE_PATH)/libmesode-master;  \
	if [ ! -e configure ]; then \
		./bootstrap.sh; \
	fi; \
	if [ ! -e Makefile ]; then \
		LD="$(CROSS_COMPILE)ld" \
		CFLAGS="-I$(DIR_USERS)/lib/include/" LDFLAGS="-L$(DIR_USERS)/lib/lib/" \
		openssl_CFLAGS="-I$(DIR_USERS)/lib/include" openssl_LIBS="-L$(DIR_USERS)/lib/lib -lssl" \
		expat_CFLAGS="-I$(DIR_USERS)/lib/include" expat_LIBS="-L$(DIR_USERS)/lib/lib" \
		./configure --target=mips-linux --host=mips-linux --build=i686-linux-gnu; \
	fi; \
	patch -p1 -N < $(DIR_USERS)/zyxel/mesode/patches/101-mesode-removeBuildExample.patch; \
	make; \
	cp -d .libs/libmesode.so* $(DIR_USERS)/lib/lib/; \
	cp mesode.h $(DIR_USERS)/lib/include/; 

# make one package clean can't use users/Makefile export value
clean:
	rm -rf $(DIR_USERS)/build_dir/$(PACKAGE_NAME)

romfs:

	cp -d $(PACKAGE_PATH)/libmesode-master/.libs/libmesode.so* $(DIR_ROMFS)/lib/
	echo "zy-common-util do romfs done."
