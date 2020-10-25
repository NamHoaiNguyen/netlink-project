#include <stdio.h>
#include <string.h>

#include "state.h"
#include "route.h"

int main(int argc, char **argv)
{
	printf("--------------------------------------------------------------------------------\n");
	if (argc != 2) {
		printf("Give a correct argument.\n");
		return 0;
	} else {
		if (strcmp(argv[1], "state") == 0) {
			monitor_net();
		}
		else if (strcmp(argv[1], "route-table") == 0) {
			int res = get_routing_table();
			if (res < 0) {
				printf("Get routing table error.\n");
				return 0;
			}
		} else {
			printf("Argument is error.\n");
		}
	}

	
	return 0;
}
