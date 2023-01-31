/*
 * entry.S - Entry point to system mode from user mode
 */

#include <asm.h>
#include <segment.h>
#include <errno.h>

/**************************************************/
/**** Save & Restore ******************************/
/**                                              **/
/** When we change to privilege level 0 (kernel) **/
/** (through an interrupt, a system call, an     **/
/** exception ...) we must save the state of the **/
/** currently running task (save).               **/
/**                                              **/
/** Stack layout in 'systemCall':                **/
/**                                              **/
/**   0(%esp) - %ebx    \                        **/
/**   4(%esp) - %ecx     |                       **/
/**   8(%esp) - %edx     |                       **/
/**   C(%esp) - %esi     | Register saved        **/
/**  10(%esp) - %edi     |  by 'save'            **/
/**  14(%esp) - %ebp     |                       **/
/**  18(%esp) - %eax     |                       **/
/**  1C(%esp) - %ds      |                       **/
/**  20(%esp) - %es      |                       **/
/**  24(%esp) - %fs      |                       **/
/**  28(%esp) - %gs     /                        **/
/**  2C(%esp) - %eip    \                        **/
/**  30(%esp) - %cs      |                       **/
/**  34(%esp) - %eflags  |  Return context saved **/
/**  38(%esp) - %oldesp  |   by the processor.   **/
/**  3C(%esp) - %oldss  /                        **/
/**                                              **/
/**************************************************/

#define SAVE_ALL \
      pushl %gs; \
      pushl %fs; \
      pushl %es; \
      pushl %ds; \
      pushl %eax; \
      pushl %ebp; \
      pushl %edi; \
      pushl %esi; \
      pushl %edx; \
      pushl %ecx; \
      pushl %ebx; \
      movl $__KERNEL_DS, %edx;    \
      movl %edx, %ds;           \
      movl %edx, %es

#define RESTORE_ALL \
      popl %ebx; \
      popl %ecx; \
      popl %edx; \
      popl %esi; \
      popl %edi; \
      popl %ebp; \
      popl %eax; \
      popl %ds; \
      popl %es; \
      popl %fs; \
      popl %gs;

//Notifies the system that you are ready to receive new interrupts.
#define EOI \
	movb $0x20, %al; \
	outb %al, $0x20;


//Guardem el context i cridem a la rutina. No guardem els registres manualment perque es un INT (ho guarda automaticament) 
//i no un sysenter
ENTRY(keyboard_handler)
	SAVE_ALL
	EOI
	call keyboard_routine
	RESTORE_ALL
	iret


ENTRY(clock_handler)
	SAVE_ALL
	EOI
	call update_user_system_st
	call clock_routine
	call update_system_user_st
	RESTORE_ALL
	iret


ENTRY(syscall_handler_sysenter)
	push $__USER_DS
	push %EBP
	pushfl
	push $__USER_CS
	push 4(%EBP)
	SAVE_ALL
	cmpl $0, %EAX
	jl sysenter_err
	cmpl $MAX_SYSCALL, %EAX
	jg sysenter_err

	push %EAX
	call update_user_system_st
	pop %EAX

	call *sys_call_table(, %EAX, 0x04)

	push %EAX
	call update_system_user_st
	pop %EAX

	jmp sysenter_fin
sysenter_err:
	movl $-ENOSYS, %EAX
sysenter_fin:
	movl %EAX, 0x18(%ESP)
	RESTORE_ALL
	movl (%ESP), %EDX
	movl 12(%ESP), %ECX
	sti
	sysexit

ENTRY(writeMSR)
	pushl %ebp
	movl %esp, %ebp

	movl 8(%ebp), %eax
	movl 12(%ebp), %ecx
	movl $0, %edx
	wrmsr

	pop %ebp
	ret

ENTRY(system_call_handler)
	SAVE_ALL				//Save context
	cmpl $0, %EAX				//Check syscall number not negative
	jl err 					//If it is, return error
	cmpl $MAX_SYSCALL, %EAX		 	//Syscall number < MAX_SYSCALL
	jg err 					//If greater, return error
	call *sys_call_table(, %EAX, 0x04)	//Call service routine
	jmp fin					//Finish

err:
	movl $-ENOSYS, %EAX			//Move to EAX the ENOSYS error
fin:
	movl %EAX, 0x18(%ESP)			//Change the EAX value in the stack
	RESTORE_ALL				// Restore the context
	iret

