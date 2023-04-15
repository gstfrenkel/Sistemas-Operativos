#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "malloc.h"
#include <assert.h>
#include "printfmt.h"
#include <stdbool.h>

#define HELLO "hello from test"
#define TEST_STRING "FISOP malloc is working!"
#define VAR2 "Esta es variable 2"
#define VAR3 "Esta es variable 3"
#define VAR4 "Esta es variable 4"

const size_t MINIMO = 256, BLOQUE_PEQUENIO = 16 * 1024,
             BLOQUE_MEDIANO = 1024 * 1024, BLOQUE_GRANDE = 32 * 1024 * 1024;


/* int
main(void)
{
//	   TESTS CALLOCCCCC
//     //This pointer will hold the
//     //base address of the block created
     int* ptr;
     int n, i;

//     //Get the number of elements for the array
     n = 5;
        printfmt("aca1\n");
//     //Dynamically allocate memory using calloc()
     ptr = (int*)calloc(n, sizeof(int));
         printfmt("aca2\n");
// 	//Get the elements of the array
        for (i = 0; i < n; ++i) {
                printfmt("la direccion %p tiene %i \n",&ptr[i],ptr[i]);
                ptr[i] = i + 1;
        }

// 	//Print the elements of the array
        printf("The elements of the array are: ");
        printfmt("la direccion de ptr es %p \n",ptr);
        for (i = 0; i < n; ++i) {
                printfmt("%i, ", ptr[i]);
        }
        return 0;
 } */
// TEsts Malloc
/* 	 char *var = malloc(3000);
         char *var1 = malloc(5000);
         char *var2 = malloc(8050);
         char *var4 = malloc(2000);
          printfmt("holaaaa \n");
         strcpy(var1,VAR2);
         printfmt("%s\n", var1);
         free(var1);
         char *var3 = malloc(5000);
         strcpy(var, TEST_STRING);
         strcpy(var2,VAR3);
         strcpy(var3,VAR4);
         strcpy(var4,VAR4);

         printfmt("%s\n", var);

         printfmt("%s\n", var2);
         printfmt("%s\n", var3);
         printfmt("%s \n",var4);
         printfmt("muere aca \n");
         free(var3);
         printfmt("va a liberar var 2 \n");
         free(var2);
         free(var);
         free(var4);


        printfmt("YA TERMINO CON PRIMERA ITERACION \n");

         char *ptr = malloc(3000);
         char *ptr1 = malloc(5000);
         char *ptr2 = malloc(8050);
          printfmt("holaaaa \n");
         strcpy(ptr1,VAR2);
         printfmt("%s\n", ptr1);
         free(ptr1);
         char *ptr3 = malloc(5000);
         strcpy(ptr, TEST_STRING);
         strcpy(ptr2,VAR3);
         strcpy(ptr3,VAR4);

         printfmt("%s\n", ptr);

         printfmt("%s\n", ptr2);
         printfmt("%s\n", ptr3);
         printfmt("muere aca \n");
         free(ptr3);
         printfmt("va a liberar ptr 2 \n");
         free(ptr2);
         free(ptr);

         return 0;
 }
 */

// char *var1 = malloc(3000);
// char *var_free = malloc(1000);
// strcpy(var_free,"se va a liberar");
// char *var2 = malloc(4000);
// char* var3 = malloc(8200);
// strcpy(var1,"hola");
// printfmt("%s \n",var_free);
// strcpy(var2,VAR3);
// strcpy(var3,VAR4);
// printfmt("%s \n",var1);
// printfmt("%s \n",var2);
// printfmt("%s \n",var3);
//
// free(var_free);
// char* var4 = realloc(var1,4000);
// printfmt("%s abafdbau \n",var4);
//
// return 0;
//}

static void
prueba_malloc()
{
	printfmt("TEST MALLOC\n");
	printfmt("\nTest1\n");
	char *ptr0 = malloc((size_t)(BLOQUE_GRANDE + 1));
	printfmt("Prueba malloc numero demasiado grande devuelve NULL, %s \n",
	         ptr0 == NULL ? "OK" : "FAIL");

	printfmt("\nTest2\n");
	char *ptr1 = malloc(100);
	printfmt("Prueba malloc con bloque pequeño devuelve puntero no nulo, "
	         "%s\n",
	         ptr1 != NULL ? "OK" : "FAIL");
	free(ptr1);

	printfmt("\nTest3\n");
	char *ptr2 = malloc(BLOQUE_PEQUENIO + 1);
	printfmt("Prueba malloc con bloque mediano devuelve puntero no nulo, "
	         "%s\n",
	         ptr2 != NULL ? "OK" : "FAIL");
	free(ptr2);

	printfmt("\nTest4\n");
	char *ptr3 = malloc(BLOQUE_MEDIANO + 1);
	printfmt("Prueba malloc con bloque grande devuelve puntero no nulo, "
	         "%s\n",
	         ptr3 != NULL ? "OK" : "FAIL");
	free(ptr3);
}

void
test_calloc()
{
	printfmt("\n\nTEST CALLOC\n\n");
	int *ptr1 = (int *) calloc(12, sizeof(int));
	bool correcta_inicializacion_ceros = true;

	for (int i = 0; i < 12; i++) {
		correcta_inicializacion_ceros &= ptr1[i] == 0;
	}
	printfmt("Calloc inicializa en cero, %s\n",
	         correcta_inicializacion_ceros ? "OK" : "FAIL");

	free(ptr1);
}

void
test_realloc()
{
	printfmt("\n\nTEST REALLOC\n\n");
	char *ptr1 = malloc(3000);
	strcpy(ptr1, "hola");
	char *ptr1_aux = realloc(ptr1, 4000);
	printfmt("\nTest1\n");
	printfmt("Realloc con valor mayor al pedido con espacio suficiente a "
	         "derecha mantiene los valores correctamente, %s\n",
	         strcmp(ptr1_aux, "hola") == 0 ? "OK" : "FAIL");
	printfmt("Realloc con valor mayor al pedido ccon espacio suficiente a "
	         "derecha no cambia posición de inicio, %s\n",
	         ptr1 == ptr1_aux ? "OK" : "FAIL");
	free(ptr1_aux);


	printfmt("\nTest2\n");
	char *ptr2 = malloc(3000);
	strcpy(ptr2, "hola");
	char *ptr2_aux = realloc(ptr2, 4000);
	printfmt("Realloc con valor menor al pedido mantiene los valores "
	         "correctamente, %s\n",
	         strcmp(ptr2_aux, "hola") == 0 ? "OK" : "FAIL");
	printfmt("Realloc con valor menor al pedido no cambia posición de "
	         "inicio, %s\n",
	         ptr2 == ptr2_aux ? "OK" : "FAIL");
	free(ptr2_aux);


	printfmt("\nTest3\n");
	char *ptr3 = malloc(3000);
	strcpy(ptr3, "hola");
	char *ptr4 = malloc(1000);
	strcpy(ptr4, "hola");
	char *ptr3_aux = realloc(ptr3, 4000);
	printfmt("Realloc con valor mayor al pedido sin espacio a derecha "
	         "copia los valores correctamente, %s\n",
	         strcmp(ptr3_aux, "hola") == 0 ? "OK" : "FAIL");
	printfmt("Realloc con valor mayor al pedido sin espacio a derecha "
	         "cambia posición de inicio, %s\n",
	         ptr3 != ptr3_aux ? "OK" : "FAIL");
	free(ptr3_aux);
	free(ptr4);

	printfmt("\nTest4\n");
	void *ptr5 = realloc(NULL, 5000);
	printfmt("Realloc con ptr == NULL devuelve un puntero válido, %s\n",
	         ptr5 != NULL ? "OK" : "FAIL");
	printfmt("Se libera la memoria pedida con size == 0, %s\n");
	realloc(ptr5, 0);
}


static void
prueba_free()
{
	printfmt("\n\nTEST DOUBLE FREE\n\n");
	char *ptr1 = malloc(6000);
	free(ptr1);
	free(ptr1);
	printfmt("Double free ignora el segundo free, OK\n");
}


int
main()
{
	prueba_malloc();
	test_calloc();
	test_realloc();
	prueba_free();
	return 0;
}