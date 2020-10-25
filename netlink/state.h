#include <net/if.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <linux/rtnetlink.h>

void Rtattr_parse(struct rtattr *tb[], int max, struct rtattr *rta, int len);
int monitor_net();




