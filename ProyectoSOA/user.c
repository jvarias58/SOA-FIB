#include <libc.h>
#include <types.h>
#include <list.h>

char buff[48];
char * bf;

int pid;
int nframes = 0;

//SEMAFORS
int bs = 1;

int fps = 0;

struct list_head frames;

struct pantallote{
	struct list_head punter;
	Word frame[2000];
};

//THREADS
void threadwrapper(int (*function) (void *param), void *param) {
        function(param);
        terminatethread();
}


void sum(int a)
{
	//THREADS I SEMAFORS

	/*
	char q;
	sem_wait(1);
	char * bf;

	if (bs == 1) {
		int x = 9999999;
		while(--x)
		{
			if (x == 1) {
				get_key(&q);
			}
		}

		bs = 0;
		bf = "Hey, im the first thread\n";
		//sem_signal(1);S
		//sem_destroy(1);
		write(1,bf,strlen(bf));
	} else
	{

		bf = "Hey, im the second thread\n";
		write(1,bf,strlen(bf));
	}
	*/
}


void pintar(void * param){
	while (1) {
		sem_wait(1); //Semafor recursos: es desactiva quan hi ha frames per pintar.
		sem_wait(2); //Semafor exclusió mutua

		struct pantallote * p = (struct pantallote *) list_first(&frames);
		--nframes;
		list_del(&(p->punter));

		sem_signal(2);


		dump_screen(&(p->frame));
		++fps;
		dealloc(p);

	}
}

int avoid_stats(int xpos, int ypos) {
	if (ypos < 3 && xpos > 68) return -1;
	return 1;
}

void game()
{
	//CROSSHAIR COORDS
	int cx = 0;
	int cy = 0;

	//X COORDS
	int px = 40;
	int py = 12;

	int punts = 0;
	char key;
	int play = 1;

	while(play)
	{
		
		//Moure crosshair
		get_key(&key);
		switch(key) {
			case 'w':
				if (cy > 0 && !(cx > 68 && cy == 3)) --cy;
				break;

			case 'a':
				if (cx > 0) --cx;
				break;

			case 's':
				if (cy < 24) ++cy;
				break;

			case 'd':
				if (cx < 79 && !(cy < 3 && cx == 68)) ++cx;
				break;

			case 'c':
				play = 0;
				break;

			default:
				break;
		}

		key = 'p';

		struct pantallote * p = alloc();




		int k = 0;
		for(int j = 0; j < 25; ++j){
			for (int i = 0; i < 80; ++i){
				p->frame[k] = 0x3F00;
				++k;
			}
		}
		p->frame[cy*80 + cx] = 0x31E9;
		p->frame[py*80 + px] = 0x3458;

		int f = 0;
		if (gettime()/18 != 0) f = (fps/((gettime()/18)));


		char buff6[2];
		itoa(cx,buff6);


		//STATS

		//pos:
		p->frame[69] = 0x3070;
		p->frame[70] = 0x306f;
		p->frame[71] = 0x3073;
		p->frame[72] = 0x303a; 

		//frm:
		p->frame[69+80] = 0x3066;
		p->frame[70+80] = 0x3072;
		p->frame[71+80] = 0x306d;
		p->frame[72+80] = 0x303a; 

		//fps:
		p->frame[69+160] = 0x3066;
		p->frame[70+160] = 0x3070;
		p->frame[71+160] = 0x3073;
		p->frame[72+160] = 0x303a; 

		//Position
		p->frame[74] = (Word) buff6[0] | 0x3000;
		p->frame[75] = (Word) buff6[1] | 0x3000;

		itoa(cy,buff6);
		p->frame[77] = (Word) buff6[0] | 0x3000;
		p->frame[78] = (Word) buff6[1] | 0x3000;


		//nframes
		char buff7[4];
		itoa(nframes,buff7);
		/*
		write(1,buff7,strlen(buff7));
		char * brd = "/n";
		write(1,brd,1);
		*/
		p->frame[75+80] = (Word) buff7[0] | 0x3000;
		p->frame[76+80] = (Word) buff7[1] | 0x3000;
		p->frame[77+80] = (Word) buff7[2] | 0x3000;
		p->frame[78+80] = (Word) buff7[3] | 0x3000;


		//fps
		itoa(f,buff7);
		p->frame[75+160] = (Word) buff7[0] | 0x3000;
		p->frame[76+160] = (Word) buff7[1] | 0x3000;
		p->frame[77+160] = (Word) buff7[2] | 0x3000;
		p->frame[78+160] = (Word) buff7[3] | 0x3000;


		if (cx==px && cy == py){
			++punts; 
			px = gettime()%68;
			py = gettime()%20 + 5;
		}

		sem_wait(2); //Semafor exclusió mutua
		list_add_tail(&(p->punter), &frames);
		++nframes;
		sem_signal(2);

		sem_signal(1); //+1 quan hi ha un frame llest per pintar.

	}

}


int __attribute__ ((__section__(".text.main")))
  main(void)
{
    /* Next line, tries to move value 0 to CR3 register. This register is a privileged one, and so it will raise an exception */
     /* __asm__ __volatile__ ("mov %0, %%cr3"::"r" (0) ); */



	//ALLOC/DEALLOC

	/*
	int page;
	char buff2[24];
	itoa(page = (int)alloc(),buff);
	write(1,buff,sizeof(buff));

	itoa(dealloc((void *) page),buff2);
	write(1,buff2,sizeof(buff2));
	*/

	INIT_LIST_HEAD(&frames);


	sem_init(1,0);
	sem_init(2,1);
	createthread((void *) &pintar, NULL);
	game();


	//KEYBOARD

	/*
	char q;
	int x = 99999999;
	while(--x)
	{

		if (x == 1) {
			get_key(&q);
			buff[0] = q;
			write(1,buff,1);
		}
	}

	for (int i = 0; i < 10; ++i)
	{
		int b = get_key(&q);
        	buff[0] = q;
        	if (b > 0) write(1,buff,1);
	}

	x = 99999999;
	while(--x)
	{
		if (x == 1) {
			get_key(&q);
			buff[0] = q;
			write(1,buff,1);
		}
	}

	for (int i = 0; i < 10; ++i)
        {
                int b = get_key(&q);
                buff[0] = q;
                if (b > 0) write(1,buff,1);
        }
	*/



	//SEMAFORS

	/*
	sem_init(1,1);
	createthread((void *) &sum, (void *)1);
	createthread((void *) &sum, (void *)2);
	*/



	//THREADS

	//int b = createthread((void*) &sum, (void *)4);
	//itoa(b,buff);
	//write(1,buff,sizeof(buff));
	//buff[0] = 'k';
	//write(1,buff,sizeof(buff));



	//DUMP_SCREEN

	/*
	char buff5[2000];
	for (int i = 0; i < 2000; ++i)
	{
		buff5[i] = 'a';
	}
	buff5[0] = 'b';
	buff5[1999] = 'b';
	void * uf = &buff5;
	int a = dump_screen(uf);
	*/
	
	while(1) { }
}
