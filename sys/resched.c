/* resched.c  -  resched */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <q.h>
#include <stdio.h>
#include <math.h>

int sched_method = AGINGSCHED;

int rollover_threshold = 1;
//int epoch_remaining = 0;

unsigned long currSP;	/* REAL sp of current process */
extern int ctxsw(int, int, int, int);

/* Some 'private' methods*/
void _resched_aging(struct pentry *optr, struct pentry *nptr);
void _resched_linux(struct pentry *optr, struct pentry *nptr);

/*-----------------------------------------------------------------------
 * resched  --  reschedule processor
 *
 * Notes:	Upon entry, currpid gives current process id.
 *		Proctab[currpid].pstate gives correct NEXT state for
 *			current process if other than PRREADY.
 *-----------------------------------------------------------------------
 */
int resched()
{
	register struct pentry	*optr;	/* pointer to old process entry */
	register struct pentry  *nptr;	/* pointer to new process entry */

	optr = &proctab[currpid];
	
	if (sched_method == AGINGSCHED) {
		_resched_aging(optr, nptr);
	} else if (sched_method == LINUXSCHED) {
		_resched_linux(optr, nptr);
	} else {
		return SYSERR;
	}

	return OK; 
}

void _resched_aging(struct pentry *optr, struct pentry *nptr)
{
	/* Loop through everything in the queue and increase each entries
	   priority by a factor of 1.2 */
	int next = q[rdyhead].qnext;
	while (q[next].qnext != EMPTY) {
		q[next].qkey = min(MAX_PRIORITY, ceil(q[next].qkey * 1.2));
		next = q[next].qnext;
	}

	/* Put the current process back into the queue with its base
           priority */
	if (optr->pstate == PRCURR) {
		optr->pstate = PRREADY;
		insert(currpid, rdyhead, optr->pprio);
	}
	/* Now get the item with the highest priority */
	int oldcurrpid = currpid;
	currpid = getlast(rdytail);

	/* If the process from the end of the list is the current process,
	   no context switch is necessary */
	if (currpid == oldcurrpid) {
		optr->pstate = PRCURR;
		return;
	}

	/* Otherwise, switch contexts to the new current process */
	nptr = &proctab[currpid];
	nptr->pstate = PRCURR;
#ifdef 	RTCLOCK
	preempt = QUANTUM;		/* Reset preempt counter */
#endif

	ctxsw((int)&optr->pesp, (int)optr->pirmask, (int)&nptr->pesp, (int)nptr->pirmask);

	return;
}

void _resched_linux(struct pentry *optr, struct pentry *nptr)
{

}

/*-----------------------------------------------------------------------
 * setschedclass(int) -- change scheduling class
 *
 * Notes:	Should be either AGINGSCHED or LINUXSCHED
 *-----------------------------------------------------------------------
 */
void setschedclass(int sched_class)
{
	/* Bail out if the parameter was not one of the accepted values */
	if (sched_class != AGINGSCHED && sched_class != LINUXSCHED) {
		return;
	}
	
	sched_method = sched_class;
}

/*-----------------------------------------------------------------------
 * getschedclass -- get scheduling class
 *
 * Notes:	Should be either AGINGSCHED or LINUXSCHED
 *-----------------------------------------------------------------------
 */
int getschedclass()
{
	return sched_method;

}
/*-----------------------------------------------------------------------
 * setrolloverthreshold -- sets the rollover threshold for LINUXSCHED
 *-----------------------------------------------------------------------
 */
void setrolloverthreshold(int threshold)
{
	rollover_threshold = threshold;
}
