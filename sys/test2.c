#include <conf.h>
#include <kernel.h>
#include <q.h>
#include <stdio.h>

/* test2.c
 * This test program creates three processes, prA, prB, and prC, at
 * priority 30.  The main process has priority 20.
 */

#define LOOP	50

int prch2(), prA, prB, prC;

int main2()
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

        kprintf("\n\nTEST2:\n");

	prA = create(prch2,2000,30,"proc A",1,'A');
	prB = create(prch2,2000,30,"proc B",1,'B');
	prC = create(prch2,2000,30,"proc C",1,'C');

	resume(prC);
	resume(prB);
	resume(prA);

	while(j++ < LOOP) {
		kprintf("%c", 'D');
		for (i = 0; i< 10000000; i++);
	}
	return OK;
}

prch2(c)
char c;
{
	int i, j=0;

	while(j++ < LOOP) {
		kprintf("%c", c);
		for (i = 0; i< 10000000; i++);
	}
}
