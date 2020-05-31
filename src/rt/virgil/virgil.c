#include <nautilus/nautilus.h>
#include <nautilus/task.h>
#include <nautilus/shell.h>

#define _GNU_SOURCE
#include <sched.h>   // to get us the cpu-set macros

#include <rt/virgil/virgil.h>


#define INFO(fmt, args...) INFO_PRINT("virgil: " fmt, ##args)
#define ERROR(fmt, args...) ERROR_PRINT("virgil: " fmt, ##args)
#ifdef NAUT_CONFIG_VIRGIL_RT_DEBUG
#define DEBUG(fmt, args...) DEBUG_PRINT("virgil: " fmt, ##args)
#else
#define DEBUG(fmt, args...)
#endif
#define WARN(fmt, args...)  WARN_PRINT("virgil: " fmt, ##args)

int nk_virgil_get_num_cpus(void)
{
    return nk_get_num_cpus();
}

// submit a task to any cpu
nk_virgil_task_t nk_virgil_submit_task_to_any_cpu(nk_virgil_func_t func,
						  void *input)
{
    // any cpu, unknown size, not detached
    return nk_task_produce(-1,0,func,input,0);
}

// submit a task to any cpu and detach
nk_virgil_task_t nk_virgil_submit_task_to_any_cpu_and_detach(nk_virgil_func_t func,
							     void *input)
{
    // any cpu, unknown size, detached
    return nk_task_produce(-1,0,func,input,NK_TASK_DETACHED);
}

// submit a task to a specific cpu
nk_virgil_task_t nk_virgil_submit_task_to_specific_cpu(nk_virgil_func_t func,
						       void *input,
						       int cpu)
{
    // specific cpu, unknown size, not detached
    return nk_task_produce(cpu,0,func,input,0);
}


// submit a task to any one of a set of cpus
nk_virgil_task_t nk_virgil_submit_task_to_cpu_set(nk_virgil_func_t func,
						  void *input,
						  cpu_set_t *cpuset)
{
    // we will define this to mean pick one at random from the set
    int i;
    int num=0;
    
    for (i=0; i<CPU_SETSIZE;i++) {
	// we cannot use CPU_COUNT here since it is not a macro
	if (CPU_ISSET(i,cpuset)) {
	    num++;
	}
    }
    
    int target = rdtsc()%num;
    int cpus;
    int last;

    DEBUG("submit task to cpu set - %d cpus in the set, our target is %d\n",num,target);
    
    cpus=0;
    last=0;
    for (i=0; i<CPU_SETSIZE;i++) {
	if (CPU_ISSET(i,cpuset)) {
	    last = i;
	    cpus++;
	    if (cpus==target) {
		break;
	    }
	}
    }

    if (cpus==target) {
	DEBUG("target cpu is %d\n",i);
	return nk_virgil_submit_task_to_specific_cpu(func,input,i);
    } else {
	ERROR("target NOT found\n");
	return 0;
    }
}


// immediately returns
//    <0 => error
//    =0 => task is not yet done
//    >0 => task is done, *output contains its output pointer
//          [in this case, do not check on the task again]
int nk_virgil_check_for_task_completion(nk_virgil_task_t task, void **output)
{
    // no statistics requested    
    int rc = nk_task_try_wait((struct nk_task *)task, output, 0);

    if (rc<0) {
	return rc;
    } else {
	// swap sense
	return !rc;
    }
}


// waits for task completion or error
//    <0 => error
//     0 => should not happen
//    >0 => task is done, *output contains its output pointer
//          [in this case, do not check on the task again]
int nk_virgil_wait_for_task_completion(nk_virgil_task_t task, void **output)
{
    int rc = nk_task_wait((struct nk_task *)task,output,0);

    if (rc<0) {
	return rc;
    } else {
	// swap sense
	return !rc;
    }
}

// get the number of tasks that are queued on a given cpu
// negative return => error
int nk_virgil_waiting_tasks_cpu(int cpu)
{
    nk_task_cpu_snapshot_t snap;

    nk_task_cpu_snapshot(cpu,&snap);

    // we assume we are responsible for all the unsized tasks
    if (snap.unsized_dequeued <= snap.unsized_enqueued) {
	return (int)(snap.unsized_enqueued - snap.unsized_dequeued);
    } else {
	return 0;
    }
}


static void _waiting_info_sys(uint64_t *waiting, uint64_t *idle_cpus)
{
    nk_task_system_snapshot_t snap;

    nk_task_system_snapshot(&snap,idle_cpus);

    // we assume we are responsible for all the unsized tasks
    if (snap.unsized_dequeued <= snap.unsized_enqueued) {
	*waiting = (snap.unsized_enqueued - snap.unsized_dequeued);
    } else {
	*waiting = 0;
    }
}

// get the number of tasks that are queued throughout the system
// note that there is no way to make this scalable...
// negative return => error
int nk_virgil_waiting_tasks_on_system(int cpu)
{
    uint64_t waiting, idle;
    
    _waiting_info_sys(&waiting,&idle);

    return (int)waiting;
}


// get number of idle cpus (ones without tasks)
// note that there is no way to make this scalable...
// negative return => error
int nk_virgil_idle_cpus(void)
{
    uint64_t waiting, idle;
    
    _waiting_info_sys(&waiting,&idle);

    return (int)idle;
}






// initialize a lock to the unlocked state
void nk_virgil_spinlock_init(nk_virgil_spinlock_t *lock)
{
    spinlock_init(lock);
}
 
void nk_virgil_spinlock_deinit(nk_virgil_spinlock_t *lock)
{
    spinlock_deinit(lock);
}

// spin on the lock until you get it
void nk_virgil_spinlock_lock(nk_virgil_spinlock_t *lock)
{
    spin_lock(lock);
}
    
// attempt to get the lock, just once, returns zero on success
int nk_virgil_spinlock_try_lock(nk_virgil_spinlock_t *lock)
{
    return spin_try_lock(lock);
}
 

void nk_virgil_spinlock_unlock(nk_virgil_spinlock_t *lock)
{
    spin_unlock(lock);
}


static int _nk_virgil_entry_test(int argc, char *argv[])
{
    int i;
    DEBUG("hello, my args are:\n");
    for (i=0;i<argc;i++) {
	DEBUG("\"%s\"\n",argv[i]);
    }

    return 0;
}

#define MAXCMD 256
    
static int
handle_virgil(char * buf, void * priv)
{
    char b[MAXCMD];
    char *argv[MAXCMD];
    int argc=0;
    
    strncpy(b,buf,MAXCMD); b[MAXCMD-1]=0;

    char *c = b;
    enum {WS,NWS} state=WS;
    
    while (*c) {
	if (state==WS) {
	    if (isspace(*c)) {
		// nothing
	    } else {
		argv[argc++] = c;
		state = NWS;
	    }
	} else { //state==NWS
	    if (isspace(*c)) {
		*c = 0;
		state = WS;
	    } else {
		//nothing
	    }
	}
	c++;
    }
	    
    int rc = _nk_virgil_entry_test(argc,argv);

    nk_vc_printf("virgil app returned %d\n", rc);
    
    
    return 0;
}


static struct shell_cmd_impl virgil_impl = {
    .cmd      = "virgil",
    .help_str = "virgil command args...",
    .handler  = handle_virgil,
};
nk_register_shell_cmd(virgil_impl);
















