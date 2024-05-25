BEGIN {
}
{
  split($0,a," "); 
  if (a[2]==1) {
    printf("zy1905App 2 %s\n", a[1] ) > "/tmp/reboot_ext.sh"
  }
}
END {
  close("/tmp/reboot_ext.sh")
  system("chmod +x /tmp/reboot_ext.sh")
}
