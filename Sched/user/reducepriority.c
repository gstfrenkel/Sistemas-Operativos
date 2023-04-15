#include <inc/lib.h>

void
umain(int argc, char **argv)
{
    if(sys_red_priority(50) == 0)
        cprintf("Reducir la prioridad devuelve: 0\n");
    if(sys_priority() == 50)
        cprintf("Intentar reducir la prioridad a 50 la actualiza correctamente\n");    
}
