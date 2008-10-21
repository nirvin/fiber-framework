#include "private/ff_common.h"

#include "private/ff_dictionary.h"
#include "private/ff_hash.h"

struct dictionary_entry
{
	struct dictionary_entry *next;
	const void *key;
	const void *value;
};

struct ff_dictionary
{
	int order;
	int entries_cnt;
	struct dictionary_entry **buckets;
};

#define WORDS_PER_PTR (sizeof(void *) / sizeof(uint32_t))
#define MASK(order) ((1ul << (order)) - 1)

static uint32_t get_bucket_num(struct ff_dictionary *dictionary, const void *key)
{
	uint32_t hash;
	uint32_t bucket_num;

	hash = ff_hash_uint32(0, (const uint32_t *) &key, WORDS_PER_PTR);
	bucket_num = hash & MASK(dictionary->order);
	return bucket_num;
}

struct ff_dictionary *ff_dictionary_create(int order)
{
	struct ff_dictionary *dictionary;
	uint32_t buckets_cnt;

	ff_assert(order >= 0);
	ff_assert(order < 20);
	buckets_cnt = 1ul << order;

	dictionary = (struct ff_dictionary *) ff_malloc(sizeof(*dictionary));
	dictionary->order = order;
	dictionary->entries_cnt = 0;
	dictionary->buckets = (struct dictionary_entry **) ff_calloc(buckets_cnt, sizeof(dictionary->buckets[0]));

	return dictionary;
}

void ff_dictionary_delete(struct ff_dictionary *dictionary)
{
	ff_assert(dictionary->entries_cnt == 0);

	ff_free(dictionary->buckets);
	ff_free(dictionary);
}

void ff_dictionary_put(struct ff_dictionary *dictionary, const void *key, const void *value)
{
	struct dictionary_entry *entry;
	uint32_t bucket_num;

	bucket_num = get_bucket_num(dictionary, key);
	entry = (struct dictionary_entry *) ff_malloc(sizeof(*entry));
	entry->next = dictionary->buckets[bucket_num];
	entry->key = key;
	entry->value = value;
	dictionary->buckets[bucket_num] = entry;
	dictionary->entries_cnt++;
}

enum ff_result ff_dictionary_get(struct ff_dictionary *dictionary, const void *key, const void **value)
{
	struct dictionary_entry *entry;
	uint32_t bucket_num;
	enum ff_result result = FF_FAILURE;

	bucket_num = get_bucket_num(dictionary, key);
	entry = dictionary->buckets[bucket_num];
	while (entry != NULL)
	{
		if (entry->key == key)
		{
			*value = entry->value;
			result = FF_SUCCESS;
			break;
		}
		entry = entry->next;
	}
	return result;
}

enum ff_result ff_dictionary_remove_entry(struct ff_dictionary *dictionary, const void *key, const void **value)
{
	struct dictionary_entry **entry_ptr;
	struct dictionary_entry *entry;
	uint32_t bucket_num;
	enum ff_result result = FF_FAILURE;

	bucket_num = get_bucket_num(dictionary, key);
	entry_ptr = &dictionary->buckets[bucket_num];
	entry = dictionary->buckets[bucket_num];
	while (entry != NULL)
	{
		if (entry->key == key)
		{
			*entry_ptr = entry->next;
			*value = entry->value;
			ff_free(entry);
			result = FF_SUCCESS;
			dictionary->entries_cnt--;
			break;
		}
		entry_ptr = &entry->next;
		entry = entry->next;
	}
	return result;
}