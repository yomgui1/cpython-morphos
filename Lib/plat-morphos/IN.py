# Generated by h2py from gg:includestd/netinet/in.h
IPPROTO_IP = 0
IPPROTO_ICMP = 1
IPPROTO_IGMP = 2
IPPROTO_GGP = 3
IPPROTO_TCP = 6
IPPROTO_EGP = 8
IPPROTO_PUP = 12
IPPROTO_UDP = 17
IPPROTO_IDP = 22
IPPROTO_TP = 29
IPPROTO_EON = 80
IPPROTO_ENCAP = 98
IPPROTO_RAW = 255
IPPROTO_MAX = 256
IPPORT_RESERVED = 1024
IPPORT_USERRESERVED = 5000
def IN_CLASSA(i): return (((long)(i) & (-2147483648)) == 0)

IN_CLASSA_NET = (-16777216)
IN_CLASSA_NSHIFT = 24
IN_CLASSA_HOST = 0x00ffffff
IN_CLASSA_MAX = 128
def IN_CLASSB(i): return (((long)(i) & (-1073741824)) == (-2147483648))

IN_CLASSB_NET = (-65536)
IN_CLASSB_NSHIFT = 16
IN_CLASSB_HOST = 0x0000ffff
IN_CLASSB_MAX = 65536
def IN_CLASSC(i): return (((long)(i) & (-536870912)) == (-1073741824))

IN_CLASSC_NET = (-256)
IN_CLASSC_NSHIFT = 8
IN_CLASSC_HOST = 0x000000ff
def IN_CLASSD(i): return (((long)(i) & (-268435456)) == (-536870912))

IN_CLASSD_NET = (-268435456)
IN_CLASSD_NSHIFT = 28
IN_CLASSD_HOST = 0x0fffffff
def IN_MULTICAST(i): return IN_CLASSD(i)

def IN_EXPERIMENTAL(i): return (((long)(i) & (-268435456)) == (-268435456))

def IN_BADCLASS(i): return (((long)(i) & (-268435456)) == (-268435456))

INADDR_NONE = (-1)
IN_LOOPBACKNET = 127
IP_OPTIONS = 1
IP_HDRINCL = 2
IP_TOS = 3
IP_TTL = 4
IP_RECVOPTS = 5
IP_RECVRETOPTS = 6
IP_RECVDSTADDR = 7
IP_RETOPTS = 8
IP_MULTICAST_IF = 9
IP_MULTICAST_TTL = 10
IP_MULTICAST_LOOP = 11
IP_ADD_MEMBERSHIP = 12
IP_DROP_MEMBERSHIP = 13
IP_DEFAULT_MULTICAST_TTL = 1
IP_DEFAULT_MULTICAST_LOOP = 1
IP_MAX_MEMBERSHIPS = 20
IPPROTO_MAXID = (IPPROTO_IDP + 1)
IPCTL_FORWARDING = 1
IPCTL_SENDREDIRECTS = 2
IPCTL_DEFTTL = 3
IPCTL_DEFMTU = 4
IPCTL_MAXID = 5
