
#pragma once

#define	NET_MAX_PAYLOAD				288000	// largest message we can send in bytes

#define NETMSG_TYPE_BITS	6	// must be 2^NETMSG_TYPE_BITS > SVC_LASTMSG
