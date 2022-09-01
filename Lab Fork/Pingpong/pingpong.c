#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h> //para random
#include <stdio.h>
#include <time.h>

const int ERROR = -1;
const int CANALES = 2;
const int FORK_HIJO = 0;

//Muestra por pantalla los mensajes correspondientes a los file descriptors abiertos con pipe(2)
void imprimir_mensajes_pipe(int fds_escritura_padre[CANALES], int fds_escritura_hijo[CANALES]){
    printf("Hola, soy PID %d:\n", getpid());
    printf("  - primer pipe me devuelve: [%i, %i]\n", fds_escritura_padre[0], fds_escritura_padre[1]);
    printf("  - segundo pipe me devuelve: [%i, %i]\n\n", fds_escritura_hijo[0], fds_escritura_hijo[1]);
}

//Muestra por pantalla los mensajes que ambos procesos deben imprimir
void imprimir_mensajes_en_comun(int fk){
    printf("Donde fork me devuelve %i:\n", fk);
    printf("  - getpid me devuelve: %i\n", getpid());
    printf("  - getppid me devuelve: %i\n", getppid());
}

//Lleva a cabo las impresiones y las tareas correspondientes al proceso original
void realizar_procesos_padre(int fds_escritura_padre[CANALES], int fds_escritura_hijo[CANALES]){
    int mensaje = rand();
    int recibido = 0;

    printf("  - random me devuelve: %i\n", mensaje);
    printf("  - envío valor %i a través de fd=%i\n\n", mensaje, fds_escritura_padre[1]);

    write(fds_escritura_padre[1], &mensaje, sizeof(mensaje));
    read(fds_escritura_hijo[0], &recibido, sizeof(recibido));

    printf("Hola, de nuevo PID %i\n", getpid());
    printf("  - recibí valor %i vía fd=%i\n", recibido, fds_escritura_hijo[0]);
}

//Lleva a cabo las impresiones y las tareas correspondientes al sub-proceso creado al utilizar fork(2)
void realizar_procesos_hijo(int fds_escritura_padre[CANALES], int fds_escritura_hijo[CANALES]){
    int recibido = 0;

    read(fds_escritura_padre[0], &recibido, sizeof(recibido));

    imprimir_mensajes_en_comun(FORK_HIJO);

    printf("  - recibo valor %i vía fd=%i\n", recibido, fds_escritura_padre[0]);
    printf("  - reenvío valor en fd=%i y termino\n\n", fds_escritura_hijo[1]);

    write(fds_escritura_hijo[1], &recibido, sizeof(recibido));
}

//Cierra todos los file descriptors abiertos
void cerrar_fds(int fds_escritura_padre[CANALES], int fds_escritura_hijo[CANALES]){
    close(fds_escritura_padre[0]);
    close(fds_escritura_padre[1]);
    close(fds_escritura_hijo[0]);
    close(fds_escritura_hijo[1]);
}

int main(){
    srand((unsigned)time(NULL));

    int fds_escritura_padre[CANALES];
    int fds_escritura_hijo[CANALES];
    
    int pipe_escritura_padre = pipe(fds_escritura_padre);

    if(pipe_escritura_padre == ERROR){
        perror("Error al crear pipe de escritura para el padre.");
        return ERROR;
    }

    int pipe_escritura_hijo = pipe(fds_escritura_hijo);

    if(pipe_escritura_hijo == ERROR){
        perror("Error al crear pipe de escritura para el padre.");
        close(fds_escritura_padre[0]);
        close(fds_escritura_padre[1]);
        return ERROR;
    }
    
    imprimir_mensajes_pipe(fds_escritura_padre, fds_escritura_hijo);
    
    int fk = fork();    

    if(fk != FORK_HIJO){
        imprimir_mensajes_en_comun(fk);
        realizar_procesos_padre(fds_escritura_padre, fds_escritura_hijo);
    } else
        realizar_procesos_hijo(fds_escritura_padre, fds_escritura_hijo);

    cerrar_fds(fds_escritura_padre, fds_escritura_hijo);
                                                                                                                                                                                                                                                                                                           
    return 0;
}