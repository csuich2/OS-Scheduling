/* chprio.c - chprio */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <q.h>
#include <stdio.h>

/*------------------------------------------------------------------------
 * chprio  --  change the scheduling priority of a process
 *------------------------------------------------------------------------
 */
SYSCALL chprio(int pid, int newprio)
{
	STATWORD ps;    
	struct	pentry	*pptr;

	disable(ps);
	if (isbadpid(pid) || newprio<=0 ||
	    (pptr = &proctab[pid])->pstate == PRFREE) {
		restore(ps);
		return(SYSERR);
	}
	pptr->pprio = newprio;
	// If we are currently using the aging scheduler...
	if (getschedclass() == AGINGSCHED) {
		// We want to updated the queue priority now
		// (the linux-like scheduler will do that itself)
		q[pid].qkey = newprio;
		// If this priority is in the ready queue...
		if (isinqueue(pid, rdyhead)) {
			// Then we need to remove it from the queue
			dequeue(pid);
			// And put it back in so that the queue
			// will remain sorted
			insert(pid, rdyhead, newprio);
		}
	}
	restore(ps);
	return(newprio);
}
