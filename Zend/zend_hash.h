/*
   +----------------------------------------------------------------------+
   | Zend Engine                                                          |
   +----------------------------------------------------------------------+
   | Copyright (c) 1998-2013 Zend Technologies Ltd. (http://www.zend.com) |
   +----------------------------------------------------------------------+
   | This source file is subject to version 2.00 of the Zend license,     |
   | that is bundled with this package in the file LICENSE, and is        | 
   | available through the world-wide-web at the following url:           |
   | http://www.zend.com/license/2_00.txt.                                |
   | If you did not receive a copy of the Zend license and are unable to  |
   | obtain it through the world-wide-web, please send a note to          |
   | license@zend.com so we can mail you a copy immediately.              |
   +----------------------------------------------------------------------+
   | Authors: Andi Gutmans <andi@zend.com>                                |
   |          Zeev Suraski <zeev@zend.com>                                |
   +----------------------------------------------------------------------+
*/

/* $Id$ */

#ifndef ZEND_HASH_H
#define ZEND_HASH_H

#include <sys/types.h>
#include "zend.h"

#define HASH_KEY_IS_STRING 1
#define HASH_KEY_IS_LONG 2
#define HASH_KEY_NON_EXISTANT 3

#define HASH_UPDATE 		(1<<0)
#define HASH_ADD			(1<<1)
#define HASH_NEXT_INSERT	(1<<2)

#define HASH_DEL_KEY 0
#define HASH_DEL_INDEX 1
#define HASH_DEL_KEY_QUICK 2

#define HASH_UPDATE_KEY_IF_NONE    0
#define HASH_UPDATE_KEY_IF_BEFORE  1
#define HASH_UPDATE_KEY_IF_AFTER   2
#define HASH_UPDATE_KEY_ANYWAY     3

typedef ulong (*hash_func_t)(const char *arKey, uint nKeyLength);
typedef int  (*compare_func_t)(const void *, const void * TSRMLS_DC);
typedef void (*sort_func_t)(void *, size_t, register size_t, compare_func_t TSRMLS_DC);
typedef void (*dtor_func_t)(void *pDest);
typedef void (*copy_ctor_func_t)(void *pElement);
typedef void (*copy_ctor_param_func_t)(void *pElement, void *pParam);

struct _hashtable;
/**
 * Zend HashTable有一个桶，包含有nTableSize个元素，这个元素限定了HashTable的大小，
 * 同时也限定了HashTable中能保存Bucket的最大数量， 这个值越大， 需要的内存就越多。
 * 为了提高效率， 系统会自动将这个值调整到最小一个不小于nTableSize的2^n整数次方上。
 * 
 * 桶中的每个桶元素(Bucket)都有一个键名(key)， 它在整个HashTable中是唯一的， 不能重复。
 * 根据键名就可以唯一的确定HashTable中的Bucket元素。
 * 键名有两种表示方式:
 *   1) 使用字符串arKey作为键名， 该字符串的长度为nKeyLength。
 *      从下面的结构体中可以看到arKey是长度为1的字符数组，但它并不意味着key只能是一个字符。
 *      实际上Bucket是一个可变长的结构体，由于arKey是Bucket中的最后一个成员变量，通过arKey与nKeyLength结合nKeyLength可确定一个长度为nKeyLength的key. 这也是C语言中比较常用的技巧。
 *   2) 使用索引方式， 这时nKeyLength总是为0， 长整型字段h就表示该数据元素的键名。
 * 简单的来说，即如果nKeyLength=0， 则键名为h；否则键名为arKey,键名长度为nKeyLength.
 * 而对于nKeyLength > 0的时候，h也是有意义的，存放的是arKey对应的hash值。 不管hash函数怎么设计， 冲突都是不可避免的， 也就是说不同的arKey可能有相同的hash值。
 * 具有相同的hash值的Bucket保存在HashTable的arBuckets数组中的同一个索引对应的桶列中。 桶列为一个双向链表， 其前驱元素， 后向元素分别使用pLast, pNext来表示。 新插入的Bucket放在该桶列的最前面。
 * 
 * Bucket中实际数据保存位置:
 *   一般情况， 实际数据是保存在pData指针指向的内存块中， 通常这个内存块是系统另外分配的。
 *   但有一种情况例外，就是当Bucket保存的数据为一个指针的时候，HashTable将不会另外请求系统分配空间来保存这个指针， 而是直接将该指针保存到pDataPtr中， 然后再将pData指向pDataPtr的地址。这样可以提高效率， 减少内存碎片。 
 *   由此可见，PHP HashTable设计的精妙之处。 如果Bucket中的数据不是一个指针， pDataPtr为NULL.
 * 
 * HashTable中的所有Bucket通过pListNext, pListLast构成了一个双向链表。 最新插入的Bucket放在双向链表的最后。
 * 注意在一般情况下， Bucket并不能提供它所存储的数据的大小信息。 所以在PHP的实现中， Bucket中保存的数据必须具有管理自身大小的能力。
 */
typedef struct bucket {
	/* Used for numeric indexing */
	ulong h;                          // 对char *key进行hash后的值，或者是用户指定的数字索引值
	uint nKeyLength;                  // hash关键字的长度，如果数组索引为数字，此值为0
	void *pData;                      // 指向value，一般是用户数据的副本，如果是指针数据，则指向pDataPtr
	void *pDataPtr;                   // 如果是指针数据，此值会指向真正的value，同时上面pData会指向此值
	struct bucket *pListNext;         // 整个hash表的下一元素
	struct bucket *pListLast;         // 整个哈希表该元素的上一个元素
	struct bucket *pNext;             // 存放在同一个hash Bucket内的下一个元素
	struct bucket *pLast;             // 同一个哈希bucket的上一个元素
	const char *arKey;                // 存储字符索引，此项必须放在最未尾，因为此处只字义了1个字节，存储的实际上是指向char *key的值，这就意味着可以省去再赋值一次的消耗，而且，有时此值并不需要，所以同时还节省了空间。
} Bucket;


/**
 * arBuckets是HashTable的关键， HashTable初始化的时候会自动申请一块内存， 并将其地址赋值给arBuckets, 该内存大小正好能容纳nTableSize个指针。 我们可以将arBuckets看作一个大小为nTableSize的数组， 每个数组元素都是一个指针，用于指向实际存放数据的Bucket。 当然刚开始的时候每个指针均为NULL.
 * nTableMask的值永远是nTableSize - 1，引入这个字段主要目的是为了提高计算效率，是为了快速计算Bucket键名在arBuckets数组中的索引。
 * nNumberOfElements记录了HashTable当前保存的数据元素的个数。当nNumberOfElement大于nTableSize时，HashTable将自动扩展为原来的两倍大小。
 * nNextFreeElement记录HashTable中下一个可用于插入数据元素的arBuckets的索引。
 * pListHead和pListTail则分别表示Bucket双向链表的第一个和最后一个元素，这些数据元素通常是根据插入的顺序排列的。 也可以通过各种排序函数对其进行重新排列。
 * pInternalPointer: 则用于在遍历HashTable时记录当前遍历的位置，它是一个指针， 指向当前遍历到的Bucket， 初始值为pListHead.
 * pDestructor是一个函数指针， 在HashTable的增加、修改、删除Bucket时自动调用，用于处理相关数据的清理工作。
 * persistent标志位指出了Bucket内存分配的方式。 如果persistent为TRUE，则使用操作系统本身的内存分配函数为Bucket分配内存，否则使用PHP的内存分配函数。
 * nApplyCount与bApplyProtection结合提供了一个防止在遍历HashTable时进入递归循环时的一种机制。
 * inconsistent成员用于调试目的， 只有在PHP编译成调试版本时有效。
 *
 *
 * 表示HashTable的状态的常量有四种:
 * 状态值             含义
 * HT_IS_DESTROYING   正在删除所有内容，包括arBuckets本身
 * HT_IS_DESTROYED    已删除，包括arBuckets本身
 * HT_CLEANING        正在清除所有的arBuckets指向的内容，但不包括arBuckets本身
 * HT_OK              正常状态,各种数据完全一致
 */
typedef struct _hashtable {
	uint nTableSize;                  // hash Bucket的大小，最小为8，以2x增长。
	uint nTableMask;                  // nTableSize-1 ， 索引取值的优化
	uint nNumOfElements;              // hash Bucket中当前存在的元素个数，count()函数会直接返回此值 
	ulong nNextFreeElement;           // 下一个数字索引的位置
	/* Used for element traversal */
	Bucket *pInternalPointer;         // 当前遍历的指针（foreach比for快的原因之一）
	Bucket *pListHead;                // 存储数组头元素指针
	Bucket *pListTail;                // 存储数组尾元素指针
	Bucket **arBuckets;               // 存储hash数组
	dtor_func_t pDestructor;          // 
	zend_bool persistent;
	unsigned char nApplyCount;        // 标记当前hash Bucket被递归访问的次数（防止多次递归）
	zend_bool bApplyProtection;       // 标记当前hash桶允许不允许多次访问，不允许时，最多只能递归3次
#if ZEND_DEBUG
	int inconsistent;
#endif
} HashTable;

/**
 * 了解了上面HashTable的结构和Bucket的结构，下面理解zend_hash_key就比较容易了。 它通过arKey, nKeyLength, h三个字段唯一确定了HashTable中的一个元素。
 */
typedef struct _zend_hash_key {
	const char *arKey;                /* hash元素key名称 */
	uint nKeyLength;                  /* hash 元素key长度 */
	ulong h;                          /* key计算出的hash值或直接指定的数值下标 */
} zend_hash_key;


typedef zend_bool (*merge_checker_func_t)(HashTable *target_ht, void *source_data, zend_hash_key *hash_key, void *pParam);

typedef Bucket* HashPosition;

BEGIN_EXTERN_C()

/* startup/shutdown */
ZEND_API int _zend_hash_init(HashTable *ht, uint nSize, hash_func_t pHashFunction, dtor_func_t pDestructor, zend_bool persistent ZEND_FILE_LINE_DC);
ZEND_API int _zend_hash_init_ex(HashTable *ht, uint nSize, hash_func_t pHashFunction, dtor_func_t pDestructor, zend_bool persistent, zend_bool bApplyProtection ZEND_FILE_LINE_DC);
ZEND_API void zend_hash_destroy(HashTable *ht);
ZEND_API void zend_hash_clean(HashTable *ht);
#define zend_hash_init(ht, nSize, pHashFunction, pDestructor, persistent)						_zend_hash_init((ht), (nSize), (pHashFunction), (pDestructor), (persistent) ZEND_FILE_LINE_CC)
#define zend_hash_init_ex(ht, nSize, pHashFunction, pDestructor, persistent, bApplyProtection)		_zend_hash_init_ex((ht), (nSize), (pHashFunction), (pDestructor), (persistent), (bApplyProtection) ZEND_FILE_LINE_CC)

/* additions/updates/changes */
ZEND_API int _zend_hash_add_or_update(HashTable *ht, const char *arKey, uint nKeyLength, void *pData, uint nDataSize, void **pDest, int flag ZEND_FILE_LINE_DC);
#define zend_hash_update(ht, arKey, nKeyLength, pData, nDataSize, pDest) \
		_zend_hash_add_or_update(ht, arKey, nKeyLength, pData, nDataSize, pDest, HASH_UPDATE ZEND_FILE_LINE_CC)
#define zend_hash_add(ht, arKey, nKeyLength, pData, nDataSize, pDest) \
		_zend_hash_add_or_update(ht, arKey, nKeyLength, pData, nDataSize, pDest, HASH_ADD ZEND_FILE_LINE_CC)

ZEND_API int _zend_hash_quick_add_or_update(HashTable *ht, const char *arKey, uint nKeyLength, ulong h, void *pData, uint nDataSize, void **pDest, int flag ZEND_FILE_LINE_DC);
#define zend_hash_quick_update(ht, arKey, nKeyLength, h, pData, nDataSize, pDest) \
		_zend_hash_quick_add_or_update(ht, arKey, nKeyLength, h, pData, nDataSize, pDest, HASH_UPDATE ZEND_FILE_LINE_CC)
#define zend_hash_quick_add(ht, arKey, nKeyLength, h, pData, nDataSize, pDest) \
		_zend_hash_quick_add_or_update(ht, arKey, nKeyLength, h, pData, nDataSize, pDest, HASH_ADD ZEND_FILE_LINE_CC)

ZEND_API int _zend_hash_index_update_or_next_insert(HashTable *ht, ulong h, void *pData, uint nDataSize, void **pDest, int flag ZEND_FILE_LINE_DC);
#define zend_hash_index_update(ht, h, pData, nDataSize, pDest) \
		_zend_hash_index_update_or_next_insert(ht, h, pData, nDataSize, pDest, HASH_UPDATE ZEND_FILE_LINE_CC)
#define zend_hash_next_index_insert(ht, pData, nDataSize, pDest) \
		_zend_hash_index_update_or_next_insert(ht, 0, pData, nDataSize, pDest, HASH_NEXT_INSERT ZEND_FILE_LINE_CC)

ZEND_API int zend_hash_add_empty_element(HashTable *ht, const char *arKey, uint nKeyLength);


#define ZEND_HASH_APPLY_KEEP				0
#define ZEND_HASH_APPLY_REMOVE				1<<0
#define ZEND_HASH_APPLY_STOP				1<<1

typedef int (*apply_func_t)(void *pDest TSRMLS_DC);
typedef int (*apply_func_arg_t)(void *pDest, void *argument TSRMLS_DC);
typedef int (*apply_func_args_t)(void *pDest TSRMLS_DC, int num_args, va_list args, zend_hash_key *hash_key);

ZEND_API void zend_hash_graceful_destroy(HashTable *ht);
ZEND_API void zend_hash_graceful_reverse_destroy(HashTable *ht);
ZEND_API void zend_hash_apply(HashTable *ht, apply_func_t apply_func TSRMLS_DC);
ZEND_API void zend_hash_apply_with_argument(HashTable *ht, apply_func_arg_t apply_func, void * TSRMLS_DC);
ZEND_API void zend_hash_apply_with_arguments(HashTable *ht TSRMLS_DC, apply_func_args_t apply_func, int, ...);

/* This function should be used with special care (in other words,
 * it should usually not be used).  When used with the ZEND_HASH_APPLY_STOP
 * return value, it assumes things about the order of the elements in the hash.
 * Also, it does not provide the same kind of reentrancy protection that
 * the standard apply functions do.
 */
ZEND_API void zend_hash_reverse_apply(HashTable *ht, apply_func_t apply_func TSRMLS_DC);


/* Deletes */
ZEND_API int zend_hash_del_key_or_index(HashTable *ht, const char *arKey, uint nKeyLength, ulong h, int flag);
#define zend_hash_del(ht, arKey, nKeyLength) \
		zend_hash_del_key_or_index(ht, arKey, nKeyLength, 0, HASH_DEL_KEY)
#define zend_hash_quick_del(ht, arKey, nKeyLength, h) \
		zend_hash_del_key_or_index(ht, arKey, nKeyLength, h, HASH_DEL_KEY_QUICK)
#define zend_hash_index_del(ht, h) \
		zend_hash_del_key_or_index(ht, NULL, 0, h, HASH_DEL_INDEX)

ZEND_API ulong zend_get_hash_value(const char *arKey, uint nKeyLength);

/* Data retreival */
ZEND_API int zend_hash_find(const HashTable *ht, const char *arKey, uint nKeyLength, void **pData);
ZEND_API int zend_hash_quick_find(const HashTable *ht, const char *arKey, uint nKeyLength, ulong h, void **pData);
ZEND_API int zend_hash_index_find(const HashTable *ht, ulong h, void **pData);

/* Misc */
ZEND_API int zend_hash_exists(const HashTable *ht, const char *arKey, uint nKeyLength);
ZEND_API int zend_hash_quick_exists(const HashTable *ht, const char *arKey, uint nKeyLength, ulong h);
ZEND_API int zend_hash_index_exists(const HashTable *ht, ulong h);
ZEND_API ulong zend_hash_next_free_element(const HashTable *ht);


/* traversing */
#define zend_hash_has_more_elements_ex(ht, pos) \
	(zend_hash_get_current_key_type_ex(ht, pos) == HASH_KEY_NON_EXISTANT ? FAILURE : SUCCESS)
ZEND_API int zend_hash_move_forward_ex(HashTable *ht, HashPosition *pos);
ZEND_API int zend_hash_move_backwards_ex(HashTable *ht, HashPosition *pos);
ZEND_API int zend_hash_get_current_key_ex(const HashTable *ht, char **str_index, uint *str_length, ulong *num_index, zend_bool duplicate, HashPosition *pos);
ZEND_API int zend_hash_get_current_key_type_ex(HashTable *ht, HashPosition *pos);
ZEND_API int zend_hash_get_current_data_ex(HashTable *ht, void **pData, HashPosition *pos);
ZEND_API void zend_hash_internal_pointer_reset_ex(HashTable *ht, HashPosition *pos);
ZEND_API void zend_hash_internal_pointer_end_ex(HashTable *ht, HashPosition *pos);
ZEND_API int zend_hash_update_current_key_ex(HashTable *ht, int key_type, const char *str_index, uint str_length, ulong num_index, int mode, HashPosition *pos);

typedef struct _HashPointer {
	HashPosition pos;
	ulong h;
} HashPointer;

ZEND_API int zend_hash_get_pointer(const HashTable *ht, HashPointer *ptr);
ZEND_API int zend_hash_set_pointer(HashTable *ht, const HashPointer *ptr);

#define zend_hash_has_more_elements(ht) \
	zend_hash_has_more_elements_ex(ht, NULL)
#define zend_hash_move_forward(ht) \
	zend_hash_move_forward_ex(ht, NULL)
#define zend_hash_move_backwards(ht) \
	zend_hash_move_backwards_ex(ht, NULL)
#define zend_hash_get_current_key(ht, str_index, num_index, duplicate) \
	zend_hash_get_current_key_ex(ht, str_index, NULL, num_index, duplicate, NULL)
#define zend_hash_get_current_key_type(ht) \
	zend_hash_get_current_key_type_ex(ht, NULL)
#define zend_hash_get_current_data(ht, pData) \
	zend_hash_get_current_data_ex(ht, pData, NULL)
#define zend_hash_internal_pointer_reset(ht) \
	zend_hash_internal_pointer_reset_ex(ht, NULL)
#define zend_hash_internal_pointer_end(ht) \
	zend_hash_internal_pointer_end_ex(ht, NULL)
#define zend_hash_update_current_key(ht, key_type, str_index, str_length, num_index) \
	zend_hash_update_current_key_ex(ht, key_type, str_index, str_length, num_index, HASH_UPDATE_KEY_ANYWAY, NULL)

/* Copying, merging and sorting */
ZEND_API void zend_hash_copy(HashTable *target, HashTable *source, copy_ctor_func_t pCopyConstructor, void *tmp, uint size);
ZEND_API void _zend_hash_merge(HashTable *target, HashTable *source, copy_ctor_func_t pCopyConstructor, void *tmp, uint size, int overwrite ZEND_FILE_LINE_DC);
ZEND_API void zend_hash_merge_ex(HashTable *target, HashTable *source, copy_ctor_func_t pCopyConstructor, uint size, merge_checker_func_t pMergeSource, void *pParam);
ZEND_API int zend_hash_sort(HashTable *ht, sort_func_t sort_func, compare_func_t compare_func, int renumber TSRMLS_DC);
ZEND_API int zend_hash_compare(HashTable *ht1, HashTable *ht2, compare_func_t compar, zend_bool ordered TSRMLS_DC);
ZEND_API int zend_hash_minmax(const HashTable *ht, compare_func_t compar, int flag, void **pData TSRMLS_DC);

#define zend_hash_merge(target, source, pCopyConstructor, tmp, size, overwrite)					\
	_zend_hash_merge(target, source, pCopyConstructor, tmp, size, overwrite ZEND_FILE_LINE_CC)

ZEND_API int zend_hash_num_elements(const HashTable *ht);

ZEND_API int zend_hash_rehash(HashTable *ht);

/*
 * DJBX33A (Daniel J. Bernstein, Times 33 with Addition)
 *
 * This is Daniel J. Bernstein's popular `times 33' hash function as
 * posted by him years ago on comp.lang.c. It basically uses a function
 * like ``hash(i) = hash(i-1) * 33 + str[i]''. This is one of the best
 * known hash functions for strings. Because it is both computed very
 * fast and distributes very well.
 *
 * The magic of number 33, i.e. why it works better than many other
 * constants, prime or not, has never been adequately explained by
 * anyone. So I try an explanation: if one experimentally tests all
 * multipliers between 1 and 256 (as RSE did now) one detects that even
 * numbers are not useable at all. The remaining 128 odd numbers
 * (except for the number 1) work more or less all equally well. They
 * all distribute in an acceptable way and this way fill a hash table
 * with an average percent of approx. 86%. 
 *
 * If one compares the Chi^2 values of the variants, the number 33 not
 * even has the best value. But the number 33 and a few other equally
 * good numbers like 17, 31, 63, 127 and 129 have nevertheless a great
 * advantage to the remaining numbers in the large set of possible
 * multipliers: their multiply operation can be replaced by a faster
 * operation based on just one shift plus either a single addition
 * or subtraction operation. And because a hash function has to both
 * distribute good _and_ has to be very fast to compute, those few
 * numbers should be preferred and seems to be the reason why Daniel J.
 * Bernstein also preferred it.
 *
 *
 *                  -- Ralf S. Engelschall <rse@engelschall.com>
 */

static inline ulong zend_inline_hash_func(const char *arKey, uint nKeyLength)
{
	register ulong hash = 5381;

	/* variant with the hash unrolled eight times */
	for (; nKeyLength >= 8; nKeyLength -= 8) {
		hash = ((hash << 5) + hash) + *arKey++;
		hash = ((hash << 5) + hash) + *arKey++;
		hash = ((hash << 5) + hash) + *arKey++;
		hash = ((hash << 5) + hash) + *arKey++;
		hash = ((hash << 5) + hash) + *arKey++;
		hash = ((hash << 5) + hash) + *arKey++;
		hash = ((hash << 5) + hash) + *arKey++;
		hash = ((hash << 5) + hash) + *arKey++;
	}
	switch (nKeyLength) {
		case 7: hash = ((hash << 5) + hash) + *arKey++; /* fallthrough... */
		case 6: hash = ((hash << 5) + hash) + *arKey++; /* fallthrough... */
		case 5: hash = ((hash << 5) + hash) + *arKey++; /* fallthrough... */
		case 4: hash = ((hash << 5) + hash) + *arKey++; /* fallthrough... */
		case 3: hash = ((hash << 5) + hash) + *arKey++; /* fallthrough... */
		case 2: hash = ((hash << 5) + hash) + *arKey++; /* fallthrough... */
		case 1: hash = ((hash << 5) + hash) + *arKey++; break;
		case 0: break;
EMPTY_SWITCH_DEFAULT_CASE()
	}
	return hash;
}


ZEND_API ulong zend_hash_func(const char *arKey, uint nKeyLength);

#if ZEND_DEBUG
/* debug */
void zend_hash_display_pListTail(const HashTable *ht);
void zend_hash_display(const HashTable *ht);
#endif

END_EXTERN_C()

#define ZEND_INIT_SYMTABLE(ht)								\
	ZEND_INIT_SYMTABLE_EX(ht, 2, 0)

#define ZEND_INIT_SYMTABLE_EX(ht, n, persistent)			\
	zend_hash_init(ht, n, NULL, ZVAL_PTR_DTOR, persistent)

#define ZEND_HANDLE_NUMERIC_EX(key, length, idx, func) do {					\
	register const char *tmp = key;											\
																			\
	if (*tmp == '-') {														\
		tmp++;																\
	}																		\
	if (*tmp >= '0' && *tmp <= '9') { /* possibly a numeric index */		\
		const char *end = key + length - 1;									\
																			\
		if ((*end != '\0') /* not a null terminated string */				\
		 || (*tmp == '0' && length > 2) /* numbers with leading zeros */	\
		 || (end - tmp > MAX_LENGTH_OF_LONG - 1) /* number too long */		\
		 || (SIZEOF_LONG == 4 &&											\
		     end - tmp == MAX_LENGTH_OF_LONG - 1 &&							\
		     *tmp > '2')) { /* overflow */									\
			break;															\
		}																	\
		idx = (*tmp - '0');													\
		while (++tmp != end && *tmp >= '0' && *tmp <= '9') {				\
			idx = (idx * 10) + (*tmp - '0');								\
		}																	\
		if (tmp == end) {													\
			if (*key == '-') {												\
				if (idx-1 > LONG_MAX) { /* overflow */						\
					break;													\
				}															\
				idx = 0 - idx;               									\
			} else if (idx > LONG_MAX) { /* overflow */						\
				break;														\
			}																\
			func;															\
		}																	\
	}																		\
} while (0)

#define ZEND_HANDLE_NUMERIC(key, length, func) do {							\
	ulong idx;																\
																			\
	ZEND_HANDLE_NUMERIC_EX(key, length, idx, return func);					\
} while (0)

static inline int zend_symtable_update(HashTable *ht, const char *arKey, uint nKeyLength, void *pData, uint nDataSize, void **pDest)					\
{
	ZEND_HANDLE_NUMERIC(arKey, nKeyLength, zend_hash_index_update(ht, idx, pData, nDataSize, pDest));
	return zend_hash_update(ht, arKey, nKeyLength, pData, nDataSize, pDest);
}


static inline int zend_symtable_del(HashTable *ht, const char *arKey, uint nKeyLength)
{
	ZEND_HANDLE_NUMERIC(arKey, nKeyLength, zend_hash_index_del(ht, idx));
	return zend_hash_del(ht, arKey, nKeyLength);
}


static inline int zend_symtable_find(HashTable *ht, const char *arKey, uint nKeyLength, void **pData)
{
	ZEND_HANDLE_NUMERIC(arKey, nKeyLength, zend_hash_index_find(ht, idx, pData));
	return zend_hash_find(ht, arKey, nKeyLength, pData);
}


static inline int zend_symtable_exists(HashTable *ht, const char *arKey, uint nKeyLength)
{
	ZEND_HANDLE_NUMERIC(arKey, nKeyLength, zend_hash_index_exists(ht, idx));
	return zend_hash_exists(ht, arKey, nKeyLength);
}

static inline int zend_symtable_update_current_key_ex(HashTable *ht, const char *arKey, uint nKeyLength, int mode, HashPosition *pos)
{
	ZEND_HANDLE_NUMERIC(arKey, nKeyLength, zend_hash_update_current_key_ex(ht, HASH_KEY_IS_LONG, NULL, 0, idx, mode, pos));
	return zend_hash_update_current_key_ex(ht, HASH_KEY_IS_STRING, arKey, nKeyLength, 0, mode, pos);
}
#define zend_symtable_update_current_key(ht,arKey,nKeyLength,mode) \
	zend_symtable_update_current_key_ex(ht, arKey, nKeyLength, mode, NULL)


#endif							/* ZEND_HASH_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * indent-tabs-mode: t
 * End:
 */
