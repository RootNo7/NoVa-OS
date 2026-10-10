#include "user.h"
/* Shared syscall wrappers and user entry point live in user.h. */

void _start(void){ main(); sys_exit(); for(;;){} }
