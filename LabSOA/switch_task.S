#include <asm.h>

ENTRY(task_switch)
        pushl %ebp
        movl %esp, %ebp

        pushl %esi
        pushl %edi
        pushl %ebx

        pushl 8(%ebp) //Passem el punter new com a parametre
        call inner_task_switch

        popl %ebx //Eliminar el punter a new de la pila
        popl %ebx
        popl %edi
        popl %esi

        popl %ebp
        ret

ENTRY(inner_task_switch2)
	push %ebp
	movl %esp, %ebp

	movl 8(%esp), %eax
	movl %esp, (%eax)
	movl 12(%esp), %esp
	popl %ebp
	ret

