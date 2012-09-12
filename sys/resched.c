/* resched.c  -  resched */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <q.h>
#include <stdio.h>
#include <math.h>

int sched_method = AGINGSCHED;

int rollover_threshold = 5;

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
	int oldpreempt = preempt;
	preempt = MAXINT;
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
		preempt = oldpreempt;
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
	int oldpreempt = preempt;
	preempt = MAXINT;
	
	/* Copy over however much counter the current process had left */
	int counter_diff = proctab[currpid].pcounter - oldpreempt;
	proctab[currpid].pcounter -= counter_diff;

	/* Check to see if there are any processes on the ready queue
 	   which are able to run and have quantum left */
	int done_with_epoch = 1;
	int item = q[rdytail].qprev;
	/* Loop over backwards, looking for can_run, pstate == PRREADY and
 	   counter > 0 */
	while (q[item].qprev != EMPTY) {
		/* If we find an eligible process, flag the epoch as not done */
		if (proctab[item].pcan_run && (proctab[item].pstate == PRREADY ||
		    proctab[item].pstate == PRCURR) && proctab[item].pcounter > 0) {
			done_with_epoch = 0;
			break;
		}
		/* Move on to the previous item */
		item = q[item].qprev;
	}

	/* If the epoch is over or we have no proceses eligible to execute for this epoch */
	if (done_with_epoch) {
		/* Make sure the null proc has a counter of 0 */
		proctab[NULLPROC].pcounter = 0;

		/* Loop over each process */
		int pid;
		for (pid=1; pid<NPROC; pid++) {
			if (proctab[pid].pstate == PRFREE)
				continue;

			/* Set this process as runnable, since we're starting a new epoch */
			proctab[pid].pcan_run = 1;
			/* Copy the priority over to the field to use during epochs, since
			   priority cannot change during runtime */
			proctab[pid].pprio2 = proctab[pid].pprio;
			/* If this process has quantum left over the threshold, factor that in */
			if (proctab[pid].pcounter >= rollover_threshold) {
				proctab[pid].pquantum = floor(proctab[pid].pcounter/2) + proctab[pid].pprio2;
				proctab[pid].pcounter = proctab[pid].pquantum;
			}
			/* Otherwise, set it's quantum counter to it's priority */
			else {
				proctab[pid].pquantum = proctab[pid].pcounter = proctab[pid].pprio2;
			}
			
			/* If this process is on the ready queue, make sure it's in the right
 			   spot since it's key may have changed */	
			if (proctab[pid].pstate == PRREADY) {
				int new_goodness = proctab[pid].pcounter + proctab[pid].pprio2;
				/* If the new value is between the processes before and after
 				   this one, then it's safe to just change the value */
				if (new_goodness >= q[q[pid].qprev].qkey &&
				    new_goodness <= q[q[pid].qnext].qkey) {
					q[pid].qkey = new_goodness;
				}
				/* If it's not though, we need to dequeue the process and
 				   insert it back in with the new goodness value */
				else {
					dequeue(pid);
					insert(pid, rdyhead, new_goodness);
				}
			}
		}
		
		/* Then put the current process back into the ready queue, if it wants to */
		if (optr->pstate == PRCURR) {
			optr->pstate = PRREADY;
			insert(currpid, rdyhead, proctab[currpid].pcounter + proctab[currpid].pprio2);
		}
	}
	/* Put the current process back into the ready queue */
	else if (optr->pstate == PRCURR) {
		int new_goodness = optr->pcounter > 0 ? optr->pcounter + optr->pprio2 : 0;
		if (new_goodness > lastkey(rdytail)) {
			if (currpid != 0)
				preempt = optr->pcounter;
			else
				preempt = QUANTUM;
			return;
		}
		optr->pstate = PRREADY;
		insert(currpid, rdyhead, new_goodness);
	}

	/* Get the item with the highest 'goodness' */
	currpid = getlast(rdytail);

	/* Otherwise, switch contexts to the new current process */
	nptr = &proctab[currpid];
	nptr->pstate = PRCURR;
#ifdef	RTCLOCK
	if (currpid != 0)
		preempt = nptr->pcounter;
	else
		preempt = QUANTUM;
#endif

	ctxsw((int)&optr->pesp, (int)optr->pirmask, (int)&nptr->pesp, (int)nptr->pirmask);

	return;
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
