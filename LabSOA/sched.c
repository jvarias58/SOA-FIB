/*
 * sched.c - initializes struct for task 0 anda task 1
 */

#include <sched.h>
#include <mm.h>
#include <io.h>
#include <utils.h>

#define QUANTUM_TIME 10

struct list_head freequeue;
struct list_head readyqueue;
void writeMSR();
void inner_task_switch2();

union task_union task[NR_TASKS]
  __attribute__((__section__(".data.task")));

struct task_struct *idle_task;

struct task_struct *list_head_to_task_struct(struct list_head *l)
{
	return (struct task_struct*) ((int)l&0xfffff000);

	  //return list_entry( l, struct task_struct, list);
}


extern struct list_head blocked;



//si s = 0
void update_st(int s)
{
	int gt = get_ticks();
	if (s)
	{
		current()->st.system_ticks += gt - current()->st.elapsed_total_ticks;

	} else
	{
		current()->st.user_ticks += gt - current()->st.elapsed_total_ticks;
	}

  	current()->st.elapsed_total_ticks = gt;
}

void update_user_system_st(void)
{
        update_st(0);
}

void update_system_user_st(void)
{
        update_st(1);
}



void init_st(struct stats *st)
{
        st->user_ticks = 0;
        st->system_ticks = 0;
        st->blocked_ticks = 0;
        st->ready_ticks = 0;
        st->elapsed_total_ticks = get_ticks();
        st->total_trans = 0;
        st->remaining_ticks = 0;
}


/* get_DIR - Returns the Page Directory address for task 't' */
page_table_entry * get_DIR (struct task_struct *t) 
{
	return t->dir_pages_baseAddr;
}

/* get_PT - Returns the Page Table address for task 't' */
page_table_entry * get_PT (struct task_struct *t) 
{
	return (page_table_entry *)(((unsigned int)(t->dir_pages_baseAddr->bits.pbase_addr))<<12);
}


int allocate_DIR(struct task_struct *t) 
{
	int pos;

	pos = ((int)t-(int)task)/sizeof(union task_union);

	t->dir_pages_baseAddr = (page_table_entry*) &dir_pages[pos]; 

	return 1;
}

void cpu_idle(void)
{
	__asm__ __volatile__("sti": : :"memory");

	while(1)
	{
	;
	}
}

//Al vector task tenim tots els task_union. Al vector freequeue tenim punters que apunten als task_union del vector task.
//Agafem un punter de la cua freequeue, el treiem d'aquesta cua i obtenim el task_struct al que fa referencia.
//Al stack del task_union afegim la direcció de retorn (cpu_idle) i ebp=0.
//esp_tks apunta a ebp.
void init_idle (void)
{

	struct list_head *e = list_first(&freequeue);
	list_del(e);
	struct task_struct *tks = list_head_to_task_struct(e);
	union task_union *tku = (union task_union*) tks;
	tks->PID = 0;
	allocate_DIR(tks);

	init_st(&tks->st);

	tku->stack[1023] = (unsigned int) &cpu_idle;
	tku->stack[1022] = 0;
	tks->esp_tks = (unsigned int) &tku->stack[1022];

	idle_task = tks;
}

void init_task1(void)
{
	struct list_head *e = list_first(&freequeue);
        list_del(e);
        struct task_struct *tks = list_head_to_task_struct(e);
        //union task_union *tku = (union task_union*) tks;
        tks->PID = 1;
        allocate_DIR(tks);
	tks->quantum = QUANTUM_TIME;
	set_user_pages(tks);

	init_st(&tks->st);

	tss.esp0 = KERNEL_ESP((union task_union*)tks);
	writeMSR((int)tss.esp0, 0x175);
	set_cr3(tks->dir_pages_baseAddr);

}


void init_sched()
{
	INIT_LIST_HEAD(&freequeue);
	INIT_LIST_HEAD(&readyqueue);
	for (int i = 0; i < NR_TASKS; ++i)
	{
		list_add(&(task[i].task.list),&(freequeue));

	}
}

int get_quantum(struct task_struct *tks)
{
	return tks->quantum;
}

void set_quantum(struct task_struct *tks, int new_quantum)
{
	tks->quantum = new_quantum;
}

//cada tick de reloj reducimos el quantum del current.
void update_sched_data_rr(void)
{
	--current()->quantum;
	current()->st.remaining_ticks = current()->quantum;
}

int needs_sched_rr(void)
{
	if (get_quantum(current()) > 0) return 0;

	if (list_empty(&readyqueue))
	{
		current()->quantum = QUANTUM_TIME;
		return 0;
	}

	return 1;

}

void update_process_state_rr(struct task_struct *t, struct list_head *dst_queue)
{
	if (dst_queue == NULL)
	{
		list_del(&(t->list));
	} else
	{
		list_add_tail(&(t->list), dst_queue);
	}
}


void sched_next_rr(void)
{
	struct task_struct *f;
	if (!list_empty(&readyqueue))
	{
		struct list_head *first = list_first(&readyqueue);
		f = list_head_to_task_struct(first);
		update_process_state_rr(f, NULL);
	} else
	{
		f = idle_task;
	}

	f->quantum = QUANTUM_TIME;
	int gt = get_ticks();


	current()->st.system_ticks += gt - current()->st.elapsed_total_ticks;
	current()->st.elapsed_total_ticks = gt;
	f->st.ready_ticks += gt - f->st.elapsed_total_ticks;
	f->st.elapsed_total_ticks = gt;
	f->st.total_trans += 1;

	task_switch((union task_union*) f);

}

void schedule()
{
	update_sched_data_rr();
	if (needs_sched_rr())
	{
		update_process_state_rr(current(), &readyqueue);
		sched_next_rr();
	}
}

void inner_task_switch(union task_union *t)
{
	tss.esp0 = (int)&(t->stack[KERNEL_STACK_SIZE]);
	writeMSR(tss.esp0, 0x175);
	set_cr3(get_DIR(&(t->task)));

	inner_task_switch2(&current()->esp_tks, t->task.esp_tks);

}


struct task_struct* current()
{
  int ret_value;

  __asm__ __volatile__(
  	"movl %%esp, %0"
	: "=g" (ret_value)
  );
  return (struct task_struct*)(ret_value&0xfffff000);
}

