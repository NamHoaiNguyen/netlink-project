#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <net/if.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <arpa/inet.h>

int open_netlink()
{
	struct sockaddr_nl saddr;

	int sock = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);

	if (sock < 0) {
		printf("Failed to open netlink socket.\n");
		return -1;
	}

	memset(&saddr, 0, sizeof(saddr));

	saddr.nl_family = AF_NETLINK;
	saddr.nl_pid = getpid();

	if (bind(sock, (struct sockaddr *)&saddr, sizeof(saddr)) < 0) {
		printf("Failed to bind to netlink socket.\n");
		close(sock);
		return -1;
	}

	return sock;
}

int send_get_route_request(int sock)
{
	struct {
		struct nlmsghdr nlh;
		struct rtmsg rtm;
	}nl_request;

	nl_request.nlh.nlmsg_type = RTM_GETROUTE;
	nl_request.nlh.nlmsg_flags = NLM_F_REQUEST | NLM_F_DUMP;
	nl_request.nlh.nlmsg_len = sizeof(nl_request);
	nl_request.nlh.nlmsg_seq = time(NULL);
	nl_request.rtm.rtm_family = AF_INET;

	return send(sock, &nl_request, sizeof(nl_request),0 );
}

int rtnl_receive(int fd, struct msghdr *msg, int flags)
{
    int len;

    do { 
        len = recvmsg(fd, msg, flags);
    } while (len < 0 && (errno == EINTR || errno == EAGAIN));

    if (len < 0) {
        perror("Netlink receive failed");
        return -errno;
    }

    if (len == 0) { 
        perror("EOF on netlink");
        return -ENODATA;
    }

    return len;
}

static int rtnl_recvmsg(int fd, struct msghdr *msg, char **answer)
{
    struct iovec *iov = msg->msg_iov;
    char *buf;
    int len;

    iov->iov_base = NULL;
    iov->iov_len = 0;

    len = rtnl_receive(fd, msg, MSG_PEEK | MSG_TRUNC);

    if (len < 0) {
        return len;
    }

    buf = malloc(len);

    if (!buf) {
        perror("malloc failed");
        return -ENOMEM;
    }

    iov->iov_base = buf;
    iov->iov_len = len;

    len = rtnl_receive(fd, msg, 0);

    if (len < 0) {
        free(buf);
        return len;
    }

    *answer = buf;

    return len;
}

void parse_rtattr(struct rtattr *tb[], int max, struct rtattr *rta, int len)
{
	memset(tb, 0, sizeof(struct rtattr *) * sizeof(max + 1));

	while (RTA_OK(rta, len)) {
		if (rta->rta_type <= max) {
			tb[rta->rta_type] = rta;
		}
		rta = RTA_NEXT(rta, len);
	}
}

void print_route(struct nlmsghdr *nl_header)
{
	struct rtmsg *r;
	int len;
	struct rtattr *tb[RTA_MAX + 1];
	int table;
	char buf[256];

	len = nl_header->nlmsg_len;
	r = (struct rtmsg *)NLMSG_DATA(nl_header);
	len -= NLMSG_LENGTH(sizeof(*r));

	if (len < 0) {
		printf("Wrong message length");
		return;
	}

	parse_rtattr(tb, RTA_MAX, RTM_RTA(r), len);

	if (r->rtm_family != AF_INET) {
		return;
	}

	if (tb[RTA_DST]) {
		printf("%s\t", inet_ntop(r->rtm_family, RTA_DATA(tb[RTA_DST]), buf, sizeof(buf)));
	} else if (r->rtm_dst_len) {
		printf("0/%u, r->rtm_dts_len");
	} else {
		printf("default         ");
	}

	if (tb[RTA_GATEWAY]) {
		printf("      %s\t", inet_ntop(r->rtm_family, RTA_DATA(tb[RTA_GATEWAY]), buf, sizeof(buf)));
    }

	if (tb[RTA_OIF]) {
		char if_nam_buf[IF_NAMESIZE];
		int ifidx = *(__u32 *)RTA_DATA(tb[RTA_OIF]);

		printf("%s\t", if_indextoname(ifidx, if_nam_buf));
	}

	if (tb[RTA_PRIORITY]) {
		char if_nam_buf[IF_NAMESIZE];
		int ifidx = *(__u32 *)RTA_DATA(tb[RTA_PRIORITY]);
		printf("      %d", ifidx);
	}

    	if (tb[RTA_SRC]) {
        	printf("%s", inet_ntop(r->rtm_family, RTA_DATA(tb[RTA_SRC]), buf, sizeof(buf)));
    	}

    printf("\n");
}

int get_route_response(int sock)
{
	struct sockaddr_nl nladdr;
	struct iovec iov;
	struct msghdr msg;
	char *buf;
	struct nlmsghdr *h; 
	int status, len;

	msg.msg_name = &nladdr;
	msg.msg_namelen = sizeof(nladdr);
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;

	char test[8192];
	iov.iov_base = test;
	iov.iov_len = sizeof(test);

//	len = recvmsg(sock, &msg, MSG_PEEK | MSG_TRUNC);
	status = rtnl_recvmsg(sock, &msg, &buf);
	h = (struct nlmsghdr *)buf;
	len = status;

	if (len < 0) {
		printf("Netlink receive failed.\n");
		return -1;
	}

	if (len == 0) {
		printf("EOF on netlink");
		return -1;
	}

	printf("Main routing table IPV4.\n");
	printf("Destination\t      Gateway\t        Iface\t      Metric\n");
	printf("Gia tri cua len : %d\n", len);

	for (h = (struct nlmsghdr *)buf; NLMSG_OK(h, len); NLMSG_NEXT(h, len)) {
		
//	while (NLMSG_OK(h, len)) {
	
		if (nladdr.nl_pid != 0) {
			continue;
		}

		if (h->nlmsg_type == NLMSG_ERROR) {
			printf("Netlink report error.\n");
			free(buf);
		}
		print_route(h);
		h = NLMSG_NEXT(h, len);
	}
	free(buf);

	return len;
}

int get_routing_table()
{
	 int sock_file, get_req;

        sock_file = open_netlink();

        if (sock_file < 0) {
                printf("Failed to perform request.\n");
                close(sock_file);
                return -1;
        }

        get_req = send_get_route_request(sock_file);
        if (get_req < 0) {
                printf("Failed to send get route request.\n");
                close(sock_file);
                return -1;
        }

        get_route_response(sock_file);

        close(sock_file);
        return 0;

}

//int main()
//{
//	int sock_file, get_req;
//
//	sock_file = open_netlink();
//
//	if (sock_file < 0) {
//		printf("Failed to perform request.\n");
//		close(sock_file);
//		return -1;
//	}
//
//	get_req = send_get_route_request(sock_file);
//	if (get_req < 0) {
//		printf("Failed to send get route request.\n");
//		close(sock_file);
//		return -1;
//	}
//
//	get_route_response(sock_file);
//
//	close(sock_file);
//	return 0;
//}
