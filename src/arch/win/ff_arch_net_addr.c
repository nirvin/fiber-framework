#include "ff_win_stdafx.h"

#include "private/arch/ff_arch_net_addr.h"
#include "private/ff_core.h"
#include "ff_win_net_addr.h"

#define MAX_STR_PORT_SIZE 6
#define ADDR_TO_STRING_BUF_SIZE 22

struct threadpool_addr_resolve_data
{
	struct ff_arch_net_addr *addr;
	const wchar_t *host;
	int port;
	enum ff_result result;
};

static enum ff_result resolve_addr(struct ff_arch_net_addr *addr, const wchar_t *host, int port)
{
	int rv;
	ADDRINFOW hint;
	PADDRINFOW addr_info_ptr;
	wchar_t str_port[MAX_STR_PORT_SIZE];
	enum ff_result result = FF_FAILURE;

	ff_assert(port >= 0);
	ff_assert(port < 0x10000);

	memset(&hint, 0, sizeof(hint));
	hint.ai_family = AF_INET;

	rv = swprintf_s(str_port, MAX_STR_PORT_SIZE, L"%d", port);
	ff_assert(rv != -1);
	addr_info_ptr = NULL;
	rv = GetAddrInfoW(host, str_port, &hint, &addr_info_ptr);
	if (rv != 0)
	{
		int last_error;

		last_error = WSAGetLastError();
		ff_log_debug(L"cannot resolve the [%ls:%d] address. WSAGetLastError()=%d", host, port, last_error);
		goto end;
	}
	ff_assert(addr_info_ptr != NULL);

	ff_assert(addr_info_ptr->ai_addrlen == sizeof(addr->addr));
	memcpy(&addr->addr, addr_info_ptr->ai_addr, sizeof(addr->addr));
	FreeAddrInfoW(addr_info_ptr);
	result = FF_SUCCESS;

end:
	return result;
}

static void threadpool_addr_resolve_func(void *ctx)
{
	struct threadpool_addr_resolve_data *data;

	data = (struct threadpool_addr_resolve_data *) ctx;
	data->result = resolve_addr(data->addr, data->host, data->port);
	if (data->result != FF_SUCCESS)
	{
		ff_log_debug(L"cannot resolve the [%ls:%d] address. See previous messages for more info", data->host, data->port);
	}
}

struct ff_arch_net_addr *ff_arch_net_addr_create()
{
	struct ff_arch_net_addr *addr;

	addr = (struct ff_arch_net_addr *) ff_malloc(sizeof(*addr));
		
	return addr;
}

void ff_arch_net_addr_delete(struct ff_arch_net_addr *addr)
{
	ff_free(addr);
}

enum ff_result ff_arch_net_addr_resolve(struct ff_arch_net_addr *addr, const wchar_t *host, int port)
{
	struct threadpool_addr_resolve_data data;

	data.addr = addr;
	data.host = host;
	data.port = port;
	data.result = FF_FAILURE;
	ff_core_threadpool_execute(threadpool_addr_resolve_func, &data);
	if (data.result != FF_SUCCESS)
	{
		ff_log_debug(L"cannot resolve the [%ls:%d] address. See previous messages for more info", host, port);
	}

	return data.result;
}

void ff_arch_net_addr_get_broadcast_addr(const struct ff_arch_net_addr *addr, const struct ff_arch_net_addr *net_mask, struct ff_arch_net_addr *broadcast_addr)
{
	struct sockaddr_in tmp_addr;

	memcpy(&tmp_addr, &addr->addr, sizeof(tmp_addr));
	tmp_addr.sin_addr.s_addr |= ~(net_mask->addr.sin_addr.s_addr);
	memcpy(&broadcast_addr->addr, &tmp_addr, sizeof(tmp_addr));
}

int ff_arch_net_addr_is_equal(const struct ff_arch_net_addr *addr1, const struct ff_arch_net_addr *addr2)
{
	int is_equal;
	
	is_equal = (memcmp(&addr1->addr, &addr2->addr, sizeof(addr1->addr)) == 0);
	return is_equal;
}

const wchar_t *ff_arch_net_addr_to_string(const struct ff_arch_net_addr *addr)
{
	wchar_t *buf;
	DWORD buf_size;
	INT rv;

	buf = (wchar_t *) ff_calloc(ADDR_TO_STRING_BUF_SIZE, sizeof(buf[0]));
	buf_size = ADDR_TO_STRING_BUF_SIZE;
	rv = WSAAddressToStringW((LPSOCKADDR) &addr->addr, sizeof(addr->addr), NULL, buf, &buf_size);
	ff_assert(rv == 0);
	ff_assert(buf_size < ADDR_TO_STRING_BUF_SIZE);

	return buf;
}

void ff_arch_net_addr_delete_string(const wchar_t *str)
{
	ff_free((void *) str);
}
