#include <inc/lib.h>

void
umain(int argc, char **argv)
{
    sys_red_priority(100);

    if(sys_red_priority(0) == -1)
        cprintf("Aumentar la prioridad devuelve: -1\n");
    if(sys_priority() == 100)
        cprintf("Intentar aumentar la prioridad no actualiza el valor\n");    
}
