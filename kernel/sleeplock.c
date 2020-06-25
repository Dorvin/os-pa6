// Sleeping locks

#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#include "sleeplock.h"
#include <stddef.h>

void
initsleeplock(struct sleeplock *lk, char *name)
{
  initlock(&lk->lk, "sleep lock");
  lk->name = name;
  lk->locked = 0;
  lk->pid = 0;
  for(int i=0; i<100; i++){
    lk->involved[i] = NULL;
    lk->donate_to[i] = -1;
  }
}

void
acquiresleep(struct sleeplock *lk)
{
  acquire(&lk->lk);
  for(int i=0; i<100; i++){
    if(lk->involved[i] == NULL){
      lk->involved[i] = myproc();
      lk->donate_to[i] = lk->pid;
      break;
    }
  }
  for(int i=0; i<100; i++){
    if(myproc()->lockon[i] == NULL){
      myproc()->lockon[i] = lk;
      break;
    }
  }
  while (lk->locked) {
    sleep(lk, &lk->lk);
  }
  lk->locked = 1;
  lk->pid = myproc()->pid;
  release(&lk->lk);
}

void
releasesleep(struct sleeplock *lk)
{
  acquire(&lk->lk);
  lk->locked = 0;
  lk->pid = 0;
  for(int i=0; i<100; i++){
    if(lk->involved[i] == myproc()){
      lk->involved[i] = NULL;
      lk->donate_to[i] = -1;
      break;
    }
  }
  for(int i=0; i<100; i++){
    if(myproc()->lockon[i] == lk){
      myproc()->lockon[i] = NULL;
      break;
    }
  }
  wakeup(lk);
  release(&lk->lk);
  // for preemptive scheduling
  yield();
}

int
holdingsleep(struct sleeplock *lk)
{
  int r;
  
  acquire(&lk->lk);
  r = lk->locked && (lk->pid == myproc()->pid);
  release(&lk->lk);
  return r;
}



