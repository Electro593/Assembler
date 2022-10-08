/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *\
**                                                                         **
**  Author: Aria Seiler                                                    **
**                                                                         **
**  This program is in the public domain. There is no implied warranty,    **
**  so use it at your own risk.                                            **
**                                                                         **
\* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

typedef s08 (cmp_func)(vptr A, vptr B, vptr Param);
typedef u64 (hash_func)(vptr Data);

// Hashing overview:
// Quadratic probing, resizing to the next largest entry in the prime number
// lookup table.
// If the table's size exceeds the largest stored prime number, we'll switch
// to linear probing. It'll be slow, but it'll work.

#define MAX_PRIMES 10000
#define MAX_PRIME Primes[MAX_PRIMES-1]

//CREDIT: https://primes.utm.edu/lists/small/10000.txt

#define HASH_HASH    0x7FFFFFFFFFFFFFFF
#define HASH_EXISTS  0x8000000000000000
#define HASH_DELETED 0x4000000000000000

typedef struct hashmap_header {
   u64 Hash;
} hashmap_header;

typedef struct hashmap {
   heap_handle *Array;
   u32 EntryCount;
   u32 EntrySize;
   u32 KeySize;
   u32 ValueSize;
   r32 ResizeRate;
   r32 ResizeThresh;
   
   vptr CmpParam;
   cmp_func *Cmp;
   hash_func *Hash;
} hashmap;