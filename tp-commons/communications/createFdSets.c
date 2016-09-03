#include "createFdSets.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

ltn_fd_sets * createFdSets(int sockNumber) {
	ltn_fd_sets * sets;
	sets = malloc(sizeof(ltn_fd_sets));

	FD_ZERO(&(sets->masterSet));
	FD_ZERO(&(sets->readFileDescriptorSet));
	FD_ZERO(&(sets->writeFileDescriptorSet));

	FD_SET(sockNumber, &(sets->masterSet));

	sets->maxFileDescriptorNumber = sockNumber;

	sets->listenerSocket = sockNumber;

	return sets;

}
