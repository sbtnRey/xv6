#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"

#ifdef CS333_P2
#include "uproc.h"
#endif

#ifdef CS333_P3P4
// p4



struct StateLists {
	//p4
	struct proc* ready[MAX+1];
	//struct proc* ready
	struct proc* free;
	struct proc* sleep;
	struct proc* zombie;
	struct proc* running;
	struct proc* embryo;	
	uint promoteAtTime;  // ticks value for which promotion will occur
};
#endif

struct {
  struct spinlock lock;
  struct proc proc[NPROC];
	#ifdef CS333_P3P4
	struct StateLists pLists;	
	//p4
	#endif
} ptable;

static struct proc *initproc;
static char *states [];

int nextpid = 1;
extern void forkret(void);
extern void trapret(void);

static void wakeup1(void *chan);

void
pinit(void)
{
  initlock(&ptable.lock, "ptable");
}

#ifdef CS333_P3P4
static int						remove(struct proc **, struct proc *);
static void						addToFront(struct proc **, struct proc *);
static struct proc*		pop(struct proc **);
static void						add(struct proc **, struct proc *);					

static void						assertState(struct proc *, enum procstate);

//p4

static void 					promote();
static void						prioritize(struct proc *);
static void						updatePriority(struct proc **);
#endif


// Look in the process table for an UNUSED proc.
// If found, change state to EMBRYO and initialize
// state required to run in the kernel.
// Otherwise return 0.
static struct proc*
allocproc(void)
{
  struct proc *p;
  char *sp;


  acquire(&ptable.lock);

#ifdef CS333_P3P4
	// Popping the first process off the list
	if(ptable.pLists.free)	
		goto found;
#else
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == UNUSED)
      goto found;
#endif
  release(&ptable.lock);
  return 0;

found:
	#ifdef CS333_P3P4
	p = ptable.pLists.free;
	ptable.pLists.free = p -> next;
	assertState(p, UNUSED);
	#endif
  p->state = EMBRYO;
	#ifdef CS333_P3P4
	p -> next = ptable.pLists.embryo;
	ptable.pLists.embryo = p;	
	assertState(p, EMBRYO);
	#endif
  
	p->pid = nextpid++;
  release(&ptable.lock);

  // Allocate kernel stack.
  if((p->kstack = kalloc()) == 0){
		// Go backwards and put
		#ifdef CS333_P3P4
		acquire(&ptable.lock);
		if(remove(&ptable.pLists.embryo, p) == 0)
			panic("I broke in allocproc, embryo->unused");
	
		assertState(p,EMBRYO);
		p -> state = UNUSED;
		addToFront(&ptable.pLists.free, p); 
		assertState(p, UNUSED);
		release(&ptable.lock);
		#else
    p->state = UNUSED;
		#endif
    return 0;
  }
  sp = p->kstack + KSTACKSIZE;
  
  // Leave room for trap frame.
  sp -= sizeof *p->tf;
  p->tf = (struct trapframe*)sp;
  
  // Set up new context to start executing at forkret,
  // which returns to trapret.
  sp -= 4;
  *(uint*)sp = (uint)trapret;

  sp -= sizeof *p->context;
  p->context = (struct context*)sp;
  memset(p->context, 0, sizeof *p->context);
  p->context->eip = (uint)forkret;
 
  // Project 1
  // Initializing start_ticks to the global kernal value of ticks
  #ifdef CS333_P1 
  p -> start_ticks = ticks; 
  #endif

  #ifdef CS333_P2
  p -> cpu_ticks_total = 0;
  p -> cpu_ticks_in = 0;
  #endif

  return p;
}


// Set up first user process.
void
userinit(void)
{
  struct proc *p;
  extern char _binary_initcode_start[], _binary_initcode_size[];
 	ptable.pLists.promoteAtTime = TICKS_TO_PROMOTE;

	#ifdef CS333_P3P4
	//Initializing free list
	int i;
	ptable.pLists.free = &ptable.proc[0];

	for (i = 0; i < NPROC - 1; i++){
		ptable.proc[i].state = UNUSED;
		ptable.proc[i].next = &ptable.proc[i + 1];
		
	}
	ptable.proc[NPROC-1].next = 0;	
	ptable.proc[NPROC-1].state = UNUSED;	
	
	int j;
	for (j = 0; j < MAX + 1; j++){
		 ptable.pLists.ready[j] = 0;
	}
	
	ptable.pLists.sleep = 0;
	ptable.pLists.zombie = 0;
	ptable.pLists.running = 0;
	ptable.pLists.embryo = 0;
	
	#endif
			
 
  p = allocproc();
  initproc = p;
  if((p->pgdir = setupkvm()) == 0)
    panic("userinit: out of memory?");
  inituvm(p->pgdir, _binary_initcode_start, (int)_binary_initcode_size);
  p->sz = PGSIZE;
  memset(p->tf, 0, sizeof(*p->tf));
  p->tf->cs = (SEG_UCODE << 3) | DPL_USER;
  p->tf->ds = (SEG_UDATA << 3) | DPL_USER;
  p->tf->es = p->tf->ds;
  p->tf->ss = p->tf->ds;
  p->tf->eflags = FL_IF;
  p->tf->esp = PGSIZE;
  p->tf->eip = 0;  // beginning of initcode.S

  safestrcpy(p->name, "initcode", sizeof(p->name));
  p->cwd = namei("/");
	// stateChange embryo -> ready/runnable
	#ifdef CS333_P3P4
	//p4
	p -> priority = 0;
	p -> budget = BUDGET;

	acquire(&ptable.lock);
	if(remove(&ptable.pLists.embryo, p) == 0)
		panic("I broke in userinit embryo->runnable");
		
	assertState(p, EMBRYO);
	p->state = RUNNABLE;
	
	// p4
	add(&ptable.pLists.ready[p->priority], p);
	assertState(p, RUNNABLE);
	release(&ptable.lock);
	#else
  p->state = RUNNABLE;
	#endif

  #ifdef CS333_P2
  p->uid = DEFAULTUID;
  p->gid = DEFAULTGID;
  #endif
}

// Grow current process's memory by n bytes.
// Return 0 on success, -1 on failure.
int
growproc(int n)
{
  uint sz;
  
  sz = proc->sz;
  if(n > 0){
    if((sz = allocuvm(proc->pgdir, sz, sz + n)) == 0)
      return -1;
  } else if(n < 0){
    if((sz = deallocuvm(proc->pgdir, sz, sz + n)) == 0)
      return -1;
  }
  proc->sz = sz;
  switchuvm(proc);
  return 0;
}

// Create a new process copying p as the parent.
// Sets up stack to return as if from system call.
// Caller must set state of returned proc to RUNNABLE.
int
fork(void)
{
  int i, pid;
  struct proc *np;

  // Allocate process.
  if((np = allocproc()) == 0)
    return -1;

  // Copy process state from p.
  if((np->pgdir = copyuvm(proc->pgdir, proc->sz)) == 0){
    kfree(np->kstack);
    np->kstack = 0;
		// stateChange embryo to free
		#ifdef CS333_P3P4
		acquire(&ptable.lock);
		if(remove(&ptable.pLists.embryo, np) == 0)
			panic("I broke in fork, embryo->free");
		
		assertState(np, EMBRYO);
		np -> state = UNUSED;
		addToFront(&ptable.pLists.free, np);
		assertState(np, UNUSED);
		release(&ptable.lock);
		#else
    np->state = UNUSED;
		#endif
    return -1;
  }
  np->sz = proc->sz;
  np->parent = proc;
  *np->tf = *proc->tf;
  
  #ifdef CS333_P2  
  np->uid = proc->uid;
  np->gid = proc->gid;
  #endif

  // Clear %eax so that fork returns 0 in the child.
  np->tf->eax = 0;

  for(i = 0; i < NOFILE; i++)
    if(proc->ofile[i])
      np->ofile[i] = filedup(proc->ofile[i]);
  np->cwd = idup(proc->cwd);

  safestrcpy(np->name, proc->name, sizeof(proc->name));
 
  pid = np->pid;

	np -> budget = BUDGET;
	np -> priority = 0; 

  // lock to force the compiler to emit the np->state write last.
  acquire(&ptable.lock);
	// stateChange embryo to runnable
	#ifdef CS333_P3P4
	if(remove(&ptable.pLists.embryo, np) == 0)
		panic("I broke in fork, embryo->runnable");

	assertState(np, EMBRYO);
	np->state = RUNNABLE;
	add(&ptable.pLists.ready[np->priority], np);
	assertState(np, RUNNABLE);
	#else
  np->state = RUNNABLE;
	#endif
  release(&ptable.lock);
  
  return pid;
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait() to find out it exited.
#ifndef CS333_P3P4
void
exit(void)
{
  struct proc *p;
  int fd;

  if(proc == initproc)
    panic("init exiting");

  // Close all open files.
  for(fd = 0; fd < NOFILE; fd++){
    if(proc->ofile[fd]){
      fileclose(proc->ofile[fd]);
      proc->ofile[fd] = 0;
    }
  }

  begin_op();
  iput(proc->cwd);
  end_op();
  proc->cwd = 0;

  acquire(&ptable.lock);

  // Parent might be sleeping in wait().
  wakeup1(proc->parent);

  // Pass abandoned children to init.
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->parent == proc){
      p->parent = initproc;
			// stateChange
      if(p->state == ZOMBIE)
        wakeup1(initproc);
    }
  }

  // Jump into the scheduler, never to return.
	// stateChange
  proc->state = ZOMBIE;
  sched();
  panic("zombie exit");
}
#else
void
exit(void)
{
struct proc *p;
  int fd;

  if(proc == initproc)
    panic("init exiting");

  // Close all open files.
  for(fd = 0; fd < NOFILE; fd++){
    if(proc->ofile[fd]){
      fileclose(proc->ofile[fd]);
      proc->ofile[fd] = 0;
    }
  }

  begin_op();
  iput(proc->cwd);
  end_op();
  proc->cwd = 0;

  acquire(&ptable.lock);

  // Parent might be sleeping in wait().
  wakeup1(proc->parent);

  // Pass abandoned children to init.
	// May need to put brackets around if/s

	// check for ready
	//p4
	int i;
	for (i = 0; i <= MAX; i++){
 		p = ptable.pLists.ready[i];

		while(p){
			if(p -> parent == proc)
				p -> parent = initproc;
		
			p = p -> next;
		}
	}
	
	// check for sleep
	p = ptable.pLists.sleep;

	while(p){
		if(p -> parent == proc)
			p -> parent = initproc;
		
		p = p -> next;
	}
	
	// check for running
	p = ptable.pLists.running;

	while(p){
		if(p -> parent == proc)
			p -> parent = initproc;
		
		p = p -> next;
	}

	// check for embryo
	p = ptable.pLists.embryo;

	while(p){
		if(p -> parent == proc)
			p -> parent = initproc;
		
		p = p -> next;
	}


	// check for zombie

	p = ptable.pLists.zombie;

	while(p){
		if(p -> parent == proc){
			p -> parent = initproc;
			wakeup1(initproc);	
		}
		p = p -> next;
	}

		

  // Jump into the scheduler, never to return.
	// stateChange running -> zombie
	if(remove(&ptable.pLists.running, proc) == 0)
		panic("Remove from running to zombie");
	assertState(proc, RUNNING);
  proc->state = ZOMBIE;
	addToFront(&ptable.pLists.zombie, proc);
	assertState(proc, ZOMBIE);
	
  sched();
  panic("zombie exit");
}
#endif

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
#ifndef CS333_P3P4
int
wait(void)
{
  struct proc *p;
  int havekids, pid;

  acquire(&ptable.lock);
  for(;;){
    // Scan through table looking for zombie children.
    havekids = 0;
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->parent != proc)
        continue;
      havekids = 1;
      if(p->state == ZOMBIE){
        // Found one.
        pid = p->pid;
        kfree(p->kstack);
        p->kstack = 0;
        freevm(p->pgdir);
				// stateChange
        p->state = UNUSED;
        p->pid = 0;
        p->parent = 0;
        p->name[0] = 0;
        p->killed = 0;
        release(&ptable.lock);
        return pid;
      }
    }

    // No point waiting if we don't have any children.
    if(!havekids || proc->killed){
      release(&ptable.lock);
      return -1;
    }

    // Wait for children to exit.  (See wakeup1 call in proc_exit.)
    sleep(proc, &ptable.lock);  //DOC: wait-sleep
  }
}
#else
int
wait(void)
{
 struct proc *p;
  int havekids, pid;

  acquire(&ptable.lock);
  for(;;){
    // Scan through table looking for zombie children.
    havekids = 0;
   	p = ptable.pLists.zombie;

		while (p){
			if(p -> parent == proc){
      	havekids = 1;

				//remove from zombie
				if(remove(&ptable.pLists.zombie, p) == 0)
					panic("Broke in wait, removing from zombie");
				
				assertState(p, ZOMBIE);
				
        pid = p->pid;
        kfree(p->kstack);
        p->kstack = 0;
        freevm(p->pgdir);

        p->state = UNUSED;
				addToFront(&ptable.pLists.free, p);
				assertState(p, UNUSED);
				
        p->pid = 0;
        p->parent = 0;
        p->name[0] = 0;
        p->killed = 0;
        release(&ptable.lock);
        return pid;
      }
			p = p-> next;
    }

		// Check each lists if lists currently have children
		if (!havekids){
			int i;
			
			for (i = 0; i <= MAX; i++){
				p = ptable.pLists.ready[i];
			
				while (p){
					if(p -> parent == proc)
						havekids = 1;
					p = p -> next;
				}
			}
		}

		if (!havekids){
			p = ptable.pLists.running;
			
			while (p){
				if(p -> parent == proc)
					havekids = 1;
				p = p -> next;
			}
		}

		if (!havekids){
			p = ptable.pLists.sleep;
			
			while (p){
				if(p -> parent == proc)
					havekids = 1;
				p = p -> next;
			}
		}

	if (!havekids){
			p = ptable.pLists.embryo;
			
			while (p){
				if(p -> parent == proc)
					havekids = 1;
				p = p -> next;
			}
		}

    // No point waiting if we don't have any children.
    if(!havekids || proc->killed){
      release(&ptable.lock);
      return -1;
    }

    // Wait for children to exit.  (See wakeup1 call in proc_exit.)
    sleep(proc, &ptable.lock);  //DOC: wait-sleep
  }
}
#endif

// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run
//  - swtch to start running that process
//  - eventually that process transfers control
//      via swtch back to the scheduler.
#ifndef CS333_P3P4
// original xv6 scheduler. Use if CS333_P3P4 NOT defined.
void
scheduler(void)
{
  struct proc *p;
  int idle;  // for checking if processor is idle
  
  
  for(;;){
    // Enable interrupts on this processor.
    sti();

    idle = 1;  // assume idle unless we schedule a process
    // Loop over process table looking for process to run.
    acquire(&ptable.lock);
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->state != RUNNABLE)
        continue;

      // Switch to chosen process.  It is the process's job
      // to release ptable.lock and then reacquire it
      // before jumping back to us.
      idle = 0;  // not idle this timeslice
      proc = p;
      switchuvm(p);

      #ifdef CS333_P2
      p->cpu_ticks_in = ticks;
      #endif  
			// stateChange
      p->state = RUNNING;
      swtch(&cpu->scheduler, proc->context);
      switchkvm();

      // Process is done running for now.
      // It should have changed its p->state before coming back.
      proc = 0;
    }
    release(&ptable.lock);
    // if idle, wait for next interrupt
    if (idle) {
      sti();
      hlt();
    }
  }
}

#else
void
scheduler(void)
{
	struct proc *p;
  int idle;  // for checking if processor is idle
	
	
	
  
  for(;;){
	
		if(ticks >= ptable.pLists.promoteAtTime){
			promote();	
			ptable.pLists.promoteAtTime = ticks + TICKS_TO_PROMOTE;
		}
    // Enable interrupts on this processor.
    sti();

    idle = 1;  // assume idle unless we schedule a process
    // Loop over process table looking for process to run.
    acquire(&ptable.lock);
		//p4
		p = 0;
		int i = 0;
		while (!p && i <= MAX){
			if((p = pop(&ptable.pLists.ready[i]))){	
				assertState(p, RUNNABLE);
			


	
      	// Switch to chosen process.  It is the process's job
      	// to release ptable.lock and then reacquire it
      	// before jumping back to us.
      	idle = 0;  // not idle this timeslice
      	proc = p;
      	switchuvm(p);

				// Attempt at inserting proccess from ready -> running
      	p->state = RUNNING;
				addToFront(&ptable.pLists.running, p);
				
				assertState(p, RUNNING);
				
      	p->cpu_ticks_in = ticks;
      	swtch(&cpu->scheduler, proc->context);
      	switchkvm();

      	// Process is done running for now.
      	// It should have changed its p->state before coming back.
      	proc = 0;
				break;
    
			}
			else{
				i++;
			}
		}
		
		
    release(&ptable.lock);
    // if idle, wait for next interrupt
    if (idle) {
      sti();
      hlt();
    }
  }
}
#endif

// Enter scheduler.  Must hold only ptable.lock
// and have changed proc->state.
#ifndef CS333_P3P4
void
sched(void)
{
  int intena;

  if(!holding(&ptable.lock))
    panic("sched ptable.lock");
  if(cpu->ncli != 1)
    panic("sched locks");
  if(proc->state == RUNNING)
    panic("sched running");
  if(readeflags()&FL_IF)
    panic("sched interruptible");
  intena = cpu->intena;

  #ifdef CS333_P2
  proc->cpu_ticks_total += ticks - proc->cpu_ticks_in;
  #endif

  swtch(&proc->context, cpu->scheduler);
  cpu->intena = intena;
}
#else
void
sched(void)
{
   int intena;

  if(!holding(&ptable.lock))
    panic("sched ptable.lock");
  if(cpu->ncli != 1)
    panic("sched locks");
  if(proc->state == RUNNING)
    panic("sched running");
  if(readeflags()&FL_IF)
    panic("sched interruptible");
  intena = cpu->intena;


  #ifdef CS333_P2
  proc->cpu_ticks_total += ticks - proc->cpu_ticks_in;
  #endif

  swtch(&proc->context, cpu->scheduler);
  cpu->intena = intena;
 
}
#endif

// Give up the CPU for one scheduling round.
void
yield(void)
{
	
  acquire(&ptable.lock);  //DOC: yieldlock
	// stateChange
	#ifdef CS333_P3P4

	if(remove(&ptable.pLists.running, proc) == 0)
		panic("I broke down in yield");
	assertState(proc, RUNNING);
	proc -> state = RUNNABLE;
	
	prioritize(proc);
	add(&ptable.pLists.ready[proc ->priority], proc);
	assertState(proc, RUNNABLE);
	#else
  proc->state = RUNNABLE;
	#endif
  sched();
  release(&ptable.lock);
}

// A fork child's very first scheduling by scheduler()
// will swtch here.  "Return" to user space.
void
forkret(void)
{
  static int first = 1;
  // Still holding ptable.lock from scheduler.
  release(&ptable.lock);

  if (first) {
    // Some initialization functions must be run in the context
    // of a regular process (e.g., they call sleep), and thus cannot 
    // be run from main().
    first = 0;
    iinit(ROOTDEV);
    initlog(ROOTDEV);
  }
  
  // Return to "caller", actually trapret (see allocproc).
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
// 2016/12/28: ticklock removed from xv6. sleep() changed to
// accept a NULL lock to accommodate.
void
sleep(void *chan, struct spinlock *lk)
{
  if(proc == 0)
    panic("sleep");

  // Must acquire ptable.lock in order to
  // change p->state and then call sched.
  // Once we hold ptable.lock, we can be
  // guaranteed that we won't miss any wakeup
  // (wakeup runs with ptable.lock locked),
  // so it's okay to release lk.
  if(lk != &ptable.lock){
    acquire(&ptable.lock);
    if (lk) release(lk);
  }

  // Go to sleep.
  proc->chan = chan;
	// stateChange
	#ifdef CS333_P3P4
	if(remove(&ptable.pLists.running, proc) == 0)
		panic("I broke in sleep, running to sleeping");
	
	assertState(proc, RUNNING);
	proc -> state = SLEEPING;
	
	prioritize(proc);
	addToFront(&ptable.pLists.sleep, proc);
	assertState(proc, SLEEPING);
	#else
  proc->state = SLEEPING;
	#endif
  sched();

  // Tidy up.
  proc->chan = 0;

  // Reacquire original lock.
  if(lk != &ptable.lock){ 
    release(&ptable.lock);
    if (lk) acquire(lk);
  }
}

#ifndef CS333_P3P4
// Wake up all processes sleeping on chan.
// The ptable lock must be held.
static void
wakeup1(void *chan)
{
  struct proc *p;

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == SLEEPING && p->chan == chan)
			// stateChange
      p->state = RUNNABLE;
}
#else
static void
wakeup1(void *chan)
{

		//Check if the sleep list is empty
		if(!ptable.pLists.sleep)
			return;
	
		struct proc *p = ptable.pLists.sleep;
		
		while(p){
			
			struct proc* current = p -> next;
			
			if(p -> chan == chan)
			{
				if(remove(&ptable.pLists.sleep, p) == 0)
					panic("Broke at removing from the sleep list in wakeup1");
				
				assertState(p, SLEEPING);
				p -> state = RUNNABLE;
				add(&ptable.pLists.ready[p ->priority], p);
				assertState(p, RUNNABLE);
			}
			p = current;
	}
}
#endif

// Wake up all processes sleeping on chan.
void
wakeup(void *chan)
{
  acquire(&ptable.lock);
  wakeup1(chan);
  release(&ptable.lock);
}

// Kill the process with the given pid.
// Process won't exit until it returns
// to user space (see trap in trap.c).
#ifndef CS333_P3P4
int
kill(int pid)
{
  struct proc *p;

  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->pid == pid){
      p->killed = 1;
      // Wake process from sleep if necessary.
      if(p->state == SLEEPING)
				// stateChange
        p->state = RUNNABLE;
      release(&ptable.lock);
      return 0;
    }
  }
  release(&ptable.lock);
  return -1;
}
#else
int
kill(int pid)
{
  struct proc *p;

  acquire(&ptable.lock);
	p = ptable.pLists.sleep;
	
	while (p){
		if(p->pid == pid){
      p->killed = 1;
			// remove from sleep and add to ready
			if(remove(&ptable.pLists.sleep, p) == 0)
				panic("Broke in kill sleep -> runnable");
			assertState(p, SLEEPING);
			p -> state = RUNNABLE;
			//p4
			add(&ptable.pLists.ready[p->priority], p);
			assertState(p, RUNNABLE);
			
      release(&ptable.lock);
      return 0;
    }
		p = p -> next;
  }
	//p4
	//p = ptable.pLists.ready[p->priority];
	// need to search through the array of linked lists
	int i;
	for (i = 0; i <= MAX; i++){
		p = ptable.pLists.ready[i];
		while(p){
			if(p -> pid == pid){
				p -> killed = 1;
				release(&ptable.lock);
				return 0;
			}
			p = p -> next;
		}
	}
	p = ptable.pLists.zombie;
	while(p){
		if(p -> pid == pid){
			p -> killed = 1;
			release(&ptable.lock);
			return 0;
		}
		p = p -> next;
	}

	p = ptable.pLists.running;
	while(p){
		if(p -> pid == pid){
			p -> killed = 1;
			release(&ptable.lock);
			return 0;
		}
		p = p -> next;
	}

	p = ptable.pLists.embryo;
	while(p){
		if(p -> pid == pid){
			p -> killed = 1;
			release(&ptable.lock);
			return 0;
		}
		p = p -> next;
	}

  release(&ptable.lock);
  return -1;
}
#endif

static char *states[] = {
  [UNUSED]    "unused",
  [EMBRYO]    "embryo",
  [SLEEPING]  "sleep ",
  [RUNNABLE]  "runnable",
  [RUNNING]   "run   ",
  [ZOMBIE]    "zombie"
};

// Free dump, essentially just print out eh number of processes in the free list
#ifdef CS333_P3P4
void
freedump(void)
{
	struct proc * current;
	int count;
	current = ptable.pLists.free;
	count= 0;

	while (current){
		count++;
		current = current -> next;
	}
	cprintf( "Free List Size: %d processes\n", count);

}

void
sleepdump(void)
{
	struct proc * current;
	current = ptable.pLists.sleep;

	cprintf("Sleep List Processes:\n");
	
	while (current){
		cprintf("%d -> ", current -> pid);
		current = current -> next;
	}
	
}

void
readydump(void)
{
	struct proc * current;
	int i;
	

	
	for (i = 0; i <= MAX; i++){
		current = ptable.pLists.ready[i];
		if(ptable.pLists.ready[i] == 0)
			cprintf("%d: Empty List", i);
			
		
		while (current){
			
			cprintf("%d:(%d, %d) -> ", i, current -> pid, current -> budget);
			current = current -> next;
		}		
		cprintf("\n");
	}
	cprintf("\n");

}

void
zombiedump(void)
{
	struct proc * current;
	int ppid;
	current = ptable.pLists.zombie;
	cprintf("Zombie List Processes:\n");

	while (current){
		if (current->pid == 1){
				ppid = 1;
			} else {
				ppid = current->parent->pid;		
			}
		cprintf("(%d, %d) -> ", current -> pid, ppid);
		current = current -> next;
	}
}


#endif

// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.
void
procdump(void)
{
  int i;
  struct proc *p;
  uint pc[10];
  char * state;
  
	#ifdef CS333_P1 
	#ifdef CS333_P2
  cprintf("PID\tName\tUID\tGID\tPPID\tPrio\tElapsed\tCPU\tState\tSize\tPCs\n");
	#endif
  #endif


  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state == UNUSED)
      continue;
    if(p->state >= 0 && p->state < NELEM(states) && states[p->state]){
      state = states[p->state];		
		}
    else{
      state = "???";
		}
    #ifdef CS333_P1
		#ifdef CS333_P2
		//P2 being utilized in the statements below
    int seconds;    
    int miliseconds;
    int current_ticks;
		int cpu;
		int ppid;

    current_ticks = ticks;
    seconds = ((current_ticks - p->start_ticks)/1000);
    miliseconds = (current_ticks - p->start_ticks) % 1000;
		cpu = current_ticks % 100;
		
		if (p->pid == 1){
				ppid = 1;
			} else {
				ppid = p->parent->pid;		
			}
   // p4 
    if(miliseconds < 10){
      cprintf("%d\t%s\t%d\t%d\t%d\t%d\t%d.00%d\t0.%d\t%s\t%d  ", p->pid, p-> name, p->uid,p->gid, ppid, p->priority, seconds, miliseconds, cpu, state, p-> sz);
			
    }
    else if(miliseconds < 100){
      cprintf("%d\t%s\t%d\t%d\t%d\t%d\t%d.0%d\t0.%d\t%s\t%d  ", p->pid, p-> name, p->uid,p->gid, ppid, p->priority, seconds, miliseconds, cpu, state, p-> sz);
    }
    else{
      cprintf("%d\t%s\t%d\t%d\t%d\t%d\t%d.%d\t0.%d\t%s\t%d  ", p->pid, p-> name, p->uid,p->gid, ppid, p->priority, seconds, miliseconds, cpu, state, p-> sz);
   }
	
    #else
    cprintf("%d %s %s", p->pid, state, p->name);  
		#endif
    #endif

    if(p->state == SLEEPING){
      getcallerpcs((uint*)p->context->ebp+2, pc);
      for(i=0; i<10 && pc[i] != 0; i++)
        cprintf(" %p", pc[i]);
    }
    cprintf("\n");
  }
}

#ifdef CS333_P2
int
proclist(uint max, struct uproc * table)
{
	int i, j;

	for ( i = 0, j = 0; i < max; ++i){
		if (ptable.proc[i].state != UNUSED && ptable.proc[i].state != EMBRYO){

			table[j].pid = ptable.proc[i].pid;
			table[j].uid = ptable.proc[i].uid;
			table[j].gid = ptable.proc[i].pid;
			if (ptable.proc[i].pid == 1){
				table[j].ppid = 1;
			} else {
				table[j].ppid = ptable.proc[i].parent->pid;		
			}
			table[j].elapsed_ticks = ticks - ptable.proc[i].start_ticks;
			table[j].CPU_total_ticks = ptable.proc[i].cpu_ticks_total;
			
					safestrcpy(table[j].state, states[ptable.proc[i].state], 9);
						
			table[j].size = ptable.proc[i].sz;
			#ifdef CS333_P3P4
			table[j].priority = ptable.proc[i].priority;
			#endif
			safestrcpy(table[j].name, ptable.proc[i].name, 16);

			++j;
		}
	}
	return j;
}
#endif

#ifdef CS333_P3P4
static void
assertState(struct proc * p, enum procstate state)
{
	if (p -> state != state){
		cprintf("%s != %s", p-> state, state);
		panic("States not equal");
	}
		
}


// pop
static struct proc *
pop(struct proc ** oldList)
{
	struct proc *temp;
	
	if (!*oldList)
		return 0;

	temp = *oldList;
	(*oldList) = (*oldList) -> next;
	
	return temp;
}

static int
remove(struct proc ** oldList, struct proc * p)
{
	struct proc *current, *prev;

	if(*oldList == 0){
		return 0;
	}
	prev = *oldList;
	current = (*oldList) -> next;
	

	//Evaluate if first process and/or only one element
	if (prev == p){
		(*oldList) = (*oldList) -> next;
		return 1;
	}

	while(current){
		// one item in the list
		if(current == p){
			prev -> next = current -> next;
			return 1;
		}		
		
		prev = current;
		current = current -> next;
	}

	return 0;
}

static void
addToFront(struct proc ** newList, struct proc * p)
{
	if(!*newList){
		*newList = p;	
		(*newList) -> next = 0;
	}
	else{
		p -> next = *newList;
		*newList = p;	
	}
}

static void
add(struct proc ** newList, struct proc * p)
{


	struct proc *current;
	current = *newList;
	if(*newList == 0){
		*newList = p;
		(*newList) -> next = 0;
	}
	else{
		while(current -> next){
			current = current -> next;
		}
	
		current -> next = p;
		current = p;
		current -> next = 0;
	}
}

static void
prioritize(struct proc * p)
{

	uint budget;
	budget = p->budget - (ticks - p->cpu_ticks_in);
	
	if (budget > BUDGET)
		budget = 0;
	if (!budget && !(p-> priority == MAX))
		p -> priority++;
	if (!budget)
		budget = BUDGET;

	p -> budget = budget;
}


static void 
promote()
{
	int n;
	n = 0;
	
	struct proc *current;
	current = ptable.pLists.ready[0];
	if (MAX){
		while (n < MAX){
			if (n || ptable.pLists.ready[0] == 0){
				ptable.pLists.ready[n] = ptable.pLists.ready[n+1];
				updatePriority(&ptable.pLists.ready[n]);
			}
			else {
				// loop 
				while(current -> next){
					current = current -> next;
				}
			
				current -> next = ptable.pLists.ready[1];
			}
			n++;
		}
		ptable.pLists.ready[MAX] = 0;
	}

	updatePriority(&ptable.pLists.sleep);
	updatePriority(&ptable.pLists.running);
}

static void
updatePriority(struct proc ** list)
{
	struct proc * current;
	current = *list;
	
		while(current){
			if(current->priority)
				current->priority--;
			current = current -> next;
		}

}

int
searchPid(int pid, int priority)
{
	
  acquire(&ptable.lock);
	struct proc * current;


	int i;
	for (i = 0; i <= MAX; i++){
		current = ptable.pLists.ready[i];
		while(current){
			if(current -> pid == pid){
				
				remove(&ptable.pLists.ready[i], current);
				if (current -> priority != i)
					panic("not in same list");
				
				current -> priority = priority;
				add(&ptable.pLists.ready[priority], current);
				if (current -> priority != priority)
					panic("not the same priority");
				
				
				release(&ptable.lock);
				return 1;
			}
			current = current -> next;
		}
	}
	
	current = ptable.pLists.embryo;
	while(current){
		if(current -> pid == pid){
			current -> priority = priority;

			
			release(&ptable.lock);
			return 1;
		}
		current = current -> next;
	}	

	current = ptable.pLists.running;
	while(current){
		if(current -> pid == pid){
			current -> priority = priority;
			
			
			release(&ptable.lock);
			return 1;
		}
		current = current -> next;
	}

	current = ptable.pLists.sleep;
	while(current){
		if(current -> pid == pid){
			current -> priority = priority;

			
			release(&ptable.lock);
			return 1;
		}
		current = current -> next;
	}

	
	release(&ptable.lock);
	return 0;
}
		

	
#endif

