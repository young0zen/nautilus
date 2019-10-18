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

#ifndef NAUT_CONFIG_DEBUG_ASPACE
#undef DEBUG_PRINT
#define DEBUG_PRINT(fmt, args...) 
#endif

#define ERROR(fmt, args...) ERROR_PRINT("aspace: ERROR %s(%d): " fmt, __FILE__, __LINE__, ##args)
#define DEBUG(fmt, args...) DEBUG_PRINT("aspace: DEBUG: " fmt, ##args)
#define INFO(fmt, args...)   INFO_PRINT("aspace: " fmt, ##args)



static spinlock_t state_lock;
static struct list_head aspace_list;

#define STATE_LOCK_CONF uint8_t _state_lock_flags
#define STATE_LOCK() _state_lock_flags = spin_lock_irq_save(&state_lock)
#define STATE_UNLOCK() spin_unlock_irq_restore(&state_lock, _state_lock_flags);


// this is to be
nk_aspace_t *nk_aspace_register(char *name, uint64_t flags, nk_aspace_interface_t *interface, void *state)
{
    nk_aspace_t *a;

    a = malloc(sizeof(*a));

    if (!a) {
	ERROR("cannot allocate aspace\n");
	return 0;
    }

    memset(a,0,sizeof(*a));

    a->flags = flags;
    strncpy(a->name,name,NK_ASPACE_NAME_LEN); a->name[NK_ASPACE_NAME_LEN-1]=0;
    a->state = state;
    a->interface = interface;

    INIT_LIST_HEAD(&a->aspace_list_node);

    STATE_LOCK_CONF;
    
    STATE_LOCK();
    list_add_tail(&a->aspace_list_node, &aspace_list);
    STATE_UNLOCK();

    return a;
}


int nk_aspace_unregister(nk_aspace_t *a)
{
    STATE_LOCK_CONF;
    
    STATE_LOCK();
    list_del_init(&a->aspace_list_node);
    STATE_UNLOCK();

    free(a);
    
    return 0;
}

static nk_aspace_impl_t *find_impl(char *impl_name)
{
    extern nk_aspace_impl_t *__start_aspace_impls;
    extern nk_aspace_impl_t * __stop_aspace_impls;
    nk_aspace_impl_t **cur;

    for (cur = &__start_aspace_impls;
	 cur && cur!=&__stop_aspace_impls && *cur;
	 cur++) {
	if (!strcmp((*cur)->impl_name,impl_name)) {
	    return *cur;
	}
    }
    return 0;
}


int nk_aspace_query(char *impl_name, nk_aspace_characteristics_t *chars)
{
    nk_aspace_impl_t *impl = find_impl(impl_name);

    if (impl && impl->get_characteristics) {
	return impl->get_characteristics(chars);
    } else {
	return -1;
    }
}



nk_aspace_t *nk_aspace_create(char *impl_name, char *name, nk_aspace_characteristics_t *chars)
{
    nk_aspace_impl_t *impl = find_impl(impl_name);

    if (impl && impl->create) {
	return impl->create(name, chars);
    } else {
	return 0;
    }
}

#define BOILERPLATE(a,f,args...)                       \
    if (a->interface && a->interface->f) {             \
       return a->interface->f(a->state, ##args);       \
    } else {                                           \
       return -1;                                      \
    }

int  nk_aspace_destroy(nk_aspace_t *aspace)
{
    BOILERPLATE(aspace,destroy)
}
    
nk_aspace_t *nk_aspace_find(char *name)
{
    struct list_head *cur;
    nk_aspace_t  *target=0;
    
    STATE_LOCK_CONF;
    STATE_LOCK();
    list_for_each(cur,&aspace_list) {
	if (!strcmp(list_entry(cur,struct nk_aspace,aspace_list_node)->name,name)) { 
	    target = list_entry(cur,struct nk_aspace, aspace_list_node);
	    break;
	}
    }
    STATE_UNLOCK();
    return target;
}


int nk_aspace_move_thread(nk_aspace_t *aspace)
{

    nk_thread_t *t = get_cur_thread();
    uint8_t flags;

    // dangerous if interrupts are off
    DEBUG("moving thread %d (%s) from %p (%s) to %p (%s)\n",t->tid,t->name,
	  t->aspace, t->aspace->name, aspace, aspace->name);
	

    flags = irq_disable_save();

    // we are now going to keep running as long as needed
    // and we are unstealable
    
    // old address space is losing thread
    BOILERPLATE(t->aspace,remove_thread);

    // new address space is gaining it
    BOILERPLATE(aspace,add_thread);


    // switch the thread's view
    t->aspace = aspace;
    
    // now actually do the switch

    nk_aspace_switch(aspace);

    DEBUG("thread %d (%s) is now in %p (%s)\n",t->tid,t->name, t->aspace,t->aspace->name);

    return 0;
}

int  nk_aspace_add_region(nk_aspace_t *aspace, nk_aspace_region_t *region)
{
    BOILERPLATE(aspace,add_region,region);
}

int  nk_aspace_remove_region(nk_aspace_t *aspace, nk_aspace_region_t *region)
{
    BOILERPLATE(aspace,remove_region,region);
}

int  nk_aspace_protect_region(nk_aspace_t *aspace, nk_aspace_region_t *region, nk_aspace_protection_t *prot)
{
    BOILERPLATE(aspace,protect_region,region,prot);
}


int  nk_aspace_move_region(nk_aspace_t *aspace, nk_aspace_region_t *cur_region, nk_aspace_region_t *new_region)
{
    BOILERPLATE(aspace,move_region,cur_region,new_region);
}



int nk_aspace_init()
{
    INIT_LIST_HEAD(&aspace_list);
    spinlock_init(&state_lock);

    return 0;
}

int nk_aspace_init_ap()
{
    INFO("inited\n");
    return 0;
}


int nk_aspace_dump_aspaces(int detail)
{
    struct list_head *cur;
    STATE_LOCK_CONF;
    STATE_LOCK();
    list_for_each(cur,&aspace_list) {
	struct nk_aspace *a = list_entry(cur,struct nk_aspace, aspace_list_node);
	BOILERPLATE(a,print,detail);
    }
    STATE_UNLOCK();
    return 0;
}


static int handle_ases (char * buf, void * priv)
{
    int detail = 0;
    if (strstr(buf,"d")) {
	detail = 1;
    }
    nk_aspace_dump_aspaces(detail);
    return 0;
}


static struct shell_cmd_impl ases_impl = {
    .cmd      = "ases",
    .help_str = "ases [detail]",
    .handler  = handle_ases,
};
nk_register_shell_cmd(ases_impl);

int nk_aspace_dump_aspace_impls()
{
    extern nk_aspace_impl_t *__start_aspace_impls;
    extern nk_aspace_impl_t * __stop_aspace_impls;
    nk_aspace_impl_t **cur;

    for (cur = &__start_aspace_impls;
	 cur && cur!=&__stop_aspace_impls && *cur;
	 cur++) {
	nk_vc_printf("%s\n",(*cur)->impl_name);
    }
    return 0;
}


static int handle_asis (char * buf, void * priv)
{
    nk_aspace_dump_aspace_impls();
    return 0;
}


static struct shell_cmd_impl asis_impl = {
    .cmd      = "asis",
    .help_str = "asis",
    .handler  = handle_asis,
};
nk_register_shell_cmd(asis_impl);
