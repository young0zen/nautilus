/* 
 * This file is part of the Nautilus AeroKernel developed
 * by the Hobbes and V3VEE Projects with funding from the 
 * United States National  Science Foundation and the Department of Energy.  
 *
 * The V3VEE Project is a joint project between Northwestern University
 * and the University of New Mexico.  The Hobbes Project is a collaboration
 * led by Sandia National Laboratories that includes several national 
 * laboratories and universities. You can find out more at:
 * http://www.v3vee.org  and
 * http://xstack.sandia.gov/hobbes
 *
 * Copyright (c) 2019, Peter Dinda
 * Copyright (c) 2019, The V3VEE Project  <http://www.v3vee.org> 
 *                     The Hobbes Project <http://xstack.sandia.gov/hobbes>
 * All rights reserved.
 *
 * Author: Peter Dinda <pdinda@northwestern.edu>
 *
 * This is free software.  You are permitted to use,
 * redistribute, and modify it as specified in the file "LICENSE.txt".
 */


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


static nk_aspace_impl_t base = {
				.impl_name = "base",
				.get_characteristics = get_characteristics,
				.create = create,
};

nk_aspace_register_impl(base);





#if 0
#include <nautilus/nautilus.h>
#include <nautilus/spinlock.h>
#include <nautilus/aspace.h>
#ifdef NAUT_CONFIG_ASPACE_PAGING
#include <nautilus/aspace/paging.h>
#endif
#ifdef NAUT_CONFIG_ASPACE_CARAT
#include <nautilus/aspace/carat.h>
#endif

#include <nautilus/paging.h>
#include <nautilus/thread.h>
#include <nautilus/shell.h>


static spinlock_t state_lock;

#define STATE_LOCK_CONF uint8_t _state_lock_flags
#define STATE_LOCK() _state_lock_flags = spin_lock_irq_save(&state_lock)
#define STATE_UNLOCK() spin_unlock_irq_restore(&state_lock, _state_lock_flags);

#define ASPACE_LOCK_CONF uint8_t _aspace_lock_flags
#define ASPACE_LOCK(a) _aspace_lock_flags = spin_lock_irq_save(a->lock)
#define ASPACE_UNLOCK(a) spin_unlock_irq_restore(a->lock, _aspace_lock_flags);

#define THREAD_LOCK_CONF uint8_t _thread_lock_flags
#define THREAD_LOCK(t) _thread_lock_flags = spin_lock_irq_save(t->lock)
#define THREAD_UNLOCK(t) spin_unlock_irq_restore(t->lock, _thread_lock_flags);

static struct list_head aspace_list;

static nk_aspace_t base;

nk_aspace_region_t base_reg;


int nk_aspace_init_aspace(nk_aspace_t *a, char *name)
{
    memset(a,0,sizeof(nk_aspace_t));
    spinklock_init(&a.lock);
    
    strncpy(a->name,name,32); a->name[NK_ASPACE_NAME_LEN-1]=0;
    INIT_LIST_HEAD(&a->region_list);
    INIT_LIST_HEAD(&a->thread_list);
    INIT_LIST_HEAD(&a->aspace_list_node);

    return 0;
}




int nk_aspace_query(nk_aspace_type_t type, nk_aspace_characteristics_t *chars)
{
    switch (type) {
    case NK_ASPACE_BASE:
	*chars = base.chars;
	return 0;
#ifdef NAUT_CONFIG_ASPACE_PAGING
    case NK_ASPACE_PAGING:
	return nk_aspace_query_paging(chars);
	break;
#endif
#ifdef NAUT_CONFIG_ASPACE_CARAT
    case NK_ASPACE_CARAT:
	return nk_aspace_query_carat(chars);
	break;
#endif
    default:
	AS_ERROR("unknown type %d\n",type);
	return -1;
    }

}

nk_aspace_t *nk_aspace_create(nk_aspace_type type, char *name)
{
    switch (type) {
    case NK_ASPACE_BASE:
	AS_ERROR("Attempt to create new base aspace\n");
	return 0;
#ifdef NAUT_CONFIG_ASPACE_PAGING
    case NK_ASPACE_PAGING:
	return nk_aspace_create_paging(name);
	break;
#endif
#ifdef NAUT_CONFIG_ASPACE_CARAT
    case NK_ASPACE_CARAT:
	return nk_aspace_query_carat(name);
	break;
#endif
    default:
	AS_ERROR("unknown type %d\n",type);
	return -1;
    }
}

int          nk_aspace_destroy(nk_aspace_t *aspace)
{
    switch (aspace->type) {
    case NK_ASPACE_BASE:
	AS_ERROR("Attempt to destory new base aspace\n");
	return -1;
#ifdef NAUT_CONFIG_ASPACE_PAGING
    case NK_ASPACE_PAGING:
	return nk_aspace_destroy_paging((nk_aspace_paging_t *)aspace);
	break;
#endif
#ifdef NAUT_CONFIG_ASPACE_CARAT
    case NK_ASPACE_CARAT:
	return nk_aspace_destroy_carat((nk_aspace_carat_t *)aspace);
	break;
#endif
    default:
	AS_ERROR("unknown type %d\n",type);
	return -1;
    }
}
    


nk_aspace_t *nk_aspace_base()
{
    return &base;
}

nk_aspace_t *nk_aspace_find(char *name)
{
    struct list_head *cur;
    nk_aspace_t  *target=0;
    
    STATE_LOCK_CONF;
    STATE_LOCK();
    list_for_each(cur,&aspace_list) {
	if (!strncasecmp(list_entry(cur,struct nk_aspace,aspace_listnode)->name,name,NK_ASPACE_NAME_LEN)) { 
	    target = list_entry(cur,struct nk_aspace, aspace_list_node);
	    break;
	}
    }
    STATE_UNLOCK();
    return target;
}


// assumes we have interrupts off
// and the thread being lost is us
int          nk_aspace_losing_thread(nk_aspace_t *aspace)
{
    switch (aspace) {
    case NK_ASPACE_BASE: {
	nk_thread_t *t = get_cur_thread();
	nk_aspace_t *a = t->aspace;
	ASPACE_LOCK_CONF;
	THREAD_LOCK_CONF;
	
	ASPACE_LOCK(a);
	THREAD_LOCK(t);

	list_del_init(t->aspace_node);
	t->aspace = 0;

	THREAD_UNLOCK(t);
	ASPACE_UNLOCK(a);
	
	return 0;
    }
#ifdef NAUT_CONFIG_ASPACE_PAGING
    case NK_ASPACE_PAGING:
	return nk_aspace_losing_thread_paging((nk_aspace_paging_t *)aspace);
	break;
#endif
#ifdef NAUT_CONFIG_ASPACE_CARAT
    case NK_ASPACE_CARAT:
	return nk_aspace_losing_thread_carat((nk_aspace_carat_t *)aspace);
	break;
#endif
    default:
	AS_ERROR("unknown type %d\n",type);
	return -1;
    }
}

int          nk_aspace_move_thread(nk_aspace_t *aspace)
{

    switch (aspace->type) {
    case NK_ASPACE_BASE: {
	nk_thread_t *t = get_cur_thread();
	uint8_t flags;
	THREAD_LOCK_CONF;
	ASPACE_LOCK_CONF;

	// dangerous if interrupts are off
	AS_DEBUG("moving thread %d (%s) from %p (%s) to %p (%s)\n",t->tid,t->name,
		 t->aspace, t->aspace->name, aspace, aspace->name);
	

	flags = irq_disable_save();

	// we are now going to keep running as long as needed
	// and we are unstealable

	// old address space is losing thread
	nk_aspace_inform_lost_thread(t->aspace);

	// we still have interrupts off
	

	ASPACE_LOCK(a);
	THREAD_LOCK(t);

	list_add_tail(&t->aspace_node,&aspace->thread_list);
	t->aspace = aspace;

	THREAD_UNLOCK(t);
	ASPACE_UNLOCK(a);

	// now actually do the switch

	nk_aspace_switch(a);

	AS_DEBUG("thread %d (%s) is now in %p (%s)\n",t->tid,t->name, t->aspace,t->aspace->name);

	return 0;
    }
	break;
#ifdef NAUT_CONFIG_ASPACE_PAGING
    case NK_ASPACE_PAGING:
	return nk_aspace_move_thread_paging((nk_aspace_paging_t *)aspace);
	break;
#endif
#ifdef NAUT_CONFIG_ASPACE_CARAT
    case NK_ASPACE_CARAT:
	return nk_aspace_move_thread_carat((nk_aspace_carat_t *)aspace);
	break;
#endif
    default:
	AS_ERROR("unknown type %d\n",type);
	return -1;
    }
}

int          nk_aspace_add_region(nk_aspace_t *aspace, nk_aspace_region_t *region)
{
    // add then inform
    ASPACE_LOCK_CONF;
    int rc;

    ASPACE_LOCK(aspace);

    list_add_tail(&region->node,&aspace->region_list);

    
    // now do informs
    switch (aspace->type) {
    case NK_ASPACE_BASE:
	rc = 0;
	break;
#ifdef NAUT_CONFIG_ASPACE_PAGING
    case NK_ASPACE_PAGING:
	rc = nk_aspace_add_region_paging((nk_aspace_paging_t *)aspace,region);
	break;
#endif
#ifdef NAUT_CONFIG_ASPACE_CARAT
    case NK_ASPACE_CARAT:
	rc = nk_aspace_add_region_carat((nk_aspace_carat_t *)aspace,region);
	break;
#endif
    default:
	AS_ERROR("unknown type %d\n",type);
	rc = -1;
	break;
    }

    ASPACE_UNLOCK(aspace);
    return rc;
}

int          nk_aspace_remove_region(nk_aspace_t *aspace, nk_aspace_region_t *region)
{
    // add then inform
    ASPACE_LOCK_CONF;
    int rc;

    ASPACE_LOCK(aspace);

    // now do informs
    switch (aspace->type) {
    case NK_ASPACE_BASE:
	rc = 0;
	break;
#ifdef NAUT_CONFIG_ASPACE_PAGING
    case NK_ASPACE_PAGING:
	rc = nk_aspace_remove_region_paging((nk_aspace_paging_t *)aspace,region);
	break;
#endif
#ifdef NAUT_CONFIG_ASPACE_CARAT
    case NK_ASPACE_CARAT:
	rc = nk_aspace_remove_region_carat((nk_aspace_carat_t *)aspace,region);
	break;
#endif
    default:
	AS_ERROR("unknown type %d\n",type);
	rc = -1;
	break;
    }

    list_del_init(&region->node);
    
    ASPACE_UNLOCK(aspace);
    return rc;
}

int          nk_aspace_invalidate_region(nk_aspace_t *aspace, nk_aspace_region_t *region)
{
    // add then inform
    ASPACE_LOCK_CONF;
    int rc;

    ASPACE_LOCK(aspace);

    // now do informs
    switch (aspace->type) {
    case NK_ASPACE_BASE:
	rc = 0;
	break;
#ifdef NAUT_CONFIG_ASPACE_PAGING
    case NK_ASPACE_PAGING:
	rc = nk_aspace_invalidate_region_paging((nk_aspace_paging_t *)aspace,region);
	break;
#endif
#ifdef NAUT_CONFIG_ASPACE_CARAT
    case NK_ASPACE_CARAT:
	rc = nk_aspace_invalidate_region_carat((nk_aspace_carat_t *)aspace,region);
	break;
#endif
    default:
	AS_ERROR("unknown type %d\n",type);
	rc = -1;
	break;
    }

    ASPACE_UNLOCK(aspace);
    return rc;
}

int          nk_aspace_validate_region(nk_aspace_t *aspace, nk_aspace_region_t *region)
{
    // add then inform
    ASPACE_LOCK_CONF;
    int rc;

    ASPACE_LOCK(aspace);

    // now do informs
    switch (aspace->type) {
    case NK_ASPACE_BASE:
	rc = 0;
	break;
#ifdef NAUT_CONFIG_ASPACE_PAGING
    case NK_ASPACE_PAGING:
	rc = nk_aspace_validate_region_paging((nk_aspace_paging_t *)aspace,region);
	break;
#endif
#ifdef NAUT_CONFIG_ASPACE_CARAT
    case NK_ASPACE_CARAT:
	rc = nk_aspace_validate_region_carat((nk_aspace_carat_t *)aspace,region);
	break;
#endif
    default:
	AS_ERROR("unknown type %d\n",type);
	rc = -1;
	break;
    }

    ASPACE_UNLOCK(aspace);
    return rc;
}


int          nk_aspace_move_region_virtual(nk_space_t *aspace, nk_aspace_region_t *region, void *new_va_start)
{
    ASPACE_LOCK_CONF;
    int rc;

    ASPACE_LOCK(aspace);

    // now do informs
    switch (aspace->type) {
    case NK_ASPACE_BASE:
	AS_ERROR("Cannot move virtual regions on base aspace\n");
	rc = -1;
	break;
#ifdef NAUT_CONFIG_ASPACE_PAGING
    case NK_ASPACE_PAGING:
	rc = nk_aspace_move_region_virtual_paging((nk_aspace_paging_t *)aspace,region,new_va_start);
	break;
#endif
#ifdef NAUT_CONFIG_ASPACE_CARAT
    case NK_ASPACE_CARAT:
	rc = nk_aspace_move_region_virtual_carat((nk_aspace_carat_t *)aspace,region,new_va_start);
	break;
#endif
    default:
	AS_ERROR("unknown type %d\n",type);
	rc = -1;
	break;
    }

    ASPACE_UNLOCK(aspace);
    return rc;
}

int          nk_aspace_move_region_physical(nk_space_t *aspace, nk_aspace_region_t *region, void *new_pa_start)
{
    ASPACE_LOCK_CONF;
    int rc;

    ASPACE_LOCK(aspace);

    // now do informs
    switch (aspace->type) {
    case NK_ASPACE_BASE:
	AS_ERROR("Cannot move physical regions on base aspace\n");
	rc = -1;
	break;
#ifdef NAUT_CONFIG_ASPACE_PAGING
    case NK_ASPACE_PAGING:
	rc = nk_aspace_move_region_physical_paging((nk_aspace_paging_t *)aspace,region,new_va_start);
	break;
#endif
#ifdef NAUT_CONFIG_ASPACE_CARAT
    case NK_ASPACE_CARAT:
	rc = nk_aspace_move_region_physical_carat((nk_aspace_carat_t *)aspace,region,new_va_start);
	break;
#endif
    default:
	AS_ERROR("unknown type %d\n",type);
	rc = -1;
	break;
    }

    ASPACE_UNLOCK(aspace);
    return rc;
}


int nk_aspace_init()
{
    INIT_LIST_HEAD(&aspace_list);
    spinlock_init(&state_lock);

    nk_aspace_init_aspace(&base,"(base)");
    base.type = NK_ASPACE_BASE;
    base.chars.granularity = PAGE_SIZE;
    base.chars.alignment = PAGE_SIZE;

    base_reg.va_start = 0;
    base_reg.pa_start = 0;
    base_reg.len_bytes = mm_boot_last_pfn() * PAGE_SIZE;
    INIT_LIST_HEAD(&base_reg.node);
    
    list_add_tail(&base_reg.node,&base.region_list);
    list_add_tail(&base.aspace_list_node,&aspace_list);
    
    INFO("inited\n");
    return 0;
}

int nk_aspace_init_ap()
{
    INFO("inited\n");
}

int nk_aspace_deinit()
{
    spinlock_deinit(&state_lock);
    INFO("deinit\n");
    return 0;
}


void nk_aspace_dump_aspaces()
{
    struct list_head *cur;
    STATE_LOCK_CONF;
    STATE_LOCK();
    list_for_each(cur,&aspace_list) {
	struct nk_aspace *a = list_entry(cur,struct nk_aspace, aspace_list_node);
	struct list_head *cr;
	nk_vc_printf("%s: %s \n", 
		     a->name, 
		     a->type==NK_ASPACE_BASE ? "base" :
		     a->type==NK_ASPACE_PAGING ? "paging" : 
		     a->type==NK_ASPACE_CARAT ? "carat" : "unknown");
	list_for_each(cr,&a->region_list) {
	    struct nk_aspace_region *r = list_entry(cr,struct nk_aspace_region, node);
	    nk_vc_printf("   [%016lx, %016lx) => %016lx\n", r->va_start, r->va_start+r->len_bytes,r->pa_start);
	}
		     
    }
    STATE_UNLOCK();
}


static int
handle_ases (char * buf, void * priv)
{
    nk_aspace_dump_aspaces();
    return 0;
}


static struct shell_cmd_impl ases_impl = {
    .cmd      = "ases",
    .help_str = "ases",
    .handler  = handle_ases,
};
nk_register_shell_cmd(ases_impl);

#endif
