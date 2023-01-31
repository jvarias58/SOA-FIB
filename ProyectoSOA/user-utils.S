#include <asm.h>

ENTRY(syscall_sysenter)
	push %ecx
	push %edx
	push $SYSENTER_RETURN
	push %ebp
	mov %esp, %ebp
	sysenter
ENTRY(SYSENTER_RETURN)
	pop %ebp
	pop %edx
	pop %edx
	pop %ecx
	ret

/* int write(int fd, char *buffer, int size) */
ENTRY(write)
	pushl %ebp
	movl %esp, %ebp
	pushl %ebx;  // Save EBX, ESI and EDI if modified
	movl $4, %eax
	movl 0x8(%ebp), %ebx;	//fd
	movl 0xC(%ebp), %ecx;	//buffer
	movl 0x10(%ebp), %edx;	//size
	call syscall_sysenter
	popl %ebx
	test %eax, %eax
	js nok	// if (eax < 0) -> 
	popl %ebp
	ret

/* Common code for negative return */
nok:
	neg %eax
	mov %eax, errno
	mov $-1, %eax
	popl %ebp
	ret

/* int gettime() */
ENTRY(gettime)
	pushl %ebp
	movl %esp, %ebp
	movl $10, %eax
	call syscall_sysenter
	popl %ebp
	ret

/* int getpid() */
ENTRY(getpid)
	pushl %ebp
	movl %esp, %ebp
	movl $20, %eax
	call syscall_sysenter
	popl %ebp
	ret

/* int fork() */
ENTRY(fork)
	pushl %ebp
	movl %esp, %ebp
	movl $2, %eax
	call syscall_sysenter
	test %eax, %eax
	js nok	// if (eax < 0) -->
	popl %ebp
	ret

/* void exit() */
ENTRY(exit)
	pushl %ebp
	movl %esp, %ebp
	movl $1, %eax
	call syscall_sysenter
	popl %ebp
	ret

/* int yield() */
ENTRY(yield)
	pushl %ebp
	movl %esp, %ebp
	movl $13, %eax
	call syscall_sysenter
	popl %ebp
	ret

/* int get_stats(int pid, struct stats *st) */
ENTRY(get_stats)
	pushl %ebp
	movl %esp, %ebp
	pushl %ebx;  // Save EBX, ESI and EDI if modified
	movl $35, %eax
	movl 0x8(%ebp), %ebx;	//pid
	movl 0xC(%ebp), %ecx;	//st
	call syscall_sysenter
	popl %ebx
	test %eax, %eax
	js nok	// if (eax < 0) -->
	popl %ebp
	ret


ENTRY(alloc)
	pushl %ebp
        movl %esp, %ebp
        movl $33, %eax
        call syscall_sysenter
        test %eax, %eax
        js nok  // if (eax < 0) -->
        popl %ebp
        ret

ENTRY(dealloc)
        pushl %ebp
        movl %esp, %ebp

	pushl %ebx
	movl 0x8(%ebp), %ebx

        movl $34, %eax
        call syscall_sysenter
        test %eax, %eax
        js nok  // if (eax < 0) -->

	popl %ebx
	popl %ebp
        ret

ENTRY(createthread)
        pushl %ebp
        movl %esp, %ebp
        pushl %ebx;  // Save EBX, ESI and EDI if modified
	pushl %ecx;
        movl 0x8(%ebp), %ebx;   //function
        movl 0xC(%ebp), %ecx;   //param

	lea threadwrapper, %eax
	movl  %eax, %edx //pasar wrapperthread

	movl $25, %eax
        call syscall_sysenter
	popl %ecx
        popl %ebx
        test %eax, %eax
        js nok  // if (eax < 0) -> 
        popl %ebp
        ret

ENTRY(terminatethread)
        pushl %ebp
        movl %esp, %ebp
        movl $26, %eax
        call syscall_sysenter
        test %eax, %eax
        js nok  // if (eax < 0) -->
        popl %ebp
        ret


ENTRY(get_key)
        pushl %ebp
        movl %esp, %ebp
        pushl %ebx;  // Save EBX, ESI and EDI if modified
        movl $28, %eax
        movl 0x8(%ebp), %ebx;   //address

        call syscall_sysenter
        popl %ebx
        test %eax, %eax
        js nok  // if (eax < 0) -> 
        popl %ebp
        ret


ENTRY(dump_screen)
	pushl %ebp
        movl %esp, %ebp
        pushl %ebx;  // Save EBX, ESI and EDI if modified
        movl $29, %eax
        movl 0x8(%ebp), %ebx;   //address

        call syscall_sysenter
        popl %ebx
        test %eax, %eax
        js nok  // if (eax < 0) -> 
        popl %ebp
        ret


ENTRY(sem_init)
	pushl %ebp
        movl %esp, %ebp
        pushl %ebx;
        pushl %ecx;
	movl $15, %eax
        movl 0x8(%ebp), %ebx; //n_sem
	movl 0xC(%ebp), %ecx //value

        call syscall_sysenter
	popl %ecx
        popl %ebx
        test %eax, %eax
        js nok  // if (eax < 0) -> 
        popl %ebp
        ret

ENTRY(sem_wait)
	pushl %ebp
        movl %esp, %ebp
        pushl %ebx;
        movl $16, %eax
        movl 0x8(%ebp), %ebx; //n_sem

        call syscall_sysenter
        popl %ebx
        test %eax, %eax
        js nok  // if (eax < 0) -> 
        popl %ebp
        ret

ENTRY(sem_signal)
	pushl %ebp
        movl %esp, %ebp
        pushl %ebx;
        movl $17, %eax
        movl 0x8(%ebp), %ebx; //n_sem

        call syscall_sysenter
        popl %ebx
        test %eax, %eax
        js nok  // if (eax < 0) -> 
        popl %ebp
        ret

ENTRY(sem_destroy)
	pushl %ebp
        movl %esp, %ebp
        pushl %ebx;
        movl $18, %eax
        movl 0x8(%ebp), %ebx; //n_sem

        call syscall_sysenter
        popl %ebx
        test %eax, %eax
        js nok  // if (eax < 0) -> 
        popl %ebp
        ret

