/*
 * sys.c - Syscalls implementation
 */
#include <devices.h>

#include <utils.h>

#include <io.h>

#include <mm.h>

#include <mm_address.h>

#include <sched.h>

#include <list.h>

#include <errno.h>

#define LECTURA 0
#define ESCRIPTURA 1

extern int zeos_ticks;
extern struct list_head freequeue;
extern struct list_head readyqueue;

int check_fd(int fd, int permissions)
{
  if (fd!=1) return -EBADF; /*EBADF*/
  if (permissions!=ESCRIPTURA) return -EACCES; /*EACCES*/
  return 0;
}

int sys_ni_syscall()
{
	return -ENOSYS; /*ENOSYS*/
}

int sys_getpid()
{
	return current()->PID;
}



int ret_from_fork()
{
	return 0;
}

int currentpid = 1000;

int sys_fork()
{

  // creates the child process
	//Comprobar si la freequeue esta vacia
	if(list_empty(&freequeue)) return -ENOMEM;

	//Coger un task_struct de la lista
	struct list_head *e = list_first(&freequeue);
        list_del(e);

	//task_struct del hijo
	struct task_struct *child_tks = list_head_to_task_struct(e);
        //task union del hijo
	union task_union *child_tku = (union task_union*) child_tks;

	//Copia task_union del padre al hijo
	copy_data(current(), child_tku, sizeof(union task_union));
	//Inicializar directorio hijo
       	allocate_DIR(child_tks);


	page_table_entry *child_PT = get_PT(child_tks);

	int pages[NUM_PAG_DATA];
	for (int i = 0; i < NUM_PAG_DATA; ++i)
	{
		pages[i] = alloc_frame();
		if (pages[i] < 0){
			for (int j = i; j >= 0; --j){
				free_frame(pages[j]); //Liberar los frames que se iban a utilizar.
			}
			list_add_tail(&child_tku->task.list, &freequeue);
			return -EAGAIN;
		}
	}


	page_table_entry *current_PT = get_PT(current());

	//ni idea de si es necesario
	for(int i=0; i<NUM_PAG_KERNEL; ++i)
	{
		set_ss_pag(child_PT, i, get_frame(current_PT,i));
	}
	//Hijo y padre comparten codigo de usuario, por lo tanto las paginas logicas correspondientes al codigo del hijo apuntan las
	//paginas fisicas del codigo del padre
	for (int i=0; i<NUM_PAG_CODE; ++i)
	{
		set_ss_pag(child_PT, PAG_LOG_INIT_CODE+i, get_frame(current_PT, PAG_LOG_INIT_CODE+i));
	}


	//No compartiran data, por lo tanto se asignan las nuevas paginas fisicas del hijo y luego en una región del padre tambien se
	//asignan paginas logicas (de una zona inmediatamente despues a su data) a las mismas paginas fisicas del hijo
	//Esto sirve para poder hacer la copia de la data del padre al hijo.

	for (int i=0; i< NUM_PAG_DATA; ++i)
	{
		set_ss_pag(child_PT, PAG_LOG_INIT_DATA+i, pages[i]);
		set_ss_pag(current_PT, PAG_LOG_INIT_DATA+NUM_PAG_DATA+i, pages[i]);
	}

	//Copy data del padre a la nueva zona del padre que comparte con su hijo
	copy_data((void*) (PAG_LOG_INIT_DATA*PAGE_SIZE), (void *) ((NUM_PAG_DATA+PAG_LOG_INIT_DATA)*PAGE_SIZE), NUM_PAG_DATA*PAGE_SIZE);

	//Borrar esas paginas logicas del padre, ahora el hijo ya tiene su data y el padre no las necesita.
	for (int i=0; i < NUM_PAG_DATA; ++i)
	{
		del_ss_pag(current_PT,i+NUM_PAG_DATA+PAG_LOG_INIT_DATA);
	}

	set_cr3(get_DIR(current()));

	child_tks->PID = currentpid;
	++currentpid;


	child_tku->stack[KERNEL_STACK_SIZE-18] = (unsigned int) &ret_from_fork;
	child_tku->stack[KERNEL_STACK_SIZE-19] = 0;
	child_tks->esp_tks = (unsigned int) &(child_tku->stack[KERNEL_STACK_SIZE-19]);


	list_add_tail(&(child_tks->list), &readyqueue);
  	return child_tks->PID;
}

void sys_exit()
{
	page_table_entry *current_PT = get_PT(current());

	for (int i=0; i < NUM_PAG_DATA; ++i)
	{
		free_frame(get_frame(current_PT, i+PAG_LOG_INIT_DATA));
		del_ss_pag(current_PT,i+PAG_LOG_INIT_DATA);
	}

	//La informacion del task_struct se va a sobreescribir
	current()->PID = -1;

	update_process_state_rr(current(),&freequeue);
	sched_next_rr();
}

char dest_buffer[512];

int sys_write(int fd, char * buffer, int size)
{
	int error_fd = check_fd(fd,ESCRIPTURA);
	if (error_fd < 0) return error_fd;

	if (buffer == NULL) return -EFAULT;
	if (size < 0) return -EINVAL;

	int bytes = size;
	int mida = 0;

	//Llegim de 512 en 512 bytes.
	while (bytes > 0) {
		//Si son menys de 512 bytes, ajustem la mida.
		if (bytes >= 512) mida = 512;
		else mida = bytes;

		copy_from_user(buffer, dest_buffer, mida);
		//Retorna num de bytes escrits
		int nbytes = sys_write_console(dest_buffer, mida);

		//bytes < 0 quan hagi acabat
		bytes -= 512;
		//Anem accedint a la direccio de memoria + els bytes llegits.
		buffer = buffer + nbytes;
	}

	return size;
}

int sys_getstats(int p, struct stats *st)
{
	if(!access_ok(VERIFY_WRITE, st, sizeof(struct stats))) return -EFAULT;
	for(int i=0; i < NR_TASKS; ++i)
	{
		if (task[i].task.PID == p)
		{
			copy_to_user(&(task[i].task.st), st, sizeof(struct stats));
			return 0;
		}
	}

	return -ESRCH;
}

int sys_gettime()
{
	return zeos_ticks;
}
