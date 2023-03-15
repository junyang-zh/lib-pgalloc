#include <uk/bbuddy_arena.h>
#include <stddef.h>
#include <sys/types.h>
#include <uk/allocregion.h>
#include <uk/alloc_impl.h>
#include <uk/page.h>	/* round_pgup() */

void *pgalloc_palloc(struct uk_alloc *a, unsigned long num_pages) {
	bbuddy_palloc(a, num_pages);
}

void pgalloc_pfree(struct uk_alloc *a, void *obj, unsigned long num_pages) {
	bbuddy_pfree(a, obj, num_pages);
}

long pgalloc_pmaxalloc(struct uk_alloc *a) {
	return bbuddy_pmaxalloc(a);
}

long pgalloc_pavailmem(struct uk_alloc *a) {
	return bbuddy_pavailmem(a);
}

int pgalloc_addmem(struct uk_alloc *a, void *base, size_t len) {
	return bbuddy_addmem(a, base, len);
}

struct uk_alloc *uk_pgalloc_init(void *base, size_t len)
{
	/* TODO: ukallocregion does not support multiple memory regions yet.
	 * Because of the multiboot layout, the first region might be a single
	 * page, so we simply ignore it.
	 */
	if (len <= __PAGE_SIZE)
		return NULL;

	struct uk_alloc *a = uk_bbuddy_arena_init(base, len);

	uk_pr_info("Initialize pgalloc allocator @ 0x%"
		   __PRIuptr ", len %"__PRIsz"\n", (uintptr_t)a, len);

	uk_alloc_init_palloc(a, pgalloc_palloc, pgalloc_pfree,
			     pgalloc_pmaxalloc, pgalloc_pavailmem,
			     pgalloc_addmem);

	return a;
}
