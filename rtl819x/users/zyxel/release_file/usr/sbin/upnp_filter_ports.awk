BEGIN {
    #initial value
    #link_down=0
}
{
    split($0,a," ");
    #a[9]: destination IP, a[10]: protocol, a[11]: destination port
    #print a[9], a[10], a[11]
    printf ("iptables -t filter -A MINIUPNPD -p %s -d %s --dport %s -j ACCEPT\n", a[10], a[9], substr(a[11], 5) ) > "/tmp/upnp_filter_rule.sh"
}
END {
    #print "****** upnp filter rules ******"
    close("/tmp/upnp_filter_rule.sh")
    system("chmod +x /tmp/upnp_filter_rule.sh")
}
