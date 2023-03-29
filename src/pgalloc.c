#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>
#include <uk/alloc_impl.h>
#include <uk/page.h>	/* round_pgup() */
#include <uk/assert.h>
// #include <uk/plat/spinlock.h>

#include "tlsf.h"

/* Size of an arena is designed larger than the size of a huge page (8 MiB)*/
#define ARENA_SIZE (1ul << 23)
#define ARENA(b, i) (b->arena_base + i * ARENA_SIZE)

// Make mallocs atomic
/*
static __spinlock pgalloc_spinlock = UKARCH_SPINLOCK_INITIALIZER();

static unsigned long pgalloc_lock(void) {
	unsigned long flags;
	ukplat_spin_lock_irqsave(&pgalloc_spinlock, flags);
	return flags;
}

static void pgalloc_unlock(unsigned long flags) {
	ukplat_spin_unlock_irqrestore(&pgalloc_spinlock, flags);
}
*/

// Shit random generator, shouldn't be here
static struct pgalloc_rng_t {
	__u64 a;
	__u64 m;
	__u64 seed;
} pgalloc_rng = { 16807, 2147483647, 0 };

static __u32 random_int() {
	pgalloc_rng.seed += 1;
	return pgalloc_rng.seed;
}

typedef struct pgsched_t {
	__u8 *arena_base;
	size_t num_arena;
} pgsched_t;

static inline size_t locate_arena(pgsched_t *pgsched, void *obj) {
	return ((__u8 *)obj - pgsched->arena_base) / ARENA_SIZE;
}

void *pgalloc_malloc(struct uk_alloc *a, size_t size) {
	// unsigned long lock_flags = pgalloc_lock();
	pgsched_t *b = (pgsched_t *)(a->priv);
	size_t arena = random_int() % b->num_arena;
	void *obj = tlsf_malloc(size, ARENA(b, arena));
	// uk_pr_err("malloc %zu bytes at %p, in arena %zu\n", size, obj, arena);
	// pgalloc_unlock(lock_flags);
	return obj;
}

void pgalloc_free(struct uk_alloc *a, void *obj) {
	// unsigned long lock_flags = pgalloc_lock();
	pgsched_t *b = (pgsched_t *)(a->priv);
	// uk_pr_err("free at %p, in arena %zu\n", obj, locate_arena(b, obj));
	tlsf_free(obj, ARENA(b, locate_arena(b, obj)));
	// pgalloc_unlock(lock_flags);
}

struct uk_alloc *uk_pgalloc_init(void *base, size_t len)
{
	struct uk_alloc *a;
	size_t i, num_arena, res;
	__u8 *arena_base;
	pgsched_t *b;

	/* enough space for allocator available?
		pgalloc requires at least an arena.
	*/
	if (sizeof(*a) + sizeof(*b) + ARENA_SIZE > len) {
		uk_pr_err("Not enough space for allocator: %" __PRIsz
			  " B required but only %" __PRIuptr" B usable\n",
			  sizeof(*a) + sizeof(*b), len);
		return NULL;
	}

	/* store allocator metadata on the heap, just before the memory pool */
	a = (struct uk_alloc *)base;
	uk_pr_info("Initialize pgalloc allocator @ 0x%" __PRIuptr ", len %"
			__PRIsz"\n", (uintptr_t)a, len);

	b = (struct pgsched_t *)a->priv;

	num_arena = len / ARENA_SIZE;
	arena_base = round_pgup((uintptr_t)base + sizeof(*a) + sizeof(*b));

	for (i = 0; i < num_arena; ++i) {
		if (arena_base + i * ARENA_SIZE >= base + len) {
			break;
		}
		res = init_memory_pool(ARENA_SIZE, arena_base + i * ARENA_SIZE);
		if (res == (size_t)-1)
			return NULL;
	}

	UK_ASSERT(i > 0);

	b->arena_base = arena_base;
	b->num_arena = i;

	uk_alloc_init_malloc_ifmalloc(a, pgalloc_malloc, pgalloc_free,
		NULL /* maxalloc */, NULL /* availmem */, NULL /* addmem */);

	return a;
}
