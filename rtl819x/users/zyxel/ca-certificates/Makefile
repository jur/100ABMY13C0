PACKAGE_NAME = ca-certificates
PACKAGE_VERSION = 20210119
PACKAGE_PATH=$(ZYXEL_BUILD_DIR)/work


all: prepare build

prepare:
	# download ca-certificates_20180409 source package
ifeq ("$(wildcard $(ZYXEL_DL)/$(PACKAGE_NAME)_$(PACKAGE_VERSION).tar.xz)", "")
	wget -P $(ZYXEL_DL)/ $(ZYXEL_SITE)/$(PACKAGE_NAME)_$(PACKAGE_VERSION).tar.xz;
endif

ifeq ("$(wildcard $(ZYXEL_BUILD_DIR))", "")
	mkdir -p $(ZYXEL_BUILD_DIR);
endif

ifeq ("$(wildcard $(PACKAGE_PATH))", "")
	tar Jxvf $(ZYXEL_DL)/$(PACKAGE_NAME)_$(PACKAGE_VERSION).tar.xz -C $(ZYXEL_BUILD_DIR)/
endif

build:
	cd $(PACKAGE_PATH) && \
	make

clean:
	rm -rf $(PACKAGE_PATH)

romfs:
ifneq ("$(wildcard $(DIR_ROMFS)/etc/ssl/certs/)", "")
	rm -rf $(DIR_ROMFS)/etc/ssl/certs/
endif
	
	mkdir -p $(DIR_ROMFS)/etc/ssl/certs/
	cp -d $(PACKAGE_PATH)/mozilla/*.crt $(DIR_ROMFS)/etc/ssl/certs/

	./link_certs.sh $(DIR_ROMFS)

	chmod +x $(ZYXEL_BUILD_DIR)/openssl-1.0.2h.shared/tools/c_rehash
	$(ZYXEL_BUILD_DIR)/openssl-1.0.2h.shared/tools/c_rehash $(DIR_ROMFS)/etc/ssl/certs/

	cat $(PACKAGE_PATH)/mozilla/*.crt > $(DIR_ROMFS)/etc/ssl/certs/ca-certificates.crt

ifneq ("$(wildcard $(DIR_ROMFS)/etc/ssl/cert.pem)", "")
	rm $(DIR_ROMFS)/etc/ssl/cert.pem
endif

	ln $(DIR_ROMFS)/etc/ssl/certs/ca-certificates.crt $(DIR_ROMFS)/etc/ssl/cert.pem
