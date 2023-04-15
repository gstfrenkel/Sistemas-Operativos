#include <inc/lib.h>
void
umain(int argc, char **argv)
{
    int pfork = fork();
    sys_red_priority(100);

    if (pfork == 0){
        if (sys_priority() == 100)
            cprintf("El proceso hijo de un proceso de prioridad 100 tiene también tiene prioridad 100\n");
    }
    else{
        if (sys_priority() == 100)
            cprintf("El proceso padre mantiene una prioridad de 100 luego del fork\n");
    }
}
