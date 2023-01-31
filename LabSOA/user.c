#include <libc.h>

//char buff[48];

int pid;

int add(int par1, int par2) {
	return par1 + par2;
}

int addASM(int, int);

int __attribute__ ((__section__(".text.main")))
  main(void)
{
    /* Next line, tries to move value 0 to CR3 register. This register is a privileged one, and so it will raise an exception */
    /* __asm__ __volatile__ ("mov %0, %%cr3"::"r" (0) ); */

  // int resultat = addASM(0x42,0x666);

  /*
    int x = 90000000;
    while(--x);
    if (write(1, "hola", -1) < 0) perror();
  */

  //itoa(getpid(), buff);
  //write(1,buff,strlen(buff));
  //write(1,buff,strlen(buff));
  char * buff;

	int f = fork();
	if (f == 0)
	{
		buff ="I am the child :) and my PID is: ";
		write(1,buff,strlen(buff));
		itoa(getpid(),buff);
		write(1,buff,strlen(buff));
		buff="\n";
		write(1,buff,strlen(buff));
		//exit();
		buff="I am still alive, pls call exit\n";
		write(1,buff,strlen(buff));

	} else
	{
		buff ="I am the father >:( and my PID is: ";
		write(1,buff,strlen(buff));
		itoa(getpid(),buff);
		write(1,buff,strlen(buff));
		buff="\n";
		write(1,buff,strlen(buff));
		exit();
	}
  while(1);
}

