/*
 * Linux 80211 wrapper functions
 * Copyright (c) 2013-2014, Mengning <mengning@ustc.edu.cn>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <string.h>
#include "includes.h"
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <net/if.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/family.h>
#include <netlink/genl/ctrl.h>
#include <linux/rtnetlink.h>
#include <netpacket/packet.h>
#include <linux/filter.h>
#include <linux/errqueue.h>
#include "nl80211_copy.h"

#include "common.h"
#include "eloop.h"
#include "utils/list.h"
#include "common/ieee802_11_defs.h"
#include "common/ieee802_11_common.h"
#include "l2_packet/l2_packet.h"
#include "netlink.h"
#include "linux_ioctl.h"
#include "radiotap.h"
#include "radiotap_iter.h"
#include "rfkill.h"
#include "driver.h"

#include "linux_80211_wrapper.h"


#ifdef CONFIG_LIBNL20
/* libnl 2.0 compatibility code */
#define nl_handle nl_sock
#define nl80211_handle_alloc nl_socket_alloc_cb
#define nl80211_handle_destroy nl_socket_free
#else
/*
 * libnl 1.1 has a bug, it tries to allocate socket numbers densely
 * but when you free a socket again it will mess up its bitmap and
 * and use the wrong number the next time it needs a socket ID.
 * Therefore, we wrap the handle alloc/destroy and add our own pid
 * accounting.
 */
static uint32_t port_bitmap[32] = { 0 };
int server_sockfd, client_sockfd;
int server_len, client_len;
struct sockaddr_in server_address;
struct sockaddr_in client_address;

static struct nl_handle *nl80211_handle_alloc(void *cb)
{
	struct nl_handle *handle;
	uint32_t pid = getpid() & 0x3FFFFF;
	int i;

	handle = nl_handle_alloc_cb(cb);

	for (i = 0; i < 1024; i++) {
		if (port_bitmap[i / 32] & (1 << (i % 32)))
			continue;
		port_bitmap[i / 32] |= 1 << (i % 32);
		pid += i << 22;
		break;
	}

	nl_socket_set_local_port(handle, pid);

	return handle;
}

static void nl80211_handle_destroy(struct nl_handle *handle)
{
	uint32_t port = nl_socket_get_local_port(handle);

	port >>= 22;
	port_bitmap[port / 32] &= ~(1 << (port % 32));

	nl_handle_destroy(handle);
}
#endif /* CONFIG_LIBNL20 */

struct nl_handle * nl_create_handle(struct nl_cb *cb, const char *dbg)
{
	printf("nl_create_handle\n");
	struct nl_handle *handle;
	handle = nl80211_handle_alloc(cb);                                                   
	if (handle == NULL) {
		wpa_printf(MSG_ERROR, "nl80211: Failed to allocate netlink "
			   "callbacks (%s)", dbg);
		return NULL;
	}
	
	if (genl_connect(handle)) {
		wpa_printf(MSG_ERROR, "nl80211: Failed to connect to generic "
			   "netlink (%s)", dbg);
		nl80211_handle_destroy(handle);
		return NULL;
	}

	return handle;
}


void nl_destroy_handles(struct nl_handle **handle)
{
	printf("nl_destroy_handles\n");
	if (*handle == NULL)
		return;
	nl80211_handle_destroy(*handle);
	*handle = NULL;
}

int nl_send_wrapper(struct nl_handle *handle, struct nl_msg *msg)
{   
    return nl_send_auto_complete(handle, msg);
}

int nl_recv_wrapper(struct nl_handle *handle, struct nl_cb *cb)
{
    return nl_recvmsgs(handle,cb);
}

int epoll_wrapper(int sock, eloop_sock_handler handler,
			     void *eloop_data, void *user_data)
{
    return eloop_register_read_sock(sock,handler,eloop_data,user_data);
}

int socket_wrapper(int domain, int type, int protocol)
{
	printf("socket\n");

        server_sockfd = socket(AF_INET, SOCK_STREAM, 0);
        server_address.sin_family = AF_INET;
        server_address.sin_addr.s_addr = htonl(INADDR_ANY);
        server_address.sin_port = htons(1234);
        server_len = sizeof(server_address);
        bind(server_sockfd, (struct sockaddr *)&server_address, server_len);
        listen(server_sockfd, 5);
        
        printf("server waiting\n");
        client_len = sizeof(client_address);
	printf("socket accept \n");
	client_sockfd = accept(server_sockfd, (struct sockaddr *)&client_address, &client_len);
       /* client_sockfd = accept(server_sockfd, (struct sockaddr *)&client_address, &client_len);
        read(client_sockfd, &ch, 1);
        ch++;
        write(client_sockfd, &ch, 1);
		sleep(1);
        close(client_sockfd);*/
	return socket(domain, type, protocol);
}

int recvmsg_wrapper(int sock, struct msghdr *msg, int flags)
{
	printf("recvmsg\n");
	return recvmsg(sock, msg, flags);
}

int sendto_wrapper(int sock, const void *msg, int len, unsigned int flags,
	const struct sockaddr *to, int tolen)
{
	char ch;
	char *a = msg;
	read(client_sockfd, &ch, 1);
   	printf("%c\n", ch);
	printf("len = %d\n", strlen(msg));
        write(client_sockfd, msg, strlen(msg));
	close(client_sockfd);
	printf("socket sendto %s\n", msg);
	printf("socket sendto %x\n", *a);
	printf("sendto\n");
	return sendto(sock, msg, len, flags, to, tolen);
}
int recvfrom_wrapper(int sock, void *buf, int len, unsigned int lags,
	struct sockaddr *from, int *fromlen)
{
	printf("recvfrom\n");
	return recvfrom(sock, buf, len, lags, from, fromlen);
}

