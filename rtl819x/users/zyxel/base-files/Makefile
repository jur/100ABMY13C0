# copy base-files
PACKAGE_NAME=base-files
PRODUCT=$(subst PROJNAME_,,$(PROJ_NAME))
# $(subst from,to,text) Performs replacement on the 'text': each occurrence of 'from' is replaced by 'to'.

all:
	@echo "base-files done"

romfs:
ifneq ($(wildcard ./platform-dependency-files/$(PRODUCT)),)
	cp -r ./platform-dependency-files/$(PRODUCT)/* $(DIR_ROMFS)/
	$(ROMFSINST) -s /var/hosts /etc/hosts
	echo "platform-dependency-files $(PRODUCT) do romfs done."
endif
	cp -r ./files/* $(DIR_ROMFS)/
	echo "base-files do romfs done."
