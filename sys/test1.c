#include <conf.h>
#include <kernel.h>
#include <q.h>
#include <stdio.h>

/* test1.c
 * This test program creates three processes, prA, prB, and prC, at
 * priority 20.  The main process also has priority 20.
 */

#define LOOP	50
 
int prch1(), prA, prB, prC;

int main1()
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
	
	kprintf("\n\nTEST1:\n");
	
	resume(prA = create(prch1,2000,20,"proc A",1,'A'));
	resume(prB = create(prch1,2000,20,"proc B",1,'B'));
	resume(prC = create(prch1,2000,20,"proc C",1,'C'));

	while(j++ < LOOP) {
		kprintf("%c", 'D');
		for (i=0; i< 10000000; i++);
	}

	return OK;
}

int prch1(c)
char c;
{
	int i, j=0;

	while(j++ < LOOP) {
		kprintf("%c", c);
		for (i=0; i< 10000000; i++);
	}

	return OK;
}
