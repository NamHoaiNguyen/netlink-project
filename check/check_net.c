#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/netlink.h>
#include <net/sock.h>
#define NETLINK_TESTFAMILY 25

struct sock *socket;

static void test_nl_receive_message(struct sk_buff *skb) {
  struct nlmsghdr *nlh = (struct nlmsghdr *) skb->data;
  pid_t pid = nlh->nlmsg_pid; // pid of the sending process

  char *message = "Hello from kernel unicast";
  size_t message_size = strlen(message) + 1;
  struct sk_buff *skb_out = nlmsg_new(message_size, GFP_KERNEL);
  if (!skb_out) {
    printk(KERN_ERR "Failed to allocate a new skb\n");
    return;
  }

  nlh = nlmsg_put(skb_out, 0, 0, NLMSG_DONE, message_size, 0);
  NETLINK_CB(skb_out).dst_group = 0;
  strncpy(nlmsg_data(nlh), message, message_size);

  int result = nlmsg_unicast(socket, skb_out, pid);
}

//static void test_nl_receive_message(struct sk_buff *skb) {
//  printk(KERN_INFO "Entering: %s\n", __FUNCTION__);
//
//  struct nlmsghdr *nlh = (struct nlmsghdr *) skb->data;
//  printk(KERN_INFO "Received message: %s\n", (char*) nlmsg_data(nlh));
//}


static int __init test_init(void) {
  struct netlink_kernel_cfg config = {
    .input = test_nl_receive_message,
  };

  socket = netlink_kernel_create(&init_net, NETLINK_TESTFAMILY, &config);
  if (socket == NULL) {
    return -1;
  }

  return 0;
}

static void __exit test_exit(void) {
  if (socket) {
    netlink_kernel_release(socket);
  }
}

module_init(test_init);
module_exit(test_exit);
