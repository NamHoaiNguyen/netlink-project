#include <arpa/inet.h>
#include <sys/socket.h>
#include <linux/rtnetlink.h>


int rtnl_receive(int fd, struct msghdr *msg, int flags);
int rtnl_recvmsg(int fd, struct msghdr *msg, char **answer);
void parse_rtattr(struct rtattr *tb[], int max, struct rtattr *rta, int len);
int rtm_get_table(struct rtmsg *r, struct rtattr **tb);
void print_route(struct nlmsghdr* nl_header_answer);
int open_netlink();
int get_route_response(int sock);
int get_routing_table();



