-include ../../.config

ifndef ZYXEL_DL
ZYXEL_DL := $(DIR_USERS)/dl
endif
ifndef PACKAGE_PATH
PACKAGE_PATH := $(DIR_USERS)/build_dir
endif
ifndef DIR_ROMFS
DIR_ROMFS := $(DIR_USERS)/../target/image
endif
ifndef ZYXEL_SITE
ZYXEL_SITE := http://172.21.83.131/file/WIFI_DL
endif
ifndef ZYXEL_PATH
ZYXEL_PATH := $(DIR_USERS)/zyxel
endif
ifndef ZYXEL_BUILD_DIR
ZYXEL_BUILD_DIR := $(DIR_USERS)/build_dir
endif

ifdef CONFIG_BATMAN_ADV_2018_3
BATMAN_VERSION := 2018.3
endif
ifdef CONFIG_BATMAN_ADV_2018_0
BATMAN_VERSION := 2018.0
endif
ifdef CONFIG_BATMAN_ADV_2017_1
BATMAN_VERSION := 2017.1
endif

PACKAGE_NAME := batman-adv-$(BATMAN_VERSION)
PATCHES_NAME := patches-V$(BATMAN_VERSION)
BATMAN_TAR_FILE := $(PACKAGE_NAME).tar.gz
BATMAN_TARGET_FILE := batman-adv.ko
PACKAGE_PATH := $(ZYXEL_BUILD_DIR)

CFLAGS := -Os $(CFLAGS)

all: prepare build install

prepare:
ifneq ("$(ZYXEL_DL)/$(BATMAN_TAR_FILE)","$(wildcard $(ZYXEL_DL)/$(BATMAN_TAR_FILE))")
	#download
	wget -P $(ZYXEL_DL) $(ZYXEL_SITE)/$(BATMAN_TAR_FILE)
endif
ifneq ("$(PACKAGE_PATH)/$(PACKAGE_NAME)","$(wildcard $(PACKAGE_PATH)/$(PACKAGE_NAME))")
	cd $(PACKAGE_PATH) && tar -xzf $(ZYXEL_DL)/$(BATMAN_TAR_FILE)
	cd $(PACKAGE_PATH)/$(PACKAGE_NAME) && cp $(DIR_USERS)/zyxel/batman-adv/$(PATCHES_NAME)/etherdevice.h compat-include/linux
	cd $(PACKAGE_PATH)/$(PACKAGE_NAME) && cp $(DIR_USERS)/zyxel/batman-adv/$(PATCHES_NAME)/genetlink.h compat-include/net
	cd $(PACKAGE_PATH)/$(PACKAGE_NAME) && patch -p0 < $(DIR_USERS)/zyxel/batman-adv/$(PATCHES_NAME)/000-fix-BANTMAN-build-error.patch
	cd $(PACKAGE_PATH)/$(PACKAGE_NAME) && patch -p0 < $(DIR_USERS)/zyxel/batman-adv/$(PATCHES_NAME)/001-default_enable_BATMAN_V.patch
	cd $(PACKAGE_PATH)/$(PACKAGE_NAME) && patch -p0 < $(DIR_USERS)/zyxel/batman-adv/$(PATCHES_NAME)/002-grab_ether_and_wifi_status.patch
	cd $(PACKAGE_PATH)/$(PACKAGE_NAME) && patch -p1 < $(DIR_USERS)/zyxel/batman-adv/$(PATCHES_NAME)/006-append_fastbat_module.patch
	cd $(PACKAGE_PATH)/$(PACKAGE_NAME) && cp $(DIR_USERS)/zyxel/batman-adv/$(PATCHES_NAME)/006-fast_batman.h $(PACKAGE_PATH)/$(PACKAGE_NAME)/net/batman-adv/fast_batman.h
	cd $(PACKAGE_PATH)/$(PACKAGE_NAME) && cp $(DIR_USERS)/zyxel/batman-adv/$(PATCHES_NAME)/006-fast_batman.c $(PACKAGE_PATH)/$(PACKAGE_NAME)/net/batman-adv/fast_batman.c
	cd $(PACKAGE_PATH)/$(PACKAGE_NAME) && patch -p1 < $(DIR_USERS)/zyxel/batman-adv/$(PATCHES_NAME)/007-del_local_entry.patch
	cd $(PACKAGE_PATH)/$(PACKAGE_NAME) && patch -p1 < $(DIR_USERS)/zyxel/batman-adv/$(PATCHES_NAME)/008-work_queue_to_clean_up_dirty_entries.patch
	cd $(PACKAGE_PATH)/$(PACKAGE_NAME) && patch -p1 < $(DIR_USERS)/zyxel/batman-adv/$(PATCHES_NAME)/009-fwd_packets_between_batman_origs.patch
	cd $(PACKAGE_PATH)/$(PACKAGE_NAME) && patch -p1 < $(DIR_USERS)/zyxel/batman-adv/$(PATCHES_NAME)/010-append_sysctl_variable_of_fastmesh.patch
	cd $(PACKAGE_PATH)/$(PACKAGE_NAME) && patch -p1 < $(DIR_USERS)/zyxel/batman-adv/$(PATCHES_NAME)/013-purge_dirty_entries_by_delayed_queue.patch
	cd $(PACKAGE_PATH)/$(PACKAGE_NAME) && patch -p1 < $(DIR_USERS)/zyxel/batman-adv/$(PATCHES_NAME)/015-push_batman_pace.patch
	cd $(PACKAGE_PATH)/$(PACKAGE_NAME) && patch -p1 < $(DIR_USERS)/zyxel/batman-adv/$(PATCHES_NAME)/014-append_hook_callback_when_disconnect.patch
	cd $(PACKAGE_PATH)/$(PACKAGE_NAME) && patch -p1 < $(DIR_USERS)/zyxel/batman-adv/$(PATCHES_NAME)/016-append_global_and_local_counters.patch
	cd $(PACKAGE_PATH)/$(PACKAGE_NAME) && patch -p1 < $(DIR_USERS)/zyxel/batman-adv/$(PATCHES_NAME)/017-modify_to_collect_orig_info.patch
	cd $(PACKAGE_PATH)/$(PACKAGE_NAME) && patch -p1 < $(DIR_USERS)/zyxel/batman-adv/$(PATCHES_NAME)/018-bypass_guest_lan.patch
	cd $(PACKAGE_PATH)/$(PACKAGE_NAME) && patch -p1 < $(DIR_USERS)/zyxel/batman-adv/$(PATCHES_NAME)/019-fix-sometimes-global-table-not-update.patch
	cd $(PACKAGE_PATH)/$(PACKAGE_NAME) && patch -p1 < $(DIR_USERS)/zyxel/batman-adv/$(PATCHES_NAME)/021-tcp_and_udp_checksum_which_from_DUT.patch
	cd $(PACKAGE_PATH)/$(PACKAGE_NAME) && patch -p1 < $(DIR_USERS)/zyxel/batman-adv/$(PATCHES_NAME)/022-fix_dismission_of_global_record_on_guest_wifi.patch
	cd $(PACKAGE_PATH)/$(PACKAGE_NAME) && patch -p1 < $(DIR_USERS)/zyxel/batman-adv/$(PATCHES_NAME)/024-fix_connection_from_wlan0_to_wlan2_by_dhcp_pkt.patch
#	cd $(PACKAGE_PATH)/$(PACKAGE_NAME) && patch -p1 < $(DIR_USERS)/zyxel/batman-adv/$(PATCHES_NAME)/025-fix_err_msg_when_get_zero_wifi_throughput.patch
#	cd $(PACKAGE_PATH)/$(PACKAGE_NAME) && patch -p1 < $(DIR_USERS)/zyxel/batman-adv/$(PATCHES_NAME)/026-remove_configuration_of_bypass_guest_wifi.patch
	cd $(PACKAGE_PATH)/$(PACKAGE_NAME) && patch -p0 < $(DIR_USERS)/zyxel/batman-adv/$(PATCHES_NAME)/027-Modify_bat0_mtu.patch
	cd $(PACKAGE_PATH)/$(PACKAGE_NAME) && patch -p1 < $(DIR_USERS)/zyxel/batman-adv/$(PATCHES_NAME)/028-bury_fastmesh_debug_msg.patch
	cd $(PACKAGE_PATH)/$(PACKAGE_NAME) && patch -p0 < $(DIR_USERS)/zyxel/batman-adv/$(PATCHES_NAME)/029-shrink_wireless_bcast_pkt_number.patch
	cd $(PACKAGE_PATH)/$(PACKAGE_NAME) && patch -p1 < $(DIR_USERS)/zyxel/batman-adv/$(PATCHES_NAME)/030-Apply_Force_mac_header_to_start_of_data_on_xmit.patch
	cd $(PACKAGE_PATH)/$(PACKAGE_NAME) && patch -p1 < $(DIR_USERS)/zyxel/batman-adv/$(PATCHES_NAME)/031-Apply_17835_17836_17837_Reduce_claim_hash_refcnt_only_for_removed_entry.patch
	cd $(PACKAGE_PATH)/$(PACKAGE_NAME) && patch -p1 < $(DIR_USERS)/zyxel/batman-adv/$(PATCHES_NAME)/032-fix_client_can_grab_correct_orig.patch
	cd $(PACKAGE_PATH)/$(PACKAGE_NAME) && patch -p1 < $(DIR_USERS)/zyxel/batman-adv/$(PATCHES_NAME)/033-suppress_excessive_error_msg_under_fastmesh.patch
	cd $(PACKAGE_PATH)/$(PACKAGE_NAME) && patch -p1 < $(DIR_USERS)/zyxel/batman-adv/$(PATCHES_NAME)/034-take_off_looping_avoidance.patch
	cd $(PACKAGE_PATH)/$(PACKAGE_NAME) && patch -p1 < $(DIR_USERS)/zyxel/batman-adv/$(PATCHES_NAME)/035-fix_elp_handle_func.patch
	cd $(PACKAGE_PATH)/$(PACKAGE_NAME) && patch -p1 < $(DIR_USERS)/zyxel/batman-adv/$(PATCHES_NAME)/036-fix_bug_when_changing_iface.patch
	cd $(PACKAGE_PATH)/$(PACKAGE_NAME) && patch -p1 < $(DIR_USERS)/zyxel/batman-adv/$(PATCHES_NAME)/037-replace_mutex_with_rwlock.patch
	cd $(PACKAGE_PATH)/$(PACKAGE_NAME) && patch -p1 < $(DIR_USERS)/zyxel/batman-adv/$(PATCHES_NAME)/038-clear_all_fastmesh_record.patch
endif

build:
	cd $(PACKAGE_PATH)/$(PACKAGE_NAME) && $(MAKE) CONFIG_BATMAN_ADV_DEBUGFS=y CONFIG_BATMAN_ADV_DEBUG=n CONFIG_BATMAN_ADV_BLA=y CONFIG_BATMAN_ADV_DAT=y CONFIG_BATMAN_ADV_NC=n CONFIG_BATMAN_ADV_MCAST=y CONFIG_BATMAN_ADV_BATMAN_V=y KERNELPATH=$(DIR_LINUX)

romfs: all
	mkdir -p $(DIR_ROMFS)/lib/modules/3.10.90/updates/net/batman-adv/
#	cp $(PACKAGE_PATH)/$(PACKAGE_NAME)/build/net/batman-adv/$(BATMAN_TARGET_FILE) $(DIR_ROMFS)/lib/modules/3.10.90/updates/net/batman-adv/
	cp $(PACKAGE_PATH)/$(PACKAGE_NAME)/net/batman-adv/$(BATMAN_TARGET_FILE) $(DIR_ROMFS)/lib/modules/3.10.90/updates/net/batman-adv/

clean:
	#$(MAKE) clean
	cd $(PACKAGE_PATH) && $(RM) $(PACKAGE_PATH)/$(PACKAGE_NAME)

.PHONY: install all clean romfs build prepare romfs

