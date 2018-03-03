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
        return iterStart<iterEnd || TQ.numDequeues()<goalNumDequeues;
    }
    Action nextAction() {
        if(iterStart < iterEnd) {
            if(_taskDepth<_maxTaskDepth && TQ.size()<TQ.capacity()) {
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
    for(RandomExecPolicy ep(iterStart, iterEnd); ep.hasNext(); ep.proceed()) {
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
