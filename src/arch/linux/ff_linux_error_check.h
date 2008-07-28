#ifndef FF_LINUX_ERROR_CHECK
#define FF_LINUX_ERROR_CHECK

#include "private/ff_common.h"

#include <errno.h>

#ifdef __cplusplus
extern "C" {
#endif

void fatal_error(const wchar_t *format, ...);

#define ff_linux_fatal_error_check(expression, format, ...) \
	do { \
		if (!(expression)) \
		{ \
			fatal_error(L"fatal error at %s:%d, errno=%d. " format, __FILE__, __LINE__, errno, ## __VA_ARGS__); \
		} \
	} while (0)

#ifdef __cplusplus
}
#endif

#endif
