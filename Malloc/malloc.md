# TP: malloc

Nuestra implementacion de malloc es la siguiente:
Primero inicializamos un bloque segun el size que se nos da:
Por ejemplo si nos piden 18 kB tendriamos que primero inicializar un bloque Mediano.

header_1 | size + size_no usado

y despues de hacer el split con el size no usado:

    header_1 | size | header_2 | size_no_usado

aca, a traves del struct :

```
struct region {
   bool free;
   size_t size;
   struct region *next;
};
```

tenemos una referencia al siguiente de nuestra lista de regiones, por lo que header_1->next estaria apuntando al header_2, y consecuententement header_2 estaria apuntando a NULL ya que no hay otro header. Al agregar un header mas, pasaria lo mismo, pero el header_2 estaria apuntando al header_3 y asi sucesivamente.
A la hora de hacer free, nos vimos obligados a cambiar esta estructura de region, ya que pensabamos que sería muy ineficiente tener que recorrer todo el bloque para poder unir dos regiones contiguas libres. Por lo que nos quedo una lista doblemente enlazada sobre un unico bloque.

    struct region {
    	bool free;
        size_t size;
       	struct region *next;
       	struct region *prev;
    };

Por lo que en el ejemplo anterior, header_1 no tendria prev pero tendria siguiente, y header_3 no tendria siguiente pero tendria anterior.

A la hora de agregar bloque, nos dimos cuenta que no podriamos saber cuando se entraba a un bloque nuevo ya que no habria forma de guardar un puntero a ese nuevo bloque. Entonces pensamos en hacer un lista enlazada sobre los bloques y una lista doblemente enlazada por dentro de cada bloque, por ejemplo, podria llegar a ser:

    header_1 |   completo  | header_2 | completo| -> Header_3 | size | Header_4 | espacio no usado

o 

    header_1 |   completo  | header_2 | completo | header_3 | vacio < size a guardar| -> Header_3 | size | Header_4 | espacio no usado



el bloque 1 conteniendo header_1 y header_2 tendria una implementacion de listas doblemente enlazadas sobre sus regiones, como tambien el bloque 2. Pero entre el bloque 1 y el bloque 2 solo serian simplemente enlazadas, ya que header_2 tendria un next apuntando a header_3 pero header_3 no tendria un prev apuntando a header_2. Al hacer esto, nos dimos cuenta que para hacer lo mas eficientemente posibles deberiamos de tener una estructura indicando el inicio del primer bloque y el inicio del bloque final, como asi el total de memoria disponible, ya que si de por si con regiones libres no llegamos al tamaño deseado, directamente crear un nuevo bloque.
    
    struct region_information {
       struct region *first;
       struct region *last;
       size_t memoria_disponible;
    };

En este caso, el header 1 sería apuntado por el first y header 3 sería apuntado por el last.
Ahora, cuando hacemos un free completo de un bloque, nos estariamos fijando si la primer region del bloque no tiene prev, y si el proximo es null o si el proximo de ese no tiene prev, en ese caso estariamos en la condicion de eliminar todo un bloque con mun map. En este caso, nos tenemos que guardar un puntero al anterior de esa primera region ya que como dijimos anteriormente, lo implementamos como una lista enlazada entre bloques. Para este momento, nos dimos cuenta que no seria la forma mas eficiente la de recorrer todos los bloques de regiones hasta encontrar al que queremos eliminar. 

Al pensar posibles soluciones, se nos ocurrio tener un arreglo que apunte a la ultima region de cada bloque, y utilizariamos un indice para saber en donde estamos parados, reduciendo asi el tamaño de la lista de regiones y siendo mas eficientes. Pero, al hacerlo asi, estariamos aumentando la memoria usada en el stack, por lo que preferimos usar la forma menos eficiente a la hora de llamar a eliminar un bloque entero.

