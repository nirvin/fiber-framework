interface foo
{
	method bar
	{
		request
		{
			uint32 a
			int64 b
			blob c
		}
		response
		{
			int32 d
		}
	}

	method baz
	{
		request
		{
		}
		response
		{
		}
	}
}

INTERFACE ::= "interface" id "{" METHODS_LIST "}"
METHODS_LIST ::= METHOD { METHOD }
METHOD ::= "method" id "{" REQUEST_PARAMS RESPONSE_PARAMS "}"
REQUEST_PARAMS ::= "request" "{" PARAMS_LIST "}"
RESPONSE_PARAMS ::= "response" "{" PARAMS_LIST "}"
PARAMS_LIST ::= PARAM { PARAM }
PARAM ::= TYPE id
TYPE ::= "uint32" | "uint64" | "int32" | "int64" | "string" | "blob"

id = [a-z_][a-z_\d]*

struct rpc_server_params
{
	struct rpc_interface *interface;
	void *service_ctx;
	struct ff_arch_net_addr *listen_addr;
};

#define RPC_SERVER_FIBER_STACK_SIZE 0x10000
#define CONNECTION_PROCESSORS_CNT 100

struct rpc_server
{
	struct ff_arch_net_addr *listen_addr;
	struct ff_pool *connection_processors;
	struct ff_fiber *main_fiber;
};

static void *create_connection_processor(void *ctx)
{
	struct rpc_connection_processor *connection_processor;

	connection_processor = rpc_connection_processor_create();
	return connection_processor;
}

static void delete_connection_processor(void *ctx)
{
	struct rpc_connection_processor *connection_processor;

	connection_processor = (struct rpc_connection_processor *) ctx;
	rpc_connection_processor_delete(connection_processor);
}

static void stop_connection_processor(void *entry, void *ctx, int is_acquired)
{
	struct rpc_connection_processor *connection_processor;

	connection_processor = (struct rpc_connection_processor *) entry;
	if (is_acquired)
	{
		rpc_connection_processor_stop(connection_processor);
	}
}

static void main_server_func(void *ctx)
{
	struct rpc_server *server;
	struct ff_tcp *server_tcp;
	struct ff_arch_net_addr *remote_addr;
	int is_success;

	server = (struct rpc_server *) ctx;
	server_tcp = ff_tcp_create();
	is_success = ff_tcp_bind(server_tcp, server->listen_addr, FF_TCP_SERVER);
	if (!is_success)
	{
		addr_str = ff_arch_net_addr_to_string(server->listen_addr);
		ff_log_write(FF_LOG_ERROR, L"cannot bind the address %ls to the server", addr_str);
	}

	remote_addr = ff_arch_net_addr_create();
	for (;;)
	{
		struct ff_tcp *client_tcp;
		struct rpc_connection_processor *connection_processor;

		client_tcp = ff_tcp_accept(server_tcp, remote_addr);
		if (client_tcp == NULL)
		{
			break;
		}

		connection_processor = acquire_connection_processor(server);
		rpc_connection_processor_start(connection_processor, client_tcp);
	}
	ff_pool_for_each_entry(server->connection_processors, stop_connection_processor, NULL);

	ff_arch_net_addr_delete(remote_addr);
	ff_tcp_delete(server_tcp);
}

struct rpc_server *rpc_server_create(const struct rpc_server_params *params)
{
	struct rpc_server *server;

	server = (struct rpc_server *) ff_malloc(sizeof(*server));

	server->listen_addr = params->listen_addr;
	server->connection_processors = ff_pool_create(CONNECTION_PROCESSORS_CNT, create_connection_processor, NULL, delete_connection_processor);
	server->main_fiber = ff_fiber_create(main_server_func, SERVER_FIBER_STACK_SIZE);
	ff_fiber_start(server->main_fiber, server);

	return server;
}

void rpc_server_delete(struct rpc_server *server)
{
	ff_pool_delete(server->connection_processors);
	ff_fiber_join(server->main_fiber);
	ff_free(server);
}

params.interface = foo_interface_extern;
params.service_ctx = foo_service_create();
params.listen_addr = listen_addr;

server = rpc_server_create(&params);
wait();
rpc_server_delete(server);


static const struct rpc_param_vtable *foo_request_param_vtables[] =
{
	&rpc_uint32_param_vtable,
	&rpc_int64_param_vtable,
	&rpc_blob_param_vtable
};

static const struct rpc_param_vtable *foo_response_param_vtables[] =
{
	&rpc_int32_param_vtable
};

static void foo_callback(struct rpc_data *data, void *service_ctx)
{
	uint32_t a;
	int64_t b;
	struct rpc_blob *c;
	int32_t d;

	rpc_data_get_request_param_value(data, 0, &a);
	rpc_data_get_request_param_value(data, 1, &b);
	rpc_data_get_request_param_value(data, 2, &c);

	service_func_foo(service_ctx, a, b, c, &d);

	rpc_data_set_response_param_value(data, 0, &d);
}

static const struct rpc_method foo_method =
{
	foo_callback,
	&foo_request_param_vtables,
	&foo_response_param_vtables,
	NULL,
	foo_request_params_cnt,
	foo_response_params_cnt
};

static const struct rpc_method *interface_methods[] =
{
	&foo_method,
	&bar_method
};

static const struct rpc_interface interface =
{
	interface_methods,
	interface_methods_cnt
};

extern const struct rpc_interface *foo_interface_extern = &interface;

struct rpc_param_vtable
{
	void *(*create)();
	void (*delete)(void *param);
	int (*read)(void *param, struct rpc_stream *stream);
	int (*write)(const void *param, struct rpc_stream *stream);
	void (*get_value)(const void *param, void **value);
	void (*set_value)(void *param, const void *value);
	uint32_t (*get_hash)(void *param, uint32_t start_value);
};

typedef void (*rpc_method_callback)(struct rpc_data *data, void *service_ctx);

struct rpc_method
{
	rpc_method_callback callback;
	struct rpc_param_vtable **request_param_vtables;
	struct rpc_param_vtable **response_param_vtables;
	int *is_key;
	int request_params_cnt;
	int response_params_cnt;
};

#define MAX_PARAMS_CNT 100

static void **create_params(struct rpc_param_vtable **param_vtables, int param_cnt)
{
	int i;
	void **params;

	ff_assert(param_cnt >= 0);
	ff_assert(param_cnt < MAX_PARAMS_CNT);
	params = (void **) ff_malloc(sizeof(*params) * param_cnt);
	for (i = 0; i < param_cnt; i++)
	{
		struct rpc_param_vtable *vtable;
		void *param;

		vtable = param_vtables[i];
		param = vtable->create();
		params[i] = param;
	}

	return params;
}

static void delete_params(void **params, struct rpc_param_vtable **param_vtables, int param_cnt)
{
	int i;

	ff_assert(param_cnt >= 0);
	ff_assert(param_cnt < MAX_PARAMS_CNT);
	for (i = 0; i < param_cnt; i++)
	{
		struct rpc_param_vtable *vtable;
		void *param;

		vtable = param_vtables[i];
		param = params[i];
		vtable->delete(param);
	}
	ff_free(params);
}

static int read_params(void **params, struct rpc_param_vtable *param_vtables, int param_cnt, struct rpc_stream *stream)
{
	int i;
	int is_success = 1;

	ff_assert(param_cnt >= 0);
	ff_assert(param_cnt < MAX_PARAMS_CNT);
	for (i = 0; i < param_cnt; i++)
	{
		struct rpc_param_vtable *vtable;
		void *param;

		vtable = param_vtables[i];
		param = params[i];
		is_success = vtable->read(param, stream);
		if (!is_success)
		{
			break;
		}
	}

	return is_success;
}

static int write_params(void **params, struct rpc_param_vtable *param_vtables, int param_cnt, struct rpc_stream *stream)
{
	int i;
	int is_success = 1;

	ff_assert(param_cnt >= 0);
	ff_assert(param_cnt < MAX_PARAMS_CNT);
	for (i = 0; i < param_cnt; i++)
	{
		struct rpc_param_vtable *vtable;
		void *param;

		vtable = param_vtables[i];
		param = params[i];
		is_success = vtable->write(param, stream);
		if (!is_success)
		{
			break;
		}
	}

	return is_success;
}

static void get_param_value(void **params, void **value, int param_idx, struct rpc_param_vtable *param_vtables, int params_cnt)
{
	struct rpc_param_vtable *vtable;
	void *param;

	ff_assert(params_cnt >= 0);
	ff_assert(params_cnt < MAX_PARAMS_CNT);
	ff_assert(param_idx >= 0);
	ff_assert(param_idx < params_cnt);

	vtable = param_vtables[param_idx];
	param = params[param_idx];
	vtable->get_value(param, value);
}

static void set_param_value(void **params, const void *value, int param_idx, struct rpc_param_vtable *param_vtables, int params_cnt)
{
	struct rpc_param_vtable *vtable;
	void *param;

	ff_assert(params_cnt >= 0);
	ff_assert(params_cnt < MAX_PARAMS_CNT);
	ff_assert(param_idx >= 0);
	ff_assert(param_idx < params_cnt);

	vtable = param_vtables[param_idx];
	param = params[param_idx];
	vtable->set_value(param, value);
}

void rpc_method_create_params(struct rpc_method *method, void ***request_params, void ***response_params)
{
	*request_params = create_params(method->request_param_vtables, method->request_params_cnt);
	*response_params = create_params(method->response_param_vtables, method->response_params_cnt);
}

void rpc_method_delete_params(struct rpc_method *method, void **request_params, void **response_params)
{
	delete_params(request_params, method->request_param_vtables, method->request_params_cnt);
	delete_params(response_params, method->response_param_vtables, method->response_params_cnt);
}

int rpc_method_read_request_params(struct rpc_method *method, void **params, struct rpc_stream *stream)
{
	int is_success;

	is_success = read_params(params, method->request_param_vtables, method->request_params_cnt, stream);
	return is_success;
}

int rpc_method_read_response_params(struct rpc_method *method, void **params, struct rpc_stream *stream)
{
	int is_success;

	is_success = read_params(params, method->response_param_vtables, method->response_params_cnt, stream);
	return is_success;
}

int rpc_method_write_request_params(struct rpc_method *method, void **params, struct rpc_stream *stream)
{
	int is_success;

	is_success = write_params(params, method->request_param_vtables, method->request_params_cnt, stream);
	return is_success;
}

int rpc_method_write_response_params(struct rpc_method *method, void **params, struct rpc_stream *stream)
{
	int is_success;

	is_success = write_params(params, method->response_param_vtables, method->response_params_cnt, stream);
	return is_success;
}

void rpc_method_set_request_param_value(struct rpc_method *method, int param_idx, void **params, const void *value)
{
	set_param_value(params, value, param_idx, method->request_param_vtables, method->request_params_cnt);
}

void rpc_method_get_request_param_value(struct rpc_method *method, int param_idx, void **params, void **value)
{
	get_param_value(params, value, param_idx, method->request_param_vtables, method->request_params_cnt);
}

void rpc_method_set_response_param_value(struct rpc_method *method, int param_idx, void **params, const void *value)
{
	set_param_value(params, value, param_idx, method->response_param_vtables, method->response_params_cnt);
}

void rpc_method_get_response_param_value(struct rpc_method *method, int param_idx, void **params, void **value)
{
	get_param_value(params, value, param_idx, method->response_param_vtables, method->response_params_cnt);
}

uint32_t rpc_method_get_request_hash(struct rpc_method *method, uint32_t start_value, void **params)
{
	int params_cnt;
	uint32_t hash_value;

	hash_value = start_value
	params_cnt = method->request_params_cnt;
	for (i = 0; i < params_cnt; i++)
	{
		int is_key;

		is_key = method->is_key[i];
		if (is_key)
		{
			struct rpc_param_vtable *vtable;
			void *param;

			vtable = method->request_param_vtables[i];
			param = params[i];
			hash_value = vtable->get_hash(param, hash_value);
		}
	}

	return hash_value;
}

void rpc_method_invoke_callback(struct rpc_method *method, struct rpc_data *data, void *service_ctx)
{
	method->callback(data, service_ctx);
}

struct rpc_interface
{
	struct rpc_method **methods;
	int methods_cnt;
};

struct rpc_method *rpc_interface_get_method(struct rpc_interface *interface, uint8_t method_id)
{
	struct rpc_method *method = NULL;

	if (method_id >= 0 && method_id < serivce->methods_cnt)
	{
		method = interface->methods[method_id];
	}
	return method;
}

struct rpc_data
{
	struct rpc_method *method;
	void **request_params;
	void **response_params;
	uint8_t method_id;
};

static struct rpc_data *read_request(struct rpc_interface *interface, struct rpc_stream *stream)
{
	uint8_t method_id;
	int bytes_read;
	struct rpc_method *method;
	struct rpc_data *data = NULL;
	int is_success;

	bytes_read = rpc_stream_read(stream, &method_id, 1);
	if (bytes_read != 1)
	{
		goto end;
	}
	method = rpc_interface_get_method(interface, method_id);
	if (method == NULL)
	{
		goto end;
	}
	data = rpc_data_create(method, method_id);
	is_success = rpc_method_read_request_params(data->method, data->request_params, stream);
	if (!is_success)
	{
		rpc_data_delete(data);
		data = NULL;
	}

end:
	return data;
}

static int write_response(struct rpc_data *data, struct rpc_stream *stream)
{
	int is_success;

	is_success = rpc_method_write_response_params(data->method, data->response_params, stream);
	return is_success;
}

static int write_request(struct rpc_data *data, struct rpc_stream *stream)
{
	int bytes_written;
	int is_success = 0;

	bytes_written = rpc_stream_write(stream, &data->method_id, 1);
	if (bytes_written == 1)
	{
		is_succes = rpc_method_write_request_params(data->method, data->request_params, stream);
	}

	return is_success;
}

static int read_response(struct rpc_data *data, struct rpc_stream *stream)
{
	int is_success;

	is_success = rpc_method_read_response_params(data->method, data->response_params, stream);
	return is_success;
}

struct rpc_data *rpc_data_create(struct rpc_method *method, uint8_t method_id)
{
	struct rpc_data *data;
	void **request_params;
	void **response_params;

	ff_assert(method_id >= 0);
	ff_assert(method_id < MAX_METHODS_CNT);
	rpc_method_create_params(method, &request_params, &response_params);

	data = (struct rpc_data *) ff_malloc(sizeof(*data));
	data->method = method;
	data->request_params = request_params;
	data->response_params = response_params;
	data->method_id = method_id;

	return data;
}

void rpc_data_delete(struct rpc_data *data)
{
	rpc_method_delete_params(data->method, data->request_params, data->response_params);
	ff_free(data);
}

int rpc_data_process_next_rpc(struct rpc_interface *interface, void *service_ctx, struct rpc_stream *stream)
{
	struct rpc_data *data;
	int is_success = 0;

	data = read_request(interface, stream);
	if (data != NULL)
	{
		rpc_method_invoke_callback(data->method, data, service_ctx);
		is_success = write_response(data, stream);
		rpc_data_delete(data);
    }

    return is_success;
}

int rpc_data_invoke_remote_call(struct rpc_data *data, struct rpc_stream *stream)
{
	int is_success;

	is_success = write_request(data, stream);
	if (is_success)
	{
		is_success = read_response(data, stream);
	}

	return is_success;
}

void rpc_data_get_request_param_value(struct rpc_data *data, int param_idx, void **value)
{
	rpc_method_get_request_param_value(data->method, param_idx, data->request_params, value);
}

void rpc_data_set_response_param_value(struct rpc_data *data, int param_idx, const void *value)
{
	rpc_method_set_response_param_value(data->method, param_idx, data->response_params, value);
}

void rpc_data_get_response_param_value(struct rpc_data *data, int param_idx, void **value)
{
	rpc_method_get_response_param_value(data->method, param_idx, data->response_params, value);
}

void rpc_data_set_request_param_value(struct rpc_data *data, int param_idx, const void *value)
{
	rpc_method_set_request_param_value(data->method, param_idx, data->request_params, value);
}

uint32_t rpc_data_get_request_hash(struct rpc_data *data, uint32_t start_value)
{
	uint32_t hash;

	hash = rpc_method_get_request_hash(data->method, start_value, data->request_params);
	return hash;
}