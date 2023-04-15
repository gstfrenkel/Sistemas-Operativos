#include <inc/assert.h>
#include <inc/x86.h>
#include <kern/spinlock.h>
#include <kern/env.h>
#include <kern/pmap.h>
#include <kern/monitor.h>
#include <inc/syscall.h>
#include <kern/syscall.h>

const int STRIDENUMBER = 1000;

unsigned int pid[120000];
unsigned int env_runs[120000];

unsigned int pid_index = 0;
unsigned int times_called = 0;

void sched_halt(void);

struct Env *find_shortest_pass(struct Env *envs){
	struct Env *cur = NULL;
	for(int i = 0; i < NENV; i++){
		if((!cur || envs[i].pass < cur->pass) && envs[i].env_status == ENV_RUNNABLE)
			cur = &envs[i];
	}
	return cur;
}

// // Choose a user environment to run and run it.
void
sched_yield(void)
{
	// Implement simple round-robin scheduling.
	//
	// Search through 'envs' for an ENV_RUNNABLE environment in
	// circular fashion starting just after the env this CPU was
	// last running.  Switch to the first such environment found.
	//
	// If no envs are runnable, but the environment previously
	// running on this CPU is still ENV_RUNNING, it's okay to
	// choose that environment.
	//
	// Never choose an environment that's currently running on
	// another CPU (env_status == ENV_RUNNING). If there are
	// no runnable environments, simply drop through to the code
	// below to halt the cpu.
	
	#ifdef STRIDE
	
	struct Env *cur = find_shortest_pass(envs);

	if(!cur){
		if(curenv && curenv->env_status == ENV_RUNNING)
			cur = curenv;	
	}
	if (!cur)
		sched_halt();	
	
	cur->pass += cur->stride;
	pid[pid_index] = cur->env_id;
	env_runs[pid_index] = cur->env_runs;
	pid_index++;
	times_called++;


	if (times_called%50 == 0){
		for (unsigned int i = 0; i < NENV; i++) { 
			if (envs[ENVX(i)].env_status == ENV_RUNNABLE)
				envs[ENVX(i)].pass /= 2;
		}
	}
	env_run(cur);
	#endif

	#ifdef ROUND_ROBIN

	int base = 0;
	
	if (curenv)
		base = ENVX(curenv->env_id);

	for (unsigned int i = 0; i < NENV; i++) { 
		if (envs[ENVX(base + i)].env_status == ENV_RUNNABLE){
			times_called++;     // si lo descomentamos falla primes (Es para las estadisticas)
			pid[pid_index] = envs[ENVX(base + i)].env_id;
			pid_index++;
			env_run(&envs[ENVX(base + i)]);
		}
	}
	
	if (curenv && curenv->env_status == ENV_RUNNING){
		times_called++;
		pid[pid_index] = curenv->env_id;
		pid_index++;
		env_run(curenv);
	}
	sched_halt();	
	
	#endif
	// sched_halt never returns
	
}

// Halt this CPU when there is nothing to do. Wait until the
// timer interrupt wakes it up. This function never returns.
//
void
sched_halt(void)
{
	int i;
	
	// For debugging and testing purposes, if there are no runnable
	// environments in the system, then drop into the kernel monitor.
	for (i = 0; i < NENV; i++) {
		if ((envs[i].env_status == ENV_RUNNABLE ||
		     envs[i].env_status == ENV_RUNNING ||
		     envs[i].env_status == ENV_DYING))
			break;
	}
	if (i == NENV) {
		cprintf("No runnable environments in the system!\n");

		for (int i = 0; i<pid_index;i++){
			env_runs[ENVX(pid[i])]++;
			cprintf("Process of id: %d Ran: %d times.\n ", pid[i],env_runs[ENVX(pid[i])]); 
		}
		cprintf("Scheduler was called a total of: %d times.\n", times_called);
		while (1)
		monitor(NULL);
	}

	

	// Mark that no environment is running on this CPU
	curenv = NULL;
	lcr3(PADDR(kern_pgdir));

	// Mark that this CPU is in the HALT state, so that when
	// timer interupts come in, we know we should re-acquire the
	// big kernel lock
	xchg(&thiscpu->cpu_status, CPU_HALTED);

	// Release the big kernel lock as if we were "leaving" the kernel
	unlock_kernel();

	// Once the scheduler has finishied it's work, print statistics on performance.

	// Reset stack pointer, enable interrupts and then halt.
	asm volatile("movl $0, %%ebp\n"
	             "movl %0, %%esp\n"
	             "pushl $0\n"
	             "pushl $0\n"
	             "sti\n"
	             "1:\n"
	             "hlt\n"
	             "jmp 1b\n"
	             :
	             : "a"(thiscpu->cpu_ts.ts_esp0));
}
