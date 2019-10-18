#include <nautilus/nautilus.h>
#include <nautilus/spinlock.h>
#include <nautilus/paging.h>
#include <nautilus/thread.h>
#include <nautilus/shell.h>

#include <nautilus/aspace.h>

#ifndef NAUT_CONFIG_DEBUG_ASPACE_BASE
#undef DEBUG_PRINT
#define DEBUG_PRINT(fmt, args...) 
#endif

#define ERROR(fmt, args...) ERROR_PRINT("aspace-base: ERROR %s(%d): " fmt, __FILE__, __LINE__, ##args)
#define DEBUG(fmt, args...) DEBUG_PRINT("aspace-base: DEBUG: " fmt, ##args)
#define INFO(fmt, args...)   INFO_PRINT("aspace-base: " fmt, ##args)


static int   get_characteristics(nk_aspace_characteristics_t *c)
{
    return -1;
}

static struct nk_aspace * create(char *name, nk_aspace_characteristics_t *c)
{
    return 0;
}


static nk_aspace_impl_t paging = {
				.impl_name = "paging",
				.get_characteristics = get_characteristics,
				.create = create,
};

nk_aspace_register_impl(paging);


