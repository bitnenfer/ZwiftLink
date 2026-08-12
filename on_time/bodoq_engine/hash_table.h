#pragma once

#include "hash.h"
#include "debug.h"

template<typename K, typename V>
struct THashTableEntry
{
	K key;
	V value;
	THashTableEntry* next;
};

template<typename K, typename V>
struct THashTable
{
	THashTableEntry<K, V>** buckets;
	size_t bucketCount;
	THashTable(size_t numBuckets = 1024)
	{
		bucketCount = numBuckets;
		buckets = new THashTableEntry<K, V>* [bucketCount];
		for (size_t i = 0; i < bucketCount; i++)
		{
			buckets[i] = nullptr;
		}
	}
	~THashTable()
	{
		for (size_t i = 0; i < bucketCount; i++)
		{
			THashTableEntry<K, V>* entry = buckets[i];
			while (entry)
			{
				THashTableEntry<K, V>* next = entry->next;
				delete entry;
				entry = next;
			}
		}
		delete[] buckets;
	}
	void insert(const K& key, const V& value)
	{
		size_t hash = hash::hash(key);
		size_t index = hash % bucketCount;
		THashTableEntry<K, V>* newEntry = new THashTableEntry<K, V>{ key, value, buckets[index] };
		buckets[index] = newEntry;
	}
	void remove(const K& key)
	{
		size_t hash = hash::hash(key);
		size_t index = hash % bucketCount;
		THashTableEntry<K, V>* entry = buckets[index];
		THashTableEntry<K, V>* prev = nullptr;
		while (entry)
		{
			if (entry->key == key)
			{
				if (prev)
				{
					prev->next = entry->next;
				}
				else
				{
					buckets[index] = entry->next;
				}
				delete entry;
				return;
			}
			prev = entry;
			entry = entry->next;
		}
	}
	V& operator[](const K& key)
	{
		size_t hash = hash::hash(key);
		size_t index = hash % bucketCount;
		THashTableEntry<K, V>* entry = buckets[index];
		while (entry)
		{
			if (entry->key == key)
			{
				return entry->value;
			}
			entry = entry->next;
		}
		// If key not found, insert a new entry with default value
		V defaultValue = V();
		insert(key, defaultValue);
		return (*this)[key];
	}
	const V& operator[](const K& key) const
	{
		size_t hash = hash::hash(key);
		size_t index = hash % bucketCount;
		THashTableEntry<K, V>* entry = buckets[index];
		while (entry)
		{
			if (entry->key == key)
			{
				return entry->value;
			}
			entry = entry->next;
		}
		DBG_PANIC("Key not found in THashTable");
	}
	V* find(const K& key)
	{
		size_t hash = hash::hash(key);
		size_t index = hash % bucketCount;
		THashTableEntry<K, V>* entry = buckets[index];
		while (entry)
		{
			if (entry->key == key)
			{
				return &entry->value;
			}
			entry = entry->next;
		}
		return nullptr;
	}
	const V* find(const K& key) const
	{
		size_t hash = hash::hash(key);
		size_t index = hash % bucketCount;
		THashTableEntry<K, V>* entry = buckets[index];
		while (entry)
		{
			if (entry->key == key)
			{
				return &entry->value;
			}
			entry = entry->next;
		}
		return nullptr;
	}
	size_t getBucketCount() const
	{
		return bucketCount;
	}
	size_t getNumEntries() const
	{
		size_t count = 0;
		for (size_t i = 0; i < bucketCount; i++)
		{
			THashTableEntry<K, V>* entry = buckets[i];
			while (entry)
			{
				count++;
				entry = entry->next;
			}
		}
		return count;
	}
};