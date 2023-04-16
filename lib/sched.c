#include <env.h>
#include <pmap.h>
#include <printf.h>
#include <color.h>

/* Overview:
 *  Implement simple round-robin scheduling.
 *
 *
 * Hints:
 *  1. The variable which is for counting should be defined as 'static'.
 *  2. Use variable 'env_sched_list', which is a pointer array.
 *  3. CANNOT use `return` statement!
 */
/*** exercise 3.15 ***/
extern struct Env_list env_sched_list[];
extern struct Env *curenv;

int sched_dbg = 0;

char *toString(struct Env *p) {
    return p->env_status == ENV_FREE ? "FREE" :
           p->env_status == ENV_RUNNABLE ? "RUN" :
           p->env_status == ENV_NOT_RUNNABLE ? "BLOCK" : "INVALID";
}

struct Env* nextEnv(int *point, int dbg) {
    struct Env *ret;
    LIST_FOREACH(ret, &env_sched_list[*point], env_sched_link) {
        if (dbg) printf("[ENV_YIELD_WALK] Peeking Env[%d] -> (%s)...\n", ret->env_id, toString(ret));
        if (ret->env_status == ENV_RUNNABLE) {
            LIST_REMOVE(ret, env_sched_link);
            LIST_INSERT_TAIL(&env_sched_list[1 - *point], ret, env_sched_link);
            return ret;
        }
    }

    /* Current List has no Runnable Env, Change list */
    *point = 1 - *point;
    LIST_FOREACH(ret, &env_sched_list[*point], env_sched_link) {
        if (dbg) printf("[ENV_YIELD_WALK] Peeking Env[%d] -> (%s)...\n", ret->env_id, toString(ret));
        if (ret->env_status == ENV_RUNNABLE) {
            LIST_REMOVE(ret, env_sched_link);
            LIST_INSERT_TAIL(&env_sched_list[1 - *point], ret, env_sched_link);
            return ret;
        }
    }
    
    panic("\n[ERROR : ENV_YIELD] No Runnable Env!\n");
}

void sched_yield(void)
{
    static int count = 0; // remaining time slices of current env
    static int point = 0; // current env_sched_list index
    int dbg = sched_dbg;    
    /*  hint:
     *  1. if (count==0), insert `e` into `env_sched_list[1-point]`
     *     using LIST_REMOVE and LIST_INSERT_TAIL.
     *  2. if (env_sched_list[point] is empty), point = 1 - point;
     *     then search through `env_sched_list[point]` for a runnable env `e`, 
     *     and set count = e->env_pri
     *  3. count--
     *  4. env_run()
     *
     *  functions or macros below may be used (not all):
     *  LIST_INSERT_TAIL, LIST_REMOVE, LIST_FIRST, LIST_EMPTY
     */
    if (0) printf("[YIELD] Start!\n");
    assert(count >= 0);    
    if (count == 0 || curenv == NULL || curenv->env_status != ENV_RUNNABLE) {
        /* Current Env Should Pause */
        struct Env *next = nextEnv(&point, 0);
        count = next->env_pri - 1;
        if (dbg) printf(B_MAGENTA"Sched"C_RESET" : Running Env[%d], asid = %d\n", next->env_id, GET_ENV_ASID(next->env_id) >> 6);
        env_run(next);
        panic("\n[ERROR : ENV_YIELD] Running Another Env Stopped!\n");  
    } else {
        count--;
        env_run(curenv);
        if (dbg) printf("\n Continue Env...... \n");
        panic("\n[ERROR : ENV_YIELD] Continue Running CURENV Stopped!\n");
    } 
}
