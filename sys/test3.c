#include <conf.h>
#include <kernel.h>
#include <q.h>
#include <stdio.h>

/* test3.c
 * This test program creates three processes, prA, prB, and prC, at
 * priority 10.  The main process has priority 20.
 *
 * The main routine then calls chprio to change the priorities of prA, prB
 * and prC to be 30, while it remains at priority 20.
 */
#define LOOP	50

int prch3(), prA, prB, prC;

int main3()
{
	int i, j=0;
	int s = 0;
	char buf[8];
	kprintf("Set Schedule Class 1-AGINGSCHED, 2-LINUXSCHED:");
	while ((i = read(CONSOLE, buf, sizeof(buf))) <1);
	buf[i] = 0;
	s = atoi(buf);
	kprintf("Get %d\n", s);
	setschedclass(s);

        kprintf("\n\nTEST3:\n");

	resume(prA = create(prch3,2000,10,"proc A",1,'A'));
	resume(prB = create(prch3,2000,10,"proc B",1,'B'));
	resume(prC = create(prch3,2000,10,"proc C",1,'C'));
	
	while (j++ < LOOP / 2) {
		kprintf("%c", 'D');
		for (i=0; i< 10000000; i++);
	}

	chprio(prA, 30);
	chprio(prB, 30);
	chprio(prC, 30);
	
	kprintf("\nPriority Changed...\n");

	while (j++ < LOOP) {
		kprintf("%c", 'D');
		for (i=0; i< 10000000; i++);
	}
	return OK;
}

prch3(c)
char c;
{
	int i, j=0;

	while(j++ < LOOP) {
		kprintf("%c", c);
		for (i=0; i< 10000000; i++);
	}
}
