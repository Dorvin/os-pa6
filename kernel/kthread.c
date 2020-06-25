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
#include <stddef.h>

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

  for(int i=0; i<100; i++){
    p->lockon[i] = NULL;
  }

  p->is_thread = 1;

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
  // return myproc()->prio;

  // return effective priority! with depth 2
  return kthread_get_eff(myproc(), 2);
}

int
kthread_get_eff(struct proc *p, int depth){
  // for user
  if(!p->is_thread){
    return p->prio;
  }
  int lk_count = 0;
  int high_prio = p->prio;
  int temp;
  // base case
  for(int i=0; i<100; i++){
    if(p->lockon[i] != NULL){
      for(int j=0; j<100; j++){
        if(p->lockon[i]->involved[j] != NULL && p->lockon[i]->involved[j] != p){
          lk_count++;
        }
      }
    }
  }
  if(lk_count == 0 || depth == 0){
    return p->prio;
  }

  depth--;
  // recursive case
  for(int i=0; i<100; i++){
    if(p->lockon[i] != NULL){
      for(int j=0; j<100; j++){
        if(p->lockon[i]->involved[j] != NULL && p->lockon[i]->involved[j] != p){
          if((temp = kthread_get_eff(p->lockon[i]->involved[j], depth)) < high_prio && p->lockon[i]->donate_to[j] == p->pid){
            //printf("%s donate to %s with prio %d... when high_prio is %d\n", p->lockon[i]->involved[j]->name, p->name, temp, high_prio);
            high_prio = temp;
          }
        }
      }
    }
  }
  return high_prio;
}
#endif
