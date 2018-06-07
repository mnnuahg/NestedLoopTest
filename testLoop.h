#ifndef __TEST_LOOP_H__
#define __TEST_LOOP_H__

#include <functional>
#include <iostream>
#include <queue>
#include <stdlib.h>
#include <inttypes.h>
#include <assert.h>
#include <time.h>

using namespace std;

struct Iter {
    function<void(int)> loopBody;
    int iterIdx;
    
    Iter(function<void(int)> f, int i) {
        loopBody = f;
        iterIdx = i;
    }
};

class TaskQueue {
private:
    queue<Iter> q;
    uint32_t    cap;
    uint64_t    numDeq;
public:
    TaskQueue(int c) {
        cap = c;
        numDeq = 0;
    }
    void enqueue(Iter t) {
        assert(q.size() < cap);
        q.push(t);
    }
    Iter dequeue() {
        assert(q.size() > 0);

        numDeq++;
        Iter it = q.front();
        q.pop();
        return it;
    }
    uint32_t size() {
        return q.size();
    }
    uint32_t capacity() {
        return cap;
    }
    uint64_t numDequeues() {
        return numDeq;
    }
};

TaskQueue TQ(1000000);

/* When we dequeue and execute a task, 
   the task may dequeue and execute another task. 
   Since execute the tasks is essentially making function calls,
   a series of dequeue will cause deep function calls which may overflow the stack.
   so we have _taskDepth to track the current call depth and _maxTaskDepth 
   to prevent the call depth to exceed certain threshold.
   Actually we don't need the two variables if your call stack is large enough. */
int _taskDepth = 0;
const int _maxTaskDepth = 10000;

enum Action {Enqueue, Dequeue, Execute};

class ExecPolicy {
public:
    virtual bool    hasNext() = 0;
    virtual Action  nextAction() = 0;
    virtual int     nextIterIdx() = 0;
    virtual void    proceed() = 0;
};

/* This policy consumes fewest task queue and stack space */
class DFExecPolicy : public ExecPolicy {
private:
    int     iterStart;
    int     iterEnd;
public:
    DFExecPolicy(int start, int end) {
        iterStart = start;
        iterEnd = end;
    }
    bool hasNext() {
        return iterStart<iterEnd;
    }
    Action nextAction() {
        /* When we encounters an iteration, we just execute them.
           This is naturally the sequential execution order, which is depth-first */
        return Execute;
    }
    int nextIterIdx() {
        return iterStart;
    }
    void proceed() {
        iterStart++;
    }
};

/* This policy consumes most task queue and stack space */
class BFExecPolicy : public ExecPolicy {
private:
    int         iterStart;
    int         iterEnd;
    uint64_t    goalNumDequeues;
public:
    BFExecPolicy(int start, int end) {
        iterStart = start;
        iterEnd = end;
        goalNumDequeues = 0;
    }
    bool hasNext() {
        /* We can leave the loop only after all iterations are executed (not just enqueued!).
           iterStart<iterEnd means some iterations (of this loop) are neither enqueued or executed.
           TQ.numDequeues()<goalNumDequeues means some iterations (of this loop) are still in the queue. */
        return iterStart<iterEnd || TQ.numDequeues()<goalNumDequeues;
    }
    Action nextAction() {
        /* BFPolicy is just enqueue all the iterations first, 
           then dequeue them until all the enqueued iterations are dequeued and executed.
           Execute the dequeued iterations may cause more iterations (inside further nested loops) are enqueued,
           however, these iterations won't be executed until all previously enqueued iterations are dequeued since the queue is FIFO. 
           This achieves breadth-first execution order. */
        if(iterStart < iterEnd) {
            if(_taskDepth<_maxTaskDepth && TQ.size()<TQ.capacity()) {
                /* If a task (iteration) is enqueued, then we should make sure that it is dequeued
                   before leaving the loop. 
                   Since the queue is FIFO, we can not dequeue the iteration enqueued here
                   until we have dequeued all iterations that are already in the queue.
                   Therefore, we set goalNumDequeues as follows so that the loop will be leaved
                   only after at least TQ.size()+1 more dequeues are made. */
                goalNumDequeues = TQ.numDequeues()+TQ.size()+1;
                return Enqueue;
            }
            else {
                return Execute;
            }
        }
        else {
            return Dequeue;
        }
    }
    int nextIterIdx() {
        return iterStart;
    }
    void proceed() {
        if(iterStart < iterEnd) {
            iterStart++;
        }
    }
};

class RandomExecPolicy : public ExecPolicy {
private:
    int         *remainingIndices;
    int         numRemainingIndices;
    uint64_t    goalNumDequeues;
    bool        shouldRemoveOneIter;

    void initializeRemainingIndices (int start, int end) {
        int size = end-start;
        remainingIndices = new int[size];
        for(int i=0; i<size; i++) {
            remainingIndices[i] = start+i;
        }
        for(int i=0; i<size; i++) {
            int idx1 = rand()%size;
            int idx2 = rand()%size;
            int tmp = remainingIndices[idx1];
            remainingIndices[idx1] = remainingIndices[idx2];
            remainingIndices[idx2] = tmp;
        }
        numRemainingIndices = size;
    }
public:
    RandomExecPolicy(int start, int end) {
        initializeRemainingIndices(start, end);
        goalNumDequeues = 0;
    }
    RandomExecPolicy(int start, int end, int randomSeed) {
        static bool hasSeedSet = false;
        if(!hasSeedSet) {
            srand(randomSeed);
            hasSeedSet = true;
        }
        initializeRemainingIndices(start, end);
        goalNumDequeues = 0;
    }
    ~RandomExecPolicy() {
        delete []remainingIndices;
    }
    bool hasNext() {
        return numRemainingIndices>0 || TQ.numDequeues()<goalNumDequeues;
    }
    Action nextAction() {
        if(numRemainingIndices > 0) {
            shouldRemoveOneIter = true;

            int numChoices = 3;
            bool canEnqueue = true;
            bool canDequeue = true;

            if(TQ.size() == 0) {
                canDequeue = false;
                numChoices--;
            }

            if(TQ.size()>=TQ.capacity() || _taskDepth>=_maxTaskDepth) {
                canEnqueue = false;
                numChoices--;
            }

            int r = rand()%numChoices;
            if(r == 0) {
                return Execute;
            }
            else if(r==1 && canEnqueue) {
                goalNumDequeues = TQ.numDequeues()+TQ.size()+1;
                return Enqueue;
            }
            /* the rest must be dequeue, so just fall back to the following */
        }

        shouldRemoveOneIter = false;
        return Dequeue;
    }
    int nextIterIdx() {
        return remainingIndices[numRemainingIndices-1];
    }
    void proceed() {
        if(shouldRemoveOneIter == true) {
            numRemainingIndices--;
        }
    }
};

void enqueueAndExec(function<void(int)> loopBody, int iterStart, int iterEnd) {
    int oldTaskDepth = _taskDepth;
    /* We can also use BFExecPolicy or DFExecPolicy here */
    for(BFExecPolicy ep(iterStart, iterEnd); ep.hasNext(); ep.proceed()) {
        switch(ep.nextAction()) {
            case Enqueue: {
                TQ.enqueue(Iter(loopBody, ep.nextIterIdx()));
                break;
            }
            case Dequeue: {
                Iter t = TQ.dequeue();
                
                _taskDepth++;
                t.loopBody(t.iterIdx);
                _taskDepth--;
                
                break;
            }
            case Execute: {
                _taskDepth++;
                loopBody(ep.nextIterIdx());
                _taskDepth--;
            }
        }
        assert(oldTaskDepth == _taskDepth);
    }
}

#define LOOP_BEGIN(_idx, _start, _end)  do { \
    int _st = _start; \
    int _ed = _end; \
    function<void(int)> _lb = [&](int _idx)

#define LOOP_END \
    ; enqueueAndExec(_lb, _st, _ed); } while(0)

#endif
