all:


romfs:
	@ln -s /usr/bin/zyxel_led_ctrl $(DIR_ROMFS)/sbin/zyxel_led_ctrl
	@cp -d $(DIR_ROOT)/users/json-c-0.12/.libs/*.so*  $(DIR_ROMFS)/lib/
	@cp -d $(DIR_ROOT)/users/expat-2.1.0/.libs/*.so*  $(DIR_ROMFS)/lib/
	@cp -d $(DIR_ROOT)/users/bluez-5.24/lib/.libs/libbluetooth.so* $(DIR_ROMFS)/lib/
	@cp -d $(DIR_ROOT)/users/glib-2.36.0/glib/.libs/libglib-2.0.so* $(DIR_ROMFS)/lib/
	@cp -d bin/* $(DIR_ROMFS)/bin/
	@cp -d sbin/* $(DIR_ROMFS)/sbin/
	@cp -drf lib/* $(DIR_ROMFS)/lib/
	@cp -drf usr/* $(DIR_ROMFS)/usr/
	@cp -drf etc/* $(DIR_ROMFS)/etc/
	-cp -drf www $(DIR_ROMFS)/www/

	#@cp -drf share/* $(DIR_ROMFS)/usr/share/
	#@ln -s /usr/share/ca-certs/rootCA.crt $(DIR_ROMFS)/usr/share/ca-certs/cc698c33.0
