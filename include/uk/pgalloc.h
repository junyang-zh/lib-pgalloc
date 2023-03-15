#ifndef __LIBPGALLOC_H__
#define __LIBPGALLOC_H__

#include <uk/alloc.h>
#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif

struct uk_alloc *uk_pgalloc_init(void *base, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* __LIBPGALLOC_H__ */