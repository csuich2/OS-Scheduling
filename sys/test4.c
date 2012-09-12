#include <conf.h>
#include <kernel.h>
#include <q.h>
#include <stdio.h>

/* test4.c
 * This test program creates three processes, prA, prB, and prC, at
 * priorities 10, 9, and 10 respectively.  The main process has priority 20.
 *
 * The main routine then calls chprio to change the priority of prB to be 15,
 * while it changes its own priority to 5.
 */
#define LOOP	50
int prch4(), prch5(), prA, prB, prC, prD;

int main()
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

        kprintf("\n\nTEST4:\n");

	resume(prA = create(prch4,2000,10,"proc A",1,'A'));
	resume(prB = create(prch4,2000, 9,"proc B",1,'B'));
	resume(prC = create(prch5,2000,10,"proc C",1,'C'));
	
	while (j++ < LOOP / 2) {
		kprintf("%c", 'D');
		for (i=0; i< 10000000; i++);
	}

	chprio(prB,      15);
	chprio(getpid(),  5);
	
	resume(prD = create(prch4,2000,70,"proc E",1,'E'));
	kprintf("%d going to sleep\n", getpid());
	sleep(10);
	while (j++ < LOOP) {
		kprintf("%c", 'D');
		for (i=0; i< 10000000; i++);
	}
	return OK;
}

prch4(c)
char c;
{
	int i, j=0;

	while(j++ < LOOP) {
		kprintf("%c", c);
		for (i=0; i< 10000000; i++);
	}
}

prch5(c)
char c;
{
	int i, j=0;
	while(j++ < LOOP) {
		kprintf("%c", c);

		if (j == 25)
			sleep(5); 
		for (i=0; i< 10000000; i++);
	}
}
