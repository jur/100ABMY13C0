BEGIN
{
    #initial value
    link_down=0
}
{
    split($0,a," ");
    #a[2] is last-seen field
    if (a[2] *10 > 80)
    {
        #print a[1] " is disconnected"
        link_down++
        #print "-->link down",link_count
    }
    #print a[2], a[1]
}
END {
    #print NR
    #print "-->link down", link_count
    if (link_down == NR)
    {
        print "ALL_DOWN"
    }
    else
    {
        print "Connected"
    }
}

