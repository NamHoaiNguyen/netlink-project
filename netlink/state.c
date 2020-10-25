#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <net/if.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <arpa/inet.h>


/*parsing attribute of struct rtattr*/
void Rtattr_parse(struct rtattr *tb[], int max, struct rtattr *rta, int len)
{
	memset(tb, 0, sizeof(struct rtattr *) * (max + 1));
	while (RTA_OK(rta, len)) {
		if (rta->rta_type <= max ) {
			tb[rta->rta_type] = rta;
		}
		rta = RTA_NEXT(rta, len);
	}
}

int monitor_net()
{
	int fd;
	char buf[8192];
	struct sockaddr_nl local;
	struct msghdr msg;
	struct iovec iov;

	fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);

	if (fd < 0) {
		printf("Failed to create netlink socket :%s\n", (char *)strerror(errno));
		return 0;
	}

	/*Struct iov*/
	iov.iov_base = buf;
	iov.iov_len = sizeof(buf);

	/*Struct sockaddr_nl*/
	memset(&local, 0, sizeof(local));
	local.nl_family = AF_NETLINK;
	local.nl_groups =   RTMGRP_LINK | RTMGRP_IPV4_IFADDR | RTMGRP_IPV4_ROUTE;   
//	 local.nl_groups =   RTMGRP_LINK;

	local.nl_pid = getpid();

	/*Struct msghdr: protocol message header*/
	msg.msg_name = &local;
	msg.msg_namelen = sizeof(local);
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;

	if (bind(fd, (struct sockaddr *)&local, sizeof(local)) < 0) {
		printf("Failed to bind netlink socket: %s\n", (char *)strerror(errno));
		close(fd);
		return 0;
	}

	while (1) {
		ssize_t status;
		struct nlmsghdr *h;
		int len, l;
		char *ifname, *ifup, *ifrun;
		char ifAddress[256];
		struct ifinfomsg *ifi;
		struct ifaddrmsg *ifa;
		struct rtattr *tb[IFLA_MAX + 1];
		struct rtattr *tba[IFA_MAX + 1];

//		status = recvmsg(fd, &msg, MSG_DONTWAIT);
		status = recvmsg(fd, &msg, 0);


		if (status < 0) {
		
			printf("Failed to read netlink: %s", (char *)strerror(errno));
			continue;
		}

		if (msg.msg_namelen != sizeof(local)) {
			printf("Invalid length of the sender address struct.\n");
			continue;
		}
		
		for (h = (struct nlmsghdr *)buf; status >= sizeof(*h); h = (struct nlmsghdr *)((char *)h + NLMSG_ALIGN(len))) {
			len = h->nlmsg_len;	/*Size of total message, including header*/
			l = len - sizeof(*h);	/*Size of message only*/

			if (l < 0 || len > status) {
				printf("Invalid message length; %i\n", len);
				continue;
			}

			/*Check message type*/
			if (h->nlmsg_type == RTM_NEWROUTE || h->nlmsg_type == RTM_DELROUTE) {
				printf("Routing table was changed.\n");
			}
			else {
				ifi = (struct ifinfomsg *)NLMSG_DATA(h);
				Rtattr_parse(tb, IFLA_MAX, IFLA_RTA(ifi), h->nlmsg_len);
				if (tb[IFLA_IFNAME]) {
					ifname = (char *)RTA_DATA(tb[IFLA_IFNAME]);				
				}

				if (ifi->ifi_flags & IFF_UP) {
					ifup = (char *)"UP";
				} else {
					ifup = (char *)"DOWN";
				}

				if (ifi->ifi_flags & IFF_RUNNING) {
					ifrun = (char *)"RUNNING";
				} else {
					ifrun = (char *)"NOT RUNNING";
				}
				
				ifa = (struct ifaddrmsg *)NLMSG_DATA(h);
                                Rtattr_parse(tba, IFA_MAX, IFA_RTA(ifa), h->nlmsg_len);
				if (tba[IFA_LOCAL]) {
					inet_ntop(AF_INET, RTA_DATA(tba[IFA_LOCAL]), ifAddress, sizeof(ifAddress));
				}

				switch (h->nlmsg_type) {
					case RTM_DELADDR:
						printf("Interface %s: address was removed.\n", ifname);
					case RTM_DELLINK:
						printf("Network interface %s was removed.\n", ifname);
					case RTM_NEWLINK:
						printf("Network interface %s was added, state: %s %s\n", ifname, ifup, ifrun);
					case RTM_NEWADDR:
						printf("Interface %s: new address was assigned : %s\n", ifname, ifAddress);
				}
			}
			status -= NLMSG_ALIGN(len);

//			h = (struct nlmsghdr *)((char *)h + NLMSG_ALIGN(len));
		}
		usleep(250000);
	}

	close (fd);

	return 0;
}

//int main()
//{
//	monitor_net();
//	return 0;
//}

