#define likely(x) 	__builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

#define offsetof(type, member) __builtin_offsetof(type, member)

#define container_of(ptr, type, member) ({	\
	void *__mptr = (void *)(ptr);			\
	((type *)(__mptr - offsetof(type, member))); })

#define min(a, b) (((a) < (b)) ? (a) : (b))
#define max(a, b) (((a) > (b)) ? (a) : (b))
