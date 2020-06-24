//--------------------------------------------------------------------
//
//  4190.307: Operating Systems (Spring 2020)
//
//  PA#6: Kernel Threads
//
//  June 2, 2020
//
//  Jin-Soo Kim
//  Systems Software and Architecture Laboratory
//  Department of Computer Science and Engineering
//  Seoul National University
//
//--------------------------------------------------------------------

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "proc.h"
#include "defs.h"

// kernel page table structure defined in vm.c
extern pagetable_t kernel_pagetable;
// proc structure defined in proc.c
extern struct proc proc[NPROC];

#ifdef SNU
int 
kthread_create(const char *name, int prio, void (*fn)(void *), void *arg)
{
  struct proc *p;

  for(p = proc; p < &proc[NPROC]; p++) {
    acquire(&p->lock);
    if(p->state == UNUSED) {
      goto found;
    } else {
      release(&p->lock);
    }
  }
  return -1;

found:
  p->pid = allocpid();

  // set name
  safestrcpy(p->name, name, sizeof(name));

  // set prio
  p->prio = prio;

  // Set to kernel page table.
  p->pagetable = kernel_pagetable;

  // Set up new context to start executing at kthread_wrapper
  memset(&p->context, 0, sizeof p->context);
  p->context.ra = (uint64)kthread_wrapper;
  p->context.sp = p->kstack + PGSIZE;

  // set fn & arg
  p->fn = fn;
  p->arg = arg;

  // make thread RUNNABLE
  p->state = RUNNABLE;

  // release lock
  release(&p->lock);

  // for preemptive scheduling
  yield();

  return p->pid;
}

void
kthread_wrapper(void)
{
  struct proc *p = myproc();
  // Because it sheduled from scheduler, holding lock
  release(&p->lock);
  void (*fn)(void *);
  void *arg;
  fn = p->fn;
  arg = p->arg;
  fn(arg);
}

void
kthread_exit(void)
{
  struct proc *p = myproc();
  acquire(&p->lock);
  p->state = UNUSED;
  // never return
  sched();
  // shoul not reach here
}

void
kthread_yield(void)
{
  yield();
}

// simply set base prio
void
kthread_set_prio(int newprio)
{
  struct proc *p = myproc();
  acquire(&p->lock);
  p->prio = newprio;
  release(&p->lock);
  // for preemptive scheduling
  yield();
}

// returns the effective priority value of the calling kernel thread in the range of [0, 99]
int
kthread_get_prio(void)
{
  // for now, just use base priority
  return myproc()->prio;
}
#endif
