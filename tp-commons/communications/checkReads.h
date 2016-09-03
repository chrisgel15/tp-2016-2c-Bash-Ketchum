#ifndef CHECKREADS_H_
#define CHECKREADS_H_

#include "ltnCommons.h"

ltn_fd_sets * checkReads(ltn_fd_sets * fdSets, void (*funcProcessInstructionCode)(int, int), void(* funcDesconectados)(int), t_log * myLog);

#endif
