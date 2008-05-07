#ifndef FF_WIN_COMPLETION_PORT
#define FF_WIN_COMPLETION_PORT

#include "private/arch/ff_arch_completion_port.h"

#define WIN32_LEAN_AND_MEAN

#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

void ff_win_completion_port_register_overlapped_data(struct ff_arch_completion_port *completion_port, LPOVERLAPPED overlapped, void *data);

void ff_win_completion_port_deregister_overlapped_data(struct ff_arch_completion_port *completion_port, LPOVERLAPPED overlapped);

void ff_win_completion_port_register_handle(struct ff_arch_completion_port *completion_port, HANDLE handle);

#ifdef __cplusplus
}
#endif

#endif
