/* This is a generated file, don't edit */

#define NUM_APPLETS 141

const char applet_names[] ALIGN1 = ""
"acpid" "\0"
"arp" "\0"
"arping" "\0"
"ash" "\0"
"awk" "\0"
"base64" "\0"
"beep" "\0"
"blockdev" "\0"
"bootchartd" "\0"
"brctl" "\0"
"bunzip2" "\0"
"bzcat" "\0"
"cat" "\0"
"chmod" "\0"
"chown" "\0"
"cksum" "\0"
"clear" "\0"
"cp" "\0"
"crond" "\0"
"crontab" "\0"
"cut" "\0"
"date" "\0"
"df" "\0"
"dmesg" "\0"
"dnsdomainname" "\0"
"echo" "\0"
"egrep" "\0"
"env" "\0"
"expr" "\0"
"false" "\0"
"fgconsole" "\0"
"find" "\0"
"flock" "\0"
"fold" "\0"
"free" "\0"
"fsync" "\0"
"ftpd" "\0"
"getty" "\0"
"grep" "\0"
"groups" "\0"
"gunzip" "\0"
"gzip" "\0"
"halt" "\0"
"head" "\0"
"hexdump" "\0"
"hostname" "\0"
"ifconfig" "\0"
"ifplugd" "\0"
"init" "\0"
"insmod" "\0"
"ionice" "\0"
"iostat" "\0"
"ip" "\0"
"ipcalc" "\0"
"kill" "\0"
"killall" "\0"
"klogd" "\0"
"ln" "\0"
"lock" "\0"
"logger" "\0"
"login" "\0"
"logread" "\0"
"ls" "\0"
"lsmod" "\0"
"lspci" "\0"
"lsusb" "\0"
"lzop" "\0"
"lzopcat" "\0"
"md5sum" "\0"
"mdev" "\0"
"mkdir" "\0"
"mkdosfs" "\0"
"mkfs.vfat" "\0"
"modinfo" "\0"
"mount" "\0"
"mpstat" "\0"
"mv" "\0"
"nbd-client" "\0"
"nc" "\0"
"netstat" "\0"
"ntpd" "\0"
"passwd" "\0"
"pgrep" "\0"
"pidof" "\0"
"ping" "\0"
"ping6" "\0"
"pmap" "\0"
"poweroff" "\0"
"powertop" "\0"
"ps" "\0"
"pstree" "\0"
"pwdx" "\0"
"reboot" "\0"
"renice" "\0"
"rev" "\0"
"rm" "\0"
"rmmod" "\0"
"route" "\0"
"scriptreplay" "\0"
"sed" "\0"
"seq" "\0"
"setserial" "\0"
"sh" "\0"
"sha1sum" "\0"
"sha256sum" "\0"
"sha512sum" "\0"
"sleep" "\0"
"smemcap" "\0"
"sync" "\0"
"sysctl" "\0"
"syslogd" "\0"
"tail" "\0"
"tar" "\0"
"telnetd" "\0"
"timeout" "\0"
"top" "\0"
"touch" "\0"
"tr" "\0"
"true" "\0"
"tunctl" "\0"
"ubiattach" "\0"
"ubidetach" "\0"
"ubimkvol" "\0"
"ubirmvol" "\0"
"ubirsvol" "\0"
"ubiupdatevol" "\0"
"udhcpc" "\0"
"umount" "\0"
"unlzop" "\0"
"unxz" "\0"
"uptime" "\0"
"vconfig" "\0"
"vi" "\0"
"volname" "\0"
"watch" "\0"
"wc" "\0"
"wget" "\0"
"whois" "\0"
"xz" "\0"
"xzcat" "\0"
"zcat" "\0"
;

#ifndef SKIP_applet_main
int (*const applet_main[])(int argc, char **argv) = {
acpid_main,
arp_main,
arping_main,
ash_main,
awk_main,
base64_main,
beep_main,
blockdev_main,
bootchartd_main,
brctl_main,
bunzip2_main,
bunzip2_main,
cat_main,
chmod_main,
chown_main,
cksum_main,
clear_main,
cp_main,
crond_main,
crontab_main,
cut_main,
date_main,
df_main,
dmesg_main,
hostname_main,
echo_main,
grep_main,
env_main,
expr_main,
false_main,
fgconsole_main,
find_main,
flock_main,
fold_main,
free_main,
fsync_main,
ftpd_main,
getty_main,
grep_main,
id_main,
gunzip_main,
gzip_main,
halt_main,
head_main,
hexdump_main,
hostname_main,
ifconfig_main,
ifplugd_main,
init_main,
insmod_main,
ionice_main,
iostat_main,
ip_main,
ipcalc_main,
kill_main,
kill_main,
klogd_main,
ln_main,
lock_main,
logger_main,
login_main,
logread_main,
ls_main,
lsmod_main,
lspci_main,
lsusb_main,
lzop_main,
lzop_main,
md5_sha1_sum_main,
mdev_main,
mkdir_main,
mkfs_vfat_main,
mkfs_vfat_main,
modinfo_main,
mount_main,
mpstat_main,
mv_main,
nbdclient_main,
nc_main,
netstat_main,
ntpd_main,
passwd_main,
pgrep_main,
pidof_main,
ping_main,
ping6_main,
pmap_main,
halt_main,
powertop_main,
ps_main,
pstree_main,
pwdx_main,
halt_main,
renice_main,
rev_main,
rm_main,
rmmod_main,
route_main,
scriptreplay_main,
sed_main,
seq_main,
setserial_main,
ash_main,
md5_sha1_sum_main,
md5_sha1_sum_main,
md5_sha1_sum_main,
sleep_main,
smemcap_main,
sync_main,
sysctl_main,
syslogd_main,
tail_main,
tar_main,
telnetd_main,
timeout_main,
top_main,
touch_main,
tr_main,
true_main,
tunctl_main,
ubi_tools_main,
ubi_tools_main,
ubi_tools_main,
ubi_tools_main,
ubi_tools_main,
ubi_tools_main,
udhcpc_main,
umount_main,
lzop_main,
unxz_main,
uptime_main,
vconfig_main,
vi_main,
volname_main,
watch_main,
wc_main,
wget_main,
whois_main,
unxz_main,
unxz_main,
gunzip_main,
};
#endif

const uint16_t applet_nameofs[] ALIGN2 = {
0x0000,
0x0006,
0x000a,
0x0011,
0x2015,
0x0019,
0x0020,
0x0025,
0x002e,
0x0039,
0x003f,
0x0047,
0x304d,
0x2051,
0x2057,
0x205d,
0x0063,
0x2069,
0x006c,
0x8072,
0x207a,
0x007e,
0x0083,
0x0086,
0x008c,
0x309a,
0x009f,
0x20a5,
0x00a9,
0x30ae,
0x00b4,
0x20be,
0x00c3,
0x20c9,
0x00ce,
0x30d3,
0x00d9,
0x00de,
0x00e4,
0x20e9,
0x00f0,
0x00f7,
0x00fc,
0x2101,
0x2106,
0x010e,
0x0117,
0x0120,
0x0128,
0x012d,
0x0134,
0x013b,
0x0142,
0x0145,
0x014c,
0x0151,
0x0159,
0x215f,
0x0162,
0x0167,
0x816e,
0x0174,
0x217c,
0x017f,
0x0185,
0x018b,
0x0191,
0x0196,
0x219e,
0x01a5,
0x31aa,
0x01b0,
0x01b8,
0x01c2,
0x01ca,
0x01d0,
0x01d7,
0x01da,
0x01e5,
0x01e8,
0x01f0,
0x81f5,
0x01fc,
0x0202,
0x4208,
0x420d,
0x0213,
0x0218,
0x0221,
0x022a,
0x022d,
0x0234,
0x0239,
0x0240,
0x0247,
0x324b,
0x024e,
0x0254,
0x025a,
0x0267,
0x326b,
0x026f,
0x0279,
0x227c,
0x2284,
0x228e,
0x0298,
0x029e,
0x32a6,
0x02ab,
0x02b2,
0x02ba,
0x02bf,
0x02c3,
0x02cb,
0x02d3,
0x32d7,
0x02dd,
0x32e0,
0x02e5,
0x02ec,
0x02f6,
0x0300,
0x0309,
0x0312,
0x031b,
0x0328,
0x032f,
0x0336,
0x033d,
0x0342,
0x0349,
0x0351,
0x0354,
0x035c,
0x0362,
0x0365,
0x036a,
0x0370,
0x0373,
0x0379,
};


#define MAX_APPLET_NAME_LEN 13
