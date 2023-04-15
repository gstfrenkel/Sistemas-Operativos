#include <inc/lib.h>

void
umain(int argc, char **argv)
{
	cprintf("Nuevo proceso para ver la prioridad\n");
	if(sys_priority() == 0)
		cprintf("La prioridad inicial es de: 0\n");
}
