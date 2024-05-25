## Compile SDK
1. Compile Environment
 x64 environment need to install ia32-libs first to support compile tools
 For Ubuntu 12.04
 <em>**sudo apt-get remove bluez ia32-libs**</em>
 <em>**sudo apt-get install ia32-libs-multiarch:i386**</em>
 <em>**sudo apt-get install ia32-libs**</em>

 Use bash as default shell. select no.
 <em>**sudo dpkg-reconfigure dash**</em>

 Install usually used tools and librarys
 <em>**sudo apt-get install zlib1g-dev bison flex gawk autoconf libtool cmake lua5.1**</em>

1. Overall suggested flow for build the SDK:

```
# git clone repo
git clone git@git.zyxel.com.tw:cpe/marble.git

# change directory to git repo
cd marble

# avoid error opening terminal
sudo apt-get install ncurses-base ncurses-bin ncurses-term libncurses5 libncurses5-dev
export TERM=xterm1
export TERMINFO=/usr/share/terminfo

# Quick start (for WSR30)
cd rtl819x/
cp ../wsr30.config .config
make oldconfig

# Finally, make...
make

```

## SDK images upload
1. Upload rlxlinux image via bootloader
Reboot AP and click 'Esc' via console during AP booting, then AP enter bootloader environment, you may see "<RealTek>" prompt in the console.
Start a Tftp Client in PC, and connect Host to 192.168.1.6. Select Local File (rtl819x/image/fw.bin), and put image file(fw.bin) to TFTP server
After fw.bin file upload had finished, boot code will auto booting.

2. Upload rlxlinux image via webpage
Open AP webpage, select MANGEMENT->UPGRADE FIRMWARE browse and Select File (rtl819x/image/fw.bin), then Upload.

## vsftpd start

```
cp /etc/vsftpd.conf /var/config/vsftpd.conf
vsftpd /var/config/vsftpd.conf &
```
