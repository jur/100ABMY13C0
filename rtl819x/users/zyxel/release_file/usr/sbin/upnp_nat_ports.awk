BEGIN {
    #initial value
    #link_down=0
}
{
    split($0,a," ");
    #a[10]: protocol, a[11]: port, a[12]: destination IP with port
    #print a[10], a[11], a[12]
    printf ("iptables -t nat -A MINIUPNPD -p %s --dport %s -j DNAT --to %s\n", a[10], substr(a[11], 5), substr(a[12],4) ) > "/tmp/upnp_nat_rule.sh"
}
END {
    #print "****** upnp filter rules ******"
    close("/tmp/upnp_nat_rule.sh")
    system("chmod +x /tmp/upnp_nat_rule.sh")
}
