/*
 * sys.c - Syscalls implementation
 */
#include <devices.h>

#include <utils.h>

#include <io.h>

#include <mm.h>

#include <mm_address.h>

#include <sched.h>

#include <p_stats.h>

#include <list.h>

#include <errno.h>

#define LECTURA 0
#define ESCRIPTURA 1

void * get_ebp();
void * alloc();





struct sem_t {
	int count;
	struct list_head blocked;
};

struct sem_t semafors[10];
int used_sem[10] = {0};

//Buffer circular de tamany 8 (8 per les proves, després es canvia a més gran)
struct circular_buffer {
	int  max;
	char key_buffer[8];
	char * kb_read;
	char * kb_write;
	int count;
};

//Inicialització del buffer circular.
struct circular_buffer keystrokes = {8, "", &keystrokes.key_buffer[0], &keystrokes.key_buffer[0], 0};







int check_fd(int fd, int permissions)
{
  if (fd!=1) return -EBADF; 
  if (permissions!=ESCRIPTURA) return -EACCES; 
  return 0;
}

void user_to_system(void)
{
  update_stats(&(current()->p_stats.user_ticks), &(current()->p_stats.elapsed_total_ticks));
}

void system_to_user(void)
{
  update_stats(&(current()->p_stats.system_ticks), &(current()->p_stats.elapsed_total_ticks));
}

int sys_ni_syscall()
{
	return -ENOSYS; 
}

int sys_getpid()
{
	return current()->PID;
}

int global_PID=1000;
int global_TID=5000;

int ret_from_fork()
{
  return 0;
}





//Agafar el següent char disponible del buffer circular.
//comprobar direccion c sea de usuario
int sys_get_key(char *c)
{
	if (!access_ok(VERIFY_WRITE, c, sizeof(char *))) return -EFAULT; 
	if (keystrokes.count == 0) return -1; //No hi ha res a llegir + canviar error

	*c = *keystrokes.kb_read;

	//Si el punter es troba al final del buffer, es posiciona al principi del buffer. Si no, ++.
	if (keystrokes.kb_read == &keystrokes.key_buffer[7]) keystrokes.kb_read = &keystrokes.key_buffer[0];
	else ++keystrokes.kb_read;

	keystrokes.count--;

	return 1;
}


//Alocatar pàgina lógica per a current(). Es busca una pàgina lógica disponible.
void *aux_alloc()
{
	for (int i = PAG_LOG_INIT_DATA+NUM_PAG_DATA; i <= TOTAL_PAGES; ++i){
                int p = get_frame(get_PT(current()),i);
                if(p==0){
                        int f = alloc_frame();
			//No hi ha pàgines físiques disponibles -> error.
                        if (f < 0) return (void *) -EAGAIN;
                        set_ss_pag(get_PT(current()), i, f);
                        return (void *) (i<<12);
                }
        }
	//No hi ha pàgines lògiques disponibles -> error.
        return (void *) -EAGAIN;
}

//Desalocatar pàgina lògica amb direcció "address" de current().
int aux_dealloc(void *address) {
        int page = (int) address >> 12;
	//La direcció no correspon a una de les pàgines otorgades en aux_alloc -> error.
        if (page < (PAG_LOG_INIT_DATA + NUM_PAG_DATA)) return -EFAULT;
        else {
                free_frame(get_frame(get_PT(current()), page));
                del_ss_pag(get_PT(current()), page);
                return 1;
        }
}

void *sys_alloc(){
        return aux_alloc();
}

int sys_dealloc(void *address) {
        return aux_dealloc(address);
}

//Creació d'un thread: rep com a paràmetres una funció i el parametre d'aquesta funció.
int sys_createthread(int (*function) (void *param), void *param, int (*tw) (int *function, void *param)) {

  struct list_head *lhcurrent = NULL;
  union task_union *uchild;

  //Comprovació de si hi ha espai per a un nou thread.
  if (list_empty(&freequeue)) return -ENOMEM;

  lhcurrent=list_first(&freequeue);
  list_del(lhcurrent);

  //Task_union del nou thread.
  uchild=(union task_union*)list_head_to_task_struct(lhcurrent);

  //Copia del task union del thread actual al nou.
  copy_data(current(), uchild, sizeof(union task_union));

  //Asignaciò del seu TID + estat del thread.
  uchild->task.TID=++global_TID;
  uchild->task.state=ST_READY;

  //Alloc de una pàgina lògica per a la pila d'usuari del thread. Si no hi ha espai -> error.
  void * us = aux_alloc();
  if ((int) us < 0) return -EAGAIN;

  //Preparació context del thread per a retornar a mode usuari. Es modifica la pila d'usuari per tal de que quan es faci
  //restore del context. Es retorni a un wrapper per a l'execució de la funció. Aquest wrapper està pensat per executar
  //sempre "terminatethread()" al acabar l'execució de la funció.

  //Parametre, funció i %ebp.
  ((unsigned long *) us)[1023] = (unsigned long) param;
  ((unsigned long *) us)[1022] = (unsigned long) function;
  ((unsigned long *) us)[1021] = NULL;

  //Modificació de la pila de sistema: %EIP ara apunta a la funció thread_wrapper i %ESP a la cima de la pila d'usuari.
  uchild->stack[KERNEL_STACK_SIZE-2] = (long unsigned int) &((unsigned long *) us)[1021];
  uchild->stack[KERNEL_STACK_SIZE-5] = (unsigned long) tw;
  uchild->task.register_esp = (unsigned int) &(uchild->stack[KERNEL_STACK_SIZE-18]);

  //Stats a 0 i encolat a la readyqueue
  init_stats(&(uchild->task.p_stats));
  list_add_tail(&(uchild->task.list), &readyqueue);


  return uchild->task.TID;
}

//Terminar un thread
int sys_terminatethread()
{

  //Es desalocata la pagina utilitzada per a la pila d'usuari del thread.
  aux_dealloc((void *) ((union task_union*) current())->stack[KERNEL_STACK_SIZE-2]);

  //Encolat a la freequeue
  list_add_tail(&(current()->list), &freequeue);
  current()->TID=-1;

  //Restart execució of the next process.
  sched_next_rr();

  return 0;
}


int sys_dump_screen(void *address)
{
  if (address < (void *)0x100000) return -EFAULT; //Direccionamiento usuario
  copy_data(address, (void *) 0xb8000, 4000); //Copiar frame a pantalla
  return 1;
}


int valid_sem(int x)
{
  return (x > -1 && x < 10);
}

//Inicialitzar un semafor
int sys_sem_init(int n_sem, unsigned int value)
{
  //Return -1 si el semafor ja s'està utilitzant o si l'identificador no es valid (ha de ser entre 0 i 9)
  if (used_sem[n_sem] || !valid_sem(n_sem)) return -1;
  
  used_sem[n_sem] = 1;
  semafors[n_sem].count = value; 
  INIT_LIST_HEAD(&(semafors[n_sem].blocked));

  return 0; 
}


int sys_sem_wait(int n_sem)
{
  //Return -1 si l'identificador no es valid.
  if (!valid_sem(n_sem)) return -1;

  //current()->sem_dest = 0;

  int sc = --semafors[n_sem].count;
  if (sc < 0) {
    list_add_tail(&current()->list, &(semafors[n_sem].blocked));
    sched_next_rr();
  }

  if (current()->sem_dest) return -1; //ojo, sem destroyed.

  return 0;
}

int sys_sem_signal(int n_sem)
{
  if (!valid_sem(n_sem)) return -1;

  int sc = ++semafors[n_sem].count;
  if (sc <= 0){
    struct list_head *l = list_first(&(semafors[n_sem].blocked));
    list_del(l);
    list_add_tail(l, &readyqueue);
  }

  return 0;
}

//Elimina un semafor 
int sys_sem_destroy(int n_sem)
{
  if (!valid_sem(n_sem)) return -1;

  used_sem[n_sem] = -1;
  while (!list_empty(&(semafors)[n_sem].blocked))
  {
    struct list_head *l = list_first(&(semafors[n_sem].blocked));
    list_head_to_task_struct(l)->sem_dest = 1; //Desbloqueo por destroy
    list_del(l);
    list_add_tail(l, &readyqueue);
  }

  return 0;
}






int sys_fork(void)
{
  struct list_head *lhcurrent = NULL;
  union task_union *uchild;
  
  /* Any free task_struct? */
  if (list_empty(&freequeue)) return -ENOMEM;

  lhcurrent=list_first(&freequeue);
  
  list_del(lhcurrent);
  
  uchild=(union task_union*)list_head_to_task_struct(lhcurrent);
  
  /* Copy the parent's task struct to child's */
  copy_data(current(), uchild, sizeof(union task_union));
  
  /* new pages dir */
  allocate_DIR((struct task_struct*)uchild);
  
  /* Allocate pages for DATA+STACK */
  int new_ph_pag, pag, i;
  page_table_entry *process_PT = get_PT(&uchild->task);
  for (pag=0; pag<NUM_PAG_DATA; pag++)
  {
    new_ph_pag=alloc_frame();
    if (new_ph_pag!=-1) /* One page allocated */
    {
      set_ss_pag(process_PT, PAG_LOG_INIT_DATA+pag, new_ph_pag);
    }
    else /* No more free pages left. Deallocate everything */
    {
      /* Deallocate allocated pages. Up to pag. */
      for (i=0; i<pag; i++)
      {
        free_frame(get_frame(process_PT, PAG_LOG_INIT_DATA+i));
        del_ss_pag(process_PT, PAG_LOG_INIT_DATA+i);
      }
      /* Deallocate task_struct */
      list_add_tail(lhcurrent, &freequeue);
      
      /* Return error */
      return -EAGAIN; 
    }
  }

  /* Copy parent's SYSTEM and CODE to child. */
  page_table_entry *parent_PT = get_PT(current());
  for (pag=0; pag<NUM_PAG_KERNEL; pag++)
  {
    set_ss_pag(process_PT, pag, get_frame(parent_PT, pag));
  }
  for (pag=0; pag<NUM_PAG_CODE; pag++)
  {
    set_ss_pag(process_PT, PAG_LOG_INIT_CODE+pag, get_frame(parent_PT, PAG_LOG_INIT_CODE+pag));
  }
  /* Copy parent's DATA to child. We will use TOTAL_PAGES-1 as a temp logical page to map to */
  for (pag=NUM_PAG_KERNEL+NUM_PAG_CODE; pag<NUM_PAG_KERNEL+NUM_PAG_CODE+NUM_PAG_DATA; pag++)
  {
    /* Map one child page to parent's address space. */
    set_ss_pag(parent_PT, pag+NUM_PAG_DATA, get_frame(process_PT, pag));
    copy_data((void*)(pag<<12), (void*)((pag+NUM_PAG_DATA)<<12), PAGE_SIZE);
    del_ss_pag(parent_PT, pag+NUM_PAG_DATA);
  }
  /* Deny access to the child's memory space */
  set_cr3(get_DIR(current()));

  uchild->task.PID=++global_PID;
  uchild->task.TID=++global_TID;
  uchild->task.state=ST_READY;

  int register_ebp;		/* frame pointer */
  /* Map Parent's ebp to child's stack */
  register_ebp = (int) get_ebp();
  register_ebp=(register_ebp - (int)current()) + (int)(uchild);

  uchild->task.register_esp=register_ebp + sizeof(DWord);

  DWord temp_ebp=*(DWord*)register_ebp;
  /* Prepare child stack for context switch */
  uchild->task.register_esp-=sizeof(DWord);
  *(DWord*)(uchild->task.register_esp)=(DWord)&ret_from_fork;
  uchild->task.register_esp-=sizeof(DWord);
  *(DWord*)(uchild->task.register_esp)=temp_ebp;

  /* Set stats to 0 */
  init_stats(&(uchild->task.p_stats));

  /* Queue child process into readyqueue */
  uchild->task.state=ST_READY;
  list_add_tail(&(uchild->task.list), &readyqueue);
  
  return uchild->task.PID;
}

#define TAM_BUFFER 512

int sys_write(int fd, char *buffer, int nbytes) {
char localbuffer [TAM_BUFFER];
int bytes_left;
int ret;

	if ((ret = check_fd(fd, ESCRIPTURA)))
		return ret;
	if (nbytes < 0)
		return -EINVAL;
	if (!access_ok(VERIFY_READ, buffer, nbytes))
		return -EFAULT;
	
	bytes_left = nbytes;
	while (bytes_left > TAM_BUFFER) {
		copy_from_user(buffer, localbuffer, TAM_BUFFER);
		ret = sys_write_console(localbuffer, TAM_BUFFER);
		bytes_left-=ret;
		buffer+=ret;
	}
	if (bytes_left > 0) {
		copy_from_user(buffer, localbuffer,bytes_left);
		ret = sys_write_console(localbuffer, bytes_left);
		bytes_left-=ret;
	}
	return (nbytes-bytes_left);
}


extern int zeos_ticks;

int sys_gettime()
{
  return zeos_ticks;
}

void sys_exit()
{  
  int i;

  page_table_entry *process_PT = get_PT(current());

  // Deallocate all the propietary physical pages
  for (i=0; i<NUM_PAG_DATA; i++)
  {
    free_frame(get_frame(process_PT, PAG_LOG_INIT_DATA+i));
    del_ss_pag(process_PT, PAG_LOG_INIT_DATA+i);
  }
  
  /* Free task_struct */
  list_add_tail(&(current()->list), &freequeue);
  
  current()->PID=-1;
  
  /* Restarts execution of the next process */
  sched_next_rr();
}

/* System call to force a task switch */
int sys_yield()
{
  force_task_switch();
  return 0;
}

extern int remaining_quantum;

int sys_get_stats(int pid, struct stats *st)
{
  int i;
  
  if (!access_ok(VERIFY_WRITE, st, sizeof(struct stats))) return -EFAULT; 
  
  if (pid<0) return -EINVAL;
  for (i=0; i<NR_TASKS; i++)
  {
    if (task[i].task.PID==pid)
    {
      task[i].task.p_stats.remaining_ticks=remaining_quantum;
      copy_to_user(&(task[i].task.p_stats), st, sizeof(struct stats));
      return 0;
    }
  }
  return -ESRCH; /*ESRCH */
}


