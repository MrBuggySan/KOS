/******************************************************************************
    Copyright � 2012-2015 Martin Karsten

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/
#include "runtime/RuntimeImpl.h"
#include "runtime/Scheduler.h"
#include "runtime/Stack.h"
#include "runtime/Thread.h"
#include "kernel/Output.h"
#include "kernel/Output.h"


mword Scheduler::minGranularity = 0;
mword Scheduler::epochLength = 0;
mword Scheduler::defaultEpochLength = 0;

Scheduler::Scheduler() : readyCount(0), preemption(0), resumption(0), partner(this), currRealTimeCount(0) {

  Thread* idleThread = Thread::create((vaddr)idleStack, minimumStack);
  idleThread->setAffinity(this)->setPriority(idlePriority);
  // use low-level routines, since runtime context might not exist
  idleThread->stackPointer = stackInit(idleThread->stackPointer, &Runtime::getDefaultMemoryContext(), (ptr_t)Runtime::idleLoop, this, nullptr, nullptr);
  readyQueue[idlePriority].push_back(*idleThread);
  readyCount += 1;


  //Initialize the tree that contains the threads waiting to be served
  readyTree = new Tree<ThreadNode>();
}

static inline void unlock() {}

template<typename... Args>
static inline void unlock(BasicLock &l, Args&... a) {
  l.release();
  unlock(a...);
}

// very simple N-class prio scheduling!
template<typename... Args>
inline void Scheduler::switchThread(Scheduler* target, Args&... a) {
  //TODO:run the leftmost task on the readyTree
<<<<<<< HEAD
    Thread* nextThread;
    CHECK_LOCK_MIN(sizeof...(Args));
    preemption += 1;
    readyLock.acquire();
    //if ready tree not empty, pop leftmost node of tree to nextThread
    if(!readyTree->empty()){
      nextThread = readyTree->popMinNode()->th;
=======


  preemption += 1;
  CHECK_LOCK_MIN(sizeof...(Args));
  Thread* nextThread;
  readyLock.acquire();
  for (mword i = 0; i < (target ? idlePriority : maxPriority); i += 1) {
    if (!readyQueue[i].empty()) {
      nextThread = readyQueue[i].pop_front();
>>>>>>> 5fa997d579614b9d401f1790ee87b751a3612410
      readyCount -= 1;
      goto threadFound;
    }

  readyLock.release();
  GENASSERT0(target);
  GENASSERT0(!sizeof...(Args));
  return;                                         // return to current thread

threadFound:
  readyLock.release();
  resumption += 1;
  Thread* currThread = Runtime::getCurrThread();
  GENASSERTN(currThread && nextThread && nextThread != currThread, currThread, ' ', nextThread);

  if (target) currThread->nextScheduler = target; // yield/preempt to given processor
  else currThread->nextScheduler = this;          // suspend/resume to same processor
  unlock(a...);                                   // ...thus can unlock now
  CHECK_LOCK_COUNT(1);
  Runtime::debugS("Thread switch <", (target ? 'Y' : 'S'), ">: ", FmtHex(currThread), '(', FmtHex(currThread->stackPointer), ") to ", FmtHex(nextThread), '(', FmtHex(nextThread->stackPointer), ')');

  Runtime::MemoryContext& ctx = Runtime::getMemoryContext();
<<<<<<< HEAD
<<<<<<< HEAD
  if(currThread->getVRuntime() > nextThread->getVRuntime()){
    Runtime::setCurrThread(nextThread);
    Thread* prevThread = stackSwitch(currThread, target, &currThread->stackPointer, nextThread->stackPointer);
    // REMEMBER: Thread might have migrated from other processor, so 'this'
    //           might not be currThread's Scheduler object anymore.
    //           However, 'this' points to prevThread's Scheduler object.
    Runtime::postResume(false, *prevThread, ctx);
  }
  else{
      readyTree->insert(nextThread);
      readyCount+= 1;
  }
=======

  //now that we have set the scheduler for this thread, let the thread run!?
=======
>>>>>>> parent of 5fa997d... comment updates.
  Runtime::setCurrThread(nextThread);
  Thread* prevThread = stackSwitch(currThread, target, &currThread->stackPointer, nextThread->stackPointer);
  // REMEMBER: Thread might have migrated from other processor, so 'this'
  //           might not be currThread's Scheduler object anymore.
  //           However, 'this' points to prevThread's Scheduler object.
  Runtime::postResume(false, *prevThread, ctx);
>>>>>>> 5fa997d579614b9d401f1790ee87b751a3612410
  if (currThread->state == Thread::Cancelled) {
    currThread->state = Thread::Finishing;
    switchThread(nullptr);
    unreachable();
  }
}

extern "C" Thread* postSwitch(Thread* prevThread, Scheduler* target) {
  CHECK_LOCK_COUNT(1);
  if fastpath(target) Scheduler::resume(*prevThread);
  return prevThread;
}

extern "C" void invokeThread(Thread* prevThread, Runtime::MemoryContext* ctx, funcvoid3_t func, ptr_t arg1, ptr_t arg2, ptr_t arg3) {
  Runtime::postResume(true, *prevThread, *ctx);
  func(arg1, arg2, arg3);
  Runtime::getScheduler()->terminate();
}

void Scheduler::enqueue(Thread& t) {


  GENASSERT1(t.priority < maxPriority, t.priority);
  readyLock.acquire();
  //readyQueue[t.priority].push_back(t);  //TODO: switch this for readyTree implementation
  //TODO: add a new thread to the readyTree
  // readyTree->insert(*(new ThreadNode(*anyThreadClassObject)));

  //Set the vRumtime of the task to vRuntime of the task of the leftmost node of the tree.
  Thread* minThread = readyTree->readMinNode()->th;
  mword newVRunTime = minThread->getVRuntime();

  if(t.vRuntime){
    t.updateVRuntime(newVRunTime);
  }
  else{
    t.initVRuntime(newVRunTime);
  }

  readyTree->insert(*(new ThreadNode(&t)));

  //update the epochLength?
  bool wake = (readyCount == 0);
  readyCount += 1;

  if(defaultEpochLength>= readyCount * minGranularity){
    epochLength = defaultEpochLength;
  }
  else{
    epochLength = readyCount * minGranularity;
  }
  readyLock.release();
  Runtime::debugS("Thread ", FmtHex(&t), " queued on ", FmtHex(this));
  if (wake) Runtime::wakeUp(this);
}

void Scheduler::resume(Thread& t) {
  GENASSERT1(&t != Runtime::getCurrThread(), Runtime::getCurrThread());
  if (t.nextScheduler) t.nextScheduler->enqueue(t);
  else Runtime::getScheduler()->enqueue(t);
}



void Scheduler::preempt() {               // IRQs disabled, lock count inflated
<<<<<<< HEAD
    currRealTimeCount++;
=======

currRealTimeCount++;
if (currRealTimeCount == minGranularity){

    //Check if the readyTree is empty
    if(!readyTree.empty()){
      Thread * currThread = readyTree.readMinNode();
      mword threadPrio = currThread->getPriority();
      //update vRuntime of the running task
      //according to runtime/Runtime.h the highest priority is 0 and lowest priority is 3.
      //higher prio tasks will increment less often than lower prio tasks
      mword vRuntimeIncremenet = threadPrio + 1 ;
      currThread->updateVRuntime(vRuntimeIncremenet);
    }


>>>>>>> 5fa997d579614b9d401f1790ee87b751a3612410

    //TODO:update the tree now that we have a new vRuntime for the current task
      //pop the current task
      //insert it again to put it in correct place inside the readyTree


      //TODO: the switchThread will have to figure out which thread in the readyTree to run next.

<<<<<<< HEAD
<<<<<<< HEAD
      //TODO: review this set of code and see how it will affect our algorithm
      //from a shallow analysys none of TESTING_NEVER_MIGRATE & TESTING_ALWAYS_MIGRATE are declared.
    if(currRealTimeCount == minGranularity){
          //Check if the readyTree is empty
          Thread * currThread = Runtime::getCurrThread();
          mword threadPrio = currThread->getPriority();
          //update vRuntime of the running task
          //according to runtime/Runtime.h the highest priority is 0 and lowest priority is 3.
          //higher prio tasks will increment less often than lower prio tasks
          mword vRuntimeIncrement = threadPrio + 1 ;
          currThread->updateVRuntime(vRuntimeIncrement);
      }

=======
      //TODO: review this set of code and see how it will affect our algorithm
      //from a shallow analysys none of TESTING_NEVER_MIGRATE & TESTING_ALWAYS_MIGRATE are declared.
>>>>>>> parent of 5fa997d... comment updates.
      #if TESTING_NEVER_MIGRATE
        switchThread(this);
      #else /* migration enabled */
        Scheduler* target = Runtime::getCurrThread()->getAffinity(); //this will run - Andrei
      #if TESTING_ALWAYS_MIGRATE
        if (!target) target = partner;
      #else /* simple load balancing */
        if (!target) target = (partner->readyCount + 2 < readyCount) ? partner : this; //this will run - Andrei
      #endif
        switchThread(target); //this will run - Andrei
      #endif
<<<<<<< HEAD
      currRealTimeCount = 0;
=======


      // //TODO: review this set of code and see how it will affect our algorithm
      // //from a shallow analysys none of TESTING_NEVER_MIGRATE & TESTING_ALWAYS_MIGRATE are declared.
      // #if TESTING_NEVER_MIGRATE
      //   switchThread(this); //Stay on this scheduler only
      // #else /* migration enabled */
      //   //get the possible cores for this task to run on.
      //   Scheduler* target = Runtime::getCurrThread()->getAffinity(); //this will run - Andrei
      // #if TESTING_ALWAYS_MIGRATE
      //   if (!target) target = partner;
      // #else /* simple load balancing */
      // //TODO: do we have to modify this load balancing algorithm?
      // //TODO: do we have to pass tasks between schedulers?
      //   if (!target) target = (partner->readyCount + 2 < readyCount) ? partner : this; //this will run - Andrei
      // #endif
      //
      //   switchThread(target); //this will run - Andrei
      // #endif
>>>>>>> 5fa997d579614b9d401f1790ee87b751a3612410
=======
>>>>>>> parent of 5fa997d... comment updates.

    return;
}

void Scheduler::suspend(BasicLock& lk) {
  //TODO:How to deal with I/O blocking?

  Runtime::FakeLock fl;
  switchThread(nullptr, lk);
}

void Scheduler::suspend(BasicLock& lk1, BasicLock& lk2) {
  Runtime::FakeLock fl;
  switchThread(nullptr, lk1, lk2);
}

void Scheduler::terminate() {
  Runtime::RealLock rl;
  Thread* thr = Runtime::getCurrThread();
  GENASSERT1(thr->state != Thread::Blocked, thr->state);
  thr->state = Thread::Finishing;
  switchThread(nullptr);
  unreachable();
}
