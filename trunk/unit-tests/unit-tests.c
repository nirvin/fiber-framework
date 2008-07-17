#include "public/ff_common.h"
#include "public/ff_core.h"
#include "public/ff_fiber.h"
#include "public/ff_event.h"
#include "public/ff_mutex.h"
#include "public/ff_semaphore.h"
#include "public/ff_blocking_queue.h"
#include "public/ff_blocking_stack.h"
#include "public/ff_pool.h"

#include <stdio.h>

#define ASSERT(expr, msg) \
	do \
	{ \
		if (!(expr)) \
		{ \
		return __FUNCTION__ ": ASSERT(" #expr ") failed: " msg; \
		} \
	} while (0)

#define RUN_TEST(test_name) \
	do \
	{ \
		char *msg; \
		msg = test_ ## test_name(); \
		if (msg != NULL) \
		{ \
			return msg; \
		} \
	} while (0)

#define DECLARE_TEST(test_name) static char *test_ ## test_name()

#pragma region ff_core_tests

DECLARE_TEST(core_init)
{
	ff_core_initialize();
	ff_core_shutdown();
	return NULL;
}

DECLARE_TEST(core_init_multiple)
{
	int i;
	for (i = 0; i < 10; i++)
	{
		RUN_TEST(core_init);
	}
	return NULL;
}

DECLARE_TEST(core_sleep)
{
	ff_core_initialize();
	ff_core_sleep(100);
	ff_core_shutdown();
	return NULL;
}

DECLARE_TEST(core_sleep_multiple)
{
	int i;

	ff_core_initialize();
	for (i = 0; i < 10; i++)
	{
		ff_core_sleep(i * 10 + 1);
	}
	ff_core_shutdown();
	return NULL;
}

static void threadpool_int_increment(void *ctx)
{
	int *a;

	a = (int *) ctx;
	a[1] = a[0] + 1;
}

DECLARE_TEST(core_threadpool_execute)
{
	int a[2];

	ff_core_initialize();
	a[0] = 1234;
	a[1] = 4321;
	ff_core_threadpool_execute(threadpool_int_increment, a);
	ASSERT(a[1] == a[0] + 1, "unexpected result");
	ff_core_shutdown();
	return NULL;
}

DECLARE_TEST(core_threadpool_execute_multiple)
{
	int i;

	ff_core_initialize();
	for (i = 0; i < 10; i++)
	{
		int a[2];

		a[0] = i;
		a[1] = i + 5;
		ff_core_threadpool_execute(threadpool_int_increment, a);
		ASSERT(a[1] == a[0] + 1, "unexpected result");
	}
	ff_core_shutdown();
	return NULL;
}

static void fiberpool_int_increment(void *ctx)
{
	int *a;

	a = (int *) ctx;
	(*a)++;
}

DECLARE_TEST(core_fiberpool_execute)
{
	int a = 0;

	ff_core_initialize();
	ff_core_fiberpool_execute_async(fiberpool_int_increment, &a);
	ff_core_shutdown();
	ASSERT(a == 1, "unexpected result");
	return NULL;
}

DECLARE_TEST(core_fiberpool_execute_multiple)
{
	int a = 0;
	int i;

	ff_core_initialize();
	for (i = 0; i < 10; i++)
	{
		ff_core_fiberpool_execute_async(fiberpool_int_increment, &a);
	}
	ff_core_shutdown();
	ASSERT(a == 10, "unexpected result");
	return NULL;
}

DECLARE_TEST(core_all)
{
	RUN_TEST(core_init);
	RUN_TEST(core_init_multiple);
	RUN_TEST(core_sleep);
	RUN_TEST(core_sleep_multiple);
	RUN_TEST(core_threadpool_execute);
	RUN_TEST(core_threadpool_execute_multiple);
	RUN_TEST(core_fiberpool_execute);
	RUN_TEST(core_fiberpool_execute_multiple);
	return NULL;
}

/* end of ff_core tests */
#pragma endregion

#pragma region ff_fiber tests

static void fiber_func(void *ctx)
{
	int *a;

	a = (int *) ctx;
	(*a)++;
}

DECLARE_TEST(fiber_create_delete)
{
	struct ff_fiber *fiber;

	ff_core_initialize();
	fiber = ff_fiber_create(fiber_func, 0);
	ASSERT(fiber != NULL, "unexpected result");
	ff_fiber_delete(fiber);
	ff_core_shutdown();
	return NULL;
}

DECLARE_TEST(fiber_start_join)
{
	struct ff_fiber *fiber;
	int a = 0;

	ff_core_initialize();
	fiber = ff_fiber_create(fiber_func, 0x100000);
	ff_fiber_start(fiber, &a);
	ff_fiber_join(fiber);
	ASSERT(a == 1, "unexpected result");
	ff_fiber_delete(fiber);
	ff_core_shutdown();
	return NULL;
}

DECLARE_TEST(fiber_start_multiple)
{
	struct ff_fiber *fibers[10];
	int a = 0;
	int i;

	ff_core_initialize();
	for (i = 0; i < 10; i++)
	{
		struct ff_fiber *fiber;

		fiber = ff_fiber_create(fiber_func, 0);
		fibers[i] = fiber;
	}
	for (i = 0; i < 10; i++)
	{
		struct ff_fiber *fiber;

		fiber = fibers[i];
		ff_fiber_start(fiber, &a);
	}
	for (i = 0; i < 10; i++)
	{
		struct ff_fiber *fiber;

		fiber = fibers[i];
		ff_fiber_join(fiber);
	}
	ASSERT(a == 10, "unexpected result");
	for (i = 0; i < 10; i++)
	{
		struct ff_fiber *fiber;

		fiber = fibers[i];
		ff_fiber_delete(fiber);
	}
	ff_core_shutdown();
	return NULL;
}

DECLARE_TEST(fiber_all)
{
	RUN_TEST(fiber_create_delete);
	RUN_TEST(fiber_start_join);
	RUN_TEST(fiber_start_multiple);
	return NULL;
}

/* end of ff_fiber tests */
#pragma endregion

#pragma region ff_event tests

DECLARE_TEST(event_manual_create_delete)
{
	struct ff_event *event;

	ff_core_initialize();

	event = ff_event_create(FF_EVENT_MANUAL);
	ASSERT(event != NULL, "unexpected result");
	ff_event_delete(event);

	ff_core_shutdown();
	return NULL;
}

DECLARE_TEST(event_auto_create_delete)
{
	struct ff_event *event;

	ff_core_initialize();

	event = ff_event_create(FF_EVENT_AUTO);
	ASSERT(event != NULL, "unexpected result");
	ff_event_delete(event);

	ff_core_shutdown();
	return NULL;
}

static void fiberpool_event_setter(void *ctx)
{
	struct ff_event *event;

	event = (struct ff_event *) ctx;
	ff_event_set(event);
}

DECLARE_TEST(event_manual_basic)
{
	struct ff_event *event;
	int is_set;

	ff_core_initialize();
	event = ff_event_create(FF_EVENT_MANUAL);
	is_set = ff_event_is_set(event);
	ASSERT(!is_set, "initial event state should be 'not set'");
	ff_core_fiberpool_execute_async(fiberpool_event_setter, event);
	ff_event_wait(event);
	is_set = ff_event_is_set(event);
	ASSERT(is_set, "event should be set by fiberpool_event_setter()");
	ff_event_wait(event);
	is_set = ff_event_is_set(event);
	ASSERT(is_set, "manual event should remain set after ff_event_wait");
	ff_event_set(event);
	is_set = ff_event_is_set(event);
	ASSERT(is_set, "event should be set after ff_event_set");
	ff_event_reset(event);
	is_set = ff_event_is_set(event);
	ASSERT(!is_set, "event should be reset by ff_event_reset");
	ff_event_reset(event);
	is_set = ff_event_is_set(event);
	ASSERT(!is_set, "event should remain reset after ff_event_set");
	ff_event_delete(event);
	ff_core_shutdown();
	return NULL;
}

DECLARE_TEST(event_auto_basic)
{
	struct ff_event *event;
	int is_set;

	ff_core_initialize();
	event = ff_event_create(FF_EVENT_AUTO);
	is_set = ff_event_is_set(event);
	ASSERT(!is_set, "initial event state should be 'not set'");
	ff_core_fiberpool_execute_async(fiberpool_event_setter, event);
	ff_event_wait(event);
	is_set = ff_event_is_set(event);
	ASSERT(!is_set, "autoreset event should be 'not set' after ff_event_wait");
	ff_event_set(event);
	is_set = ff_event_is_set(event);
	ASSERT(is_set, "event should be set after ff_event_set");
	ff_event_set(event);
	is_set = ff_event_is_set(event);
	ASSERT(is_set, "event should remain set after ff_event_set");
	ff_event_reset(event);
	is_set = ff_event_is_set(event);
	ASSERT(!is_set, "event should be 'not set' after ff_event_reset");
	ff_event_reset(event);
	is_set = ff_event_is_set(event);
	ASSERT(!is_set, "event should remain 'not set' after ff_event_reset");
	ff_event_delete(event);
	ff_core_shutdown();
	return NULL;
}

DECLARE_TEST(event_manual_timeout)
{
	struct ff_event *event;
	int is_success;
	int is_set;

	ff_core_initialize();

	event = ff_event_create(FF_EVENT_MANUAL);
	is_success = ff_event_wait_with_timeout(event, 100);
	ASSERT(!is_success, "event should timeout");
	is_set = ff_event_is_set(event);
	ASSERT(!is_set, "event should remain 'not set' after timeout");
	ff_event_set(event);
	is_success = ff_event_wait_with_timeout(event, 100);
	ASSERT(is_success, "event shouldn't timeout");
	is_set = ff_event_is_set(event);
	ASSERT(is_set, "manual event should remain set after ff_event_wait_with_timeout");
	ff_event_delete(event);

	ff_core_shutdown();
	return NULL;
}

DECLARE_TEST(event_auto_timeout)
{
	struct ff_event *event;
	int is_success;
	int is_set;

	ff_core_initialize();

	event = ff_event_create(FF_EVENT_AUTO);
	is_success = ff_event_wait_with_timeout(event, 100);
	ASSERT(!is_success, "event should timeout");
	is_set = ff_event_is_set(event);
	ASSERT(!is_set, "event should remain 'not set' after timeout");
	ff_event_set(event);
	is_success = ff_event_wait_with_timeout(event, 100);
	ASSERT(is_success, "event shouldn't timeout");
	is_set = ff_event_is_set(event);
	ASSERT(!is_set, "auto reset event should be 'not set' after ff_event_wait_with_timeout");
	ff_event_delete(event);

	ff_core_shutdown();
	return NULL;
}

static void fiberpool_event_func(void *ctx)
{
	struct ff_event *event, *done_event;
	int *a;

	event = (struct ff_event *) ((void **)ctx)[0];
	done_event = (struct ff_event *) ((void **)ctx)[1];
	a = (int *) ((void **)ctx)[2];

	ff_event_wait(event);
	(*a)++;
	if (*a == 15)
	{
		ff_event_set(done_event);
	}
}

DECLARE_TEST(event_manual_multiple)
{
	struct ff_event *event, *done_event;
	int i;
	int a = 0;
	void *data[3];

	ff_core_initialize();
	event = ff_event_create(FF_EVENT_MANUAL);
	done_event = ff_event_create(FF_EVENT_MANUAL);
	data[0] = event;
	data[1] = done_event;
	data[2] = &a;
	for (i = 0; i < 15; i++)
	{
		ff_core_fiberpool_execute_async(fiberpool_event_func, data);
	}
	ASSERT(a == 0, "a shouldn't change while event isn't set");
	ff_event_set(event);
	ff_event_wait(done_event);
	ASSERT(a == 15, "a should set to 15 after done_event set inside fiberpool_event_func");
	ff_event_delete(done_event);
	ff_event_delete(event);
	ff_core_shutdown();
	return NULL;
}

DECLARE_TEST(event_auto_multiple)
{
	struct ff_event *event, *done_event;
	int i;
	int a = 0;
	int is_success;
	void *data[3];

	ff_core_initialize();
	event = ff_event_create(FF_EVENT_AUTO);
	done_event = ff_event_create(FF_EVENT_MANUAL);
	data[0] = event;
	data[1] = done_event;
	data[2] = &a;
	for (i = 0; i < 15; i++)
	{
		ff_core_fiberpool_execute_async(fiberpool_event_func, data);
	}
	ASSERT(a == 0, "a shouldn't change while event isn't set");
	for (i = 0; i < 14; i++)
	{
		ff_event_set(event);
		is_success = ff_event_wait_with_timeout(done_event, 1);
		ASSERT(!is_success, "done_event should remain 'not set'");
	}
	ff_event_set(event);
	ff_event_wait(done_event);
	ASSERT(a == 15, "a should have value 15 after done_event set");
	ff_event_delete(done_event);
	ff_event_delete(event);
	ff_core_shutdown();
	return NULL;
}

DECLARE_TEST(event_all)
{
	RUN_TEST(event_manual_create_delete);
	RUN_TEST(event_auto_create_delete);
	RUN_TEST(event_manual_basic);
	RUN_TEST(event_auto_basic);
	RUN_TEST(event_manual_timeout);
	RUN_TEST(event_auto_timeout);
	RUN_TEST(event_manual_multiple);
	RUN_TEST(event_auto_multiple);
	return NULL;
}

/* end of ff_event tests */
#pragma endregion

#pragma region ff_mutex tests

DECLARE_TEST(mutex_create_delete)
{
	struct ff_mutex *mutex;

	ff_core_initialize();
	mutex = ff_mutex_create();
	ASSERT(mutex != NULL, "mutex should be initialized");
	ff_mutex_delete(mutex);
	ff_core_shutdown();
	return NULL;
}

static void fiberpool_mutex_func(void *ctx)
{
	struct ff_mutex *mutex;
	struct ff_event *event;
	int *a;

	mutex = (struct ff_mutex *) ((void **)ctx)[0];
	event = (struct ff_event *) ((void **)ctx)[1];
	a = (int *) ((void **)ctx)[2];

	ff_mutex_lock(mutex);
	*a = 123;
	ff_event_set(event);
	ff_core_sleep(100);
	*a = 10;
	ff_mutex_unlock(mutex);
}

DECLARE_TEST(mutex_basic)
{
	void *data[3];
	int a = 0;
	struct ff_mutex *mutex;
	struct ff_event *event;

	ff_core_initialize();
	mutex = ff_mutex_create();
	event = ff_event_create(FF_EVENT_AUTO);
	ff_mutex_lock(mutex);
	ff_mutex_unlock(mutex);

	data[0] = mutex;
	data[1] = event;
	data[2] = &a;
	ff_core_fiberpool_execute_async(fiberpool_mutex_func, data);
	ff_event_wait(event);
	ASSERT(a == 123, "a should be 123, because the fiberpool_mutex_func should sleep in the ff_core_sleep");
	ff_mutex_lock(mutex);
	ASSERT(a == 10, "a should be 10, because fiberpool_mutex_func should unlock the mutex after sleep");
	ff_mutex_unlock(mutex);

	ff_event_delete(event);
	ff_mutex_delete(mutex);
	ff_core_shutdown();
	return NULL;
}

DECLARE_TEST(mutex_all)
{
	RUN_TEST(mutex_create_delete);
	RUN_TEST(mutex_basic);
	return NULL;
}

/* end of ff_mutex tests */
#pragma endregion

#pragma region ff_semaphore tests

DECLARE_TEST(semaphore_create_delete)
{
	struct ff_semaphore *semaphore;

	ff_core_initialize();
	semaphore = ff_semaphore_create(0);
	ASSERT(semaphore != NULL, "semaphore should be initialized");
	ff_semaphore_delete(semaphore);
	ff_core_shutdown();
	return NULL;
}

DECLARE_TEST(semaphore_basic)
{
	int i;
	int is_success;
	struct ff_semaphore *semaphore;

	ff_core_initialize();
	semaphore = ff_semaphore_create(0);
	is_success = ff_semaphore_down_with_timeout(semaphore, 1);
	ASSERT(!is_success, "semaphore with 0 value cannot be down");
	for (i = 0; i < 10; i++)
	{
		ff_semaphore_up(semaphore);
	}
	is_success = ff_semaphore_down_with_timeout(semaphore, 1);
	ASSERT(is_success, "semaphore should be down");
	for (i = 0; i < 9; i++)
	{
		ff_semaphore_down(semaphore);
	}
	is_success = ff_semaphore_down_with_timeout(semaphore, 1);
	ASSERT(!is_success, "semaphore cannot be down");
	ff_semaphore_delete(semaphore);
	ff_core_shutdown();
	return NULL;
}

DECLARE_TEST(semaphore_all)
{
	RUN_TEST(semaphore_create_delete);
	RUN_TEST(semaphore_basic);
	return NULL;
}

/* end of ff_semaphore tests */
#pragma endregion

#pragma region ff_blocking_queue tests

DECLARE_TEST(blocking_queue_create_delete)
{
	struct ff_blocking_queue *queue;

	ff_core_initialize();
	queue = ff_blocking_queue_create(10);
	ASSERT(queue != NULL, "queue should be initialized");
	ff_blocking_queue_delete(queue);
	ff_core_shutdown();
	return NULL;
}

DECLARE_TEST(blocking_queue_basic)
{
	int i;
	int is_success;
	int data;
	struct ff_blocking_queue *queue;

	ff_core_initialize();
	queue = ff_blocking_queue_create(10);
	for (i = 0; i < 10; i++)
	{
		ff_blocking_queue_put(queue, (void *) i);
	}
	is_success = ff_blocking_queue_put_with_timeout(queue, (void *) 123, 1);
	ASSERT(!is_success, "queue should be full");
	for (i = 0; i < 10; i++)
	{
		ff_blocking_queue_get(queue, &(void *)data);
		ASSERT(data == i, "wrong value received from the queue");
	}
	is_success = ff_blocking_queue_get_with_timeout(queue, &(void *)data, 1);
	ASSERT(!is_success, "queue should be empty");
	ff_blocking_queue_delete(queue);
	ff_core_shutdown();
	return NULL;
}

static void fiberpool_blocking_queue_func(void *ctx)
{
	struct ff_blocking_queue *queue;

	queue = (struct ff_blocking_queue *) ctx;
	ff_blocking_queue_put(queue, (void *)543);
}

DECLARE_TEST(blocking_queue_fiberpool)
{
	int data;
	int is_success;
	struct ff_blocking_queue *queue;

	ff_core_initialize();
	queue = ff_blocking_queue_create(1);
	is_success = ff_blocking_queue_get_with_timeout(queue, &(void *)data, 1);
	ASSERT(!is_success, "queue should be empty");
	ff_core_fiberpool_execute_async(fiberpool_blocking_queue_func, queue);
	ff_blocking_queue_get(queue, &(void *)data);
	ASSERT(data == 543, "unexpected value received from the queue");
	is_success = ff_blocking_queue_get_with_timeout(queue, &(void *)data, 1);
	ASSERT(!is_success, "queue shouldn't have values");
	ff_blocking_queue_delete(queue);
	ff_core_shutdown();
	return NULL;
}

DECLARE_TEST(blocking_queue_all)
{
	RUN_TEST(blocking_queue_create_delete);
	RUN_TEST(blocking_queue_basic);
	RUN_TEST(blocking_queue_fiberpool);
	return NULL;
}

/* end of ff_blocking_queue tests */
#pragma endregion

#pragma region ff_blocking_stack tests

DECLARE_TEST(blocking_stack_create_delete)
{
	struct ff_blocking_stack *stack;

	ff_core_initialize();
	stack = ff_blocking_stack_create(10);
	ASSERT(stack != NULL, "stack should be initialized");
	ff_blocking_stack_delete(stack);
	ff_core_shutdown();
	return NULL;
}

DECLARE_TEST(blocking_stack_basic)
{
	int i;
	int is_success;
	int data;
	struct ff_blocking_stack *stack;

	ff_core_initialize();
	stack = ff_blocking_stack_create(10);
	for (i = 0; i < 10; i++)
	{
		ff_blocking_stack_push(stack, (void *) i);
	}
	is_success = ff_blocking_stack_push_with_timeout(stack, (void *) 1234, 1);
	ASSERT(!is_success, "stack should be fulll");
	for (i = 9; i >= 0; i--)
	{
		ff_blocking_stack_pop(stack, &(void *)data);
		ASSERT(data == i, "wrong value retrieved from the stack");
	}
	is_success = ff_blocking_stack_pop_with_timeout(stack, &(void *)data, 1);
	ASSERT(!is_success, "stack should be empty");
	ff_blocking_stack_delete(stack);
	ff_core_shutdown();
	return NULL;
}

static void fiberpool_blocking_stack_func(void *ctx)
{
	struct ff_blocking_stack *stack;

	stack = (struct ff_blocking_stack *) ctx;
	ff_blocking_stack_push(stack, (void *)543);
}

DECLARE_TEST(blocking_stack_fiberpool)
{
	int data;
	int is_success;
	struct ff_blocking_stack *stack;

	ff_core_initialize();
	stack = ff_blocking_stack_create(1);
	is_success = ff_blocking_stack_pop_with_timeout(stack, &(void *)data, 1);
	ASSERT(!is_success, "stack should be empty");
	ff_core_fiberpool_execute_async(fiberpool_blocking_stack_func, stack);
	ff_blocking_stack_pop(stack, &(void *)data);
	ASSERT(data == 543, "unexpected value received from the stack");
	is_success = ff_blocking_stack_pop_with_timeout(stack, &(void *)data, 1);
	ASSERT(!is_success, "stack shouldn't have values");
	ff_blocking_stack_delete(stack);
	ff_core_shutdown();
	return NULL;
}

DECLARE_TEST(blocking_stack_all)
{
	RUN_TEST(blocking_stack_create_delete);
	RUN_TEST(blocking_stack_basic);
	RUN_TEST(blocking_stack_fiberpool);
	return NULL;
}

/* end of ff_blocking_stack tests */
#pragma endregion

#pragma region ff_pool tests

static int pool_entries_cnt = 0;

static void *pool_entry_constructor(void *ctx)
{
	pool_entries_cnt++;
	return (void *)123;
}

static void pool_entry_destructor(void *entry)
{
	pool_entries_cnt--;
}

DECLARE_TEST(pool_create_delete)
{
	struct ff_pool *pool;

	ff_core_initialize();
	pool = ff_pool_create(10, pool_entry_constructor, NULL, pool_entry_destructor);
	ASSERT(pool_entries_cnt == 0, "pool should be empty after creation");
	ff_pool_delete(pool);
	ASSERT(pool_entries_cnt == 0, "pool should be empty after deletion");
	ff_core_shutdown();
	return NULL;
}

DECLARE_TEST(pool_basic)
{
	struct ff_pool *pool;
	void *entry;
	int i;

	ff_core_initialize();
	pool = ff_pool_create(10, pool_entry_constructor, NULL, pool_entry_destructor);
	for (i = 0; i < 10; i++)
	{
		entry = ff_pool_acquire_entry(pool);
		ASSERT(entry == (void *)123, "unexpected value for the entry");
		ASSERT(pool_entries_cnt == i + 1, "unexpected entries number");
	}
	for (i = 0; i < 10; i++)
	{
		ff_pool_release_entry(pool, (void *)123);
	}
	ff_pool_delete(pool);
	ASSERT(pool_entries_cnt == 0, "pool should be empty after deletion");
	ff_core_shutdown();
	return NULL;
}

static void fiberpool_pool_func(void *ctx)
{
	struct ff_pool *pool;

	pool = (struct ff_pool *) ctx;
	ff_pool_release_entry(pool, (void *)123);
}

DECLARE_TEST(pool_fiberpool)
{
	struct ff_pool *pool;
	void *entry;
	
	ff_core_initialize();
	pool = ff_pool_create(1, pool_entry_constructor, NULL, pool_entry_destructor);
	ASSERT(pool_entries_cnt == 0, "pool should be empty after creation");
	entry = ff_pool_acquire_entry(pool);
	ASSERT(entry == (void *)123, "unexpected value received from the pool");
	ASSERT(pool_entries_cnt == 1, "pool should create one entry");
	ff_core_fiberpool_execute_async(fiberpool_pool_func, pool);
	entry = ff_pool_acquire_entry(pool);
	ASSERT(entry == (void *)123, "wrong entry value");
	ASSERT(pool_entries_cnt == 1, "pool should contain one entry");
	ff_pool_release_entry(pool, (void *)123);
	ff_pool_delete(pool);
	ASSERT(pool_entries_cnt == 0, "pool should be empty after deletion");
	ff_core_shutdown();
	return NULL;
}

DECLARE_TEST(pool_all)
{
	RUN_TEST(pool_create_delete);
	RUN_TEST(pool_basic);
	RUN_TEST(pool_fiberpool);
	return NULL;
}

/* end of ff_pool tests */
#pragma endregion

static char *run_all_tests()
{
	RUN_TEST(core_all);
	RUN_TEST(fiber_all);
	RUN_TEST(event_all);
	RUN_TEST(mutex_all);
	RUN_TEST(semaphore_all);
	RUN_TEST(blocking_queue_all);
	RUN_TEST(blocking_stack_all);
	RUN_TEST(pool_all);
	return NULL;
}

int main(int argc, char* argv[])
{
	char *msg;

	msg = run_all_tests();
	if (msg != NULL)
	{
		printf("%s\n", msg);
	}
	else
	{
		printf("ALL TESTS PASSED\n");
	}

	return 0;
}
