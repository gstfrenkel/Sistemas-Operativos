# Lab: shell

### Búsqueda en $PATH

####¿Cuáles son las diferencias entre la syscall execve(2) y la familia de wrappers proporcionados por la librería estándar de C (libc) exec(3)?
Los miembros de la familia de syscalls de exec son todos de la forma *execXX* donde XX representan caracteres variables según la syscall. Aquellos miembros que contengan una *'v'*, como por ejemplo execve, reciben como parámetro el vector *argv* del programa a ejecutar. Los que contengan una *'l'*, como execlp, reciben los argumentos del nuevo programa como un argumento a la syscall en sí, a través del vector de largo variable *arg*. A su vez, los que posean una *'p'*, como execvp, realizan la búsqueda del programa a ejecutar a través de la variable de entorno PATH (siempre y cuando no empiece con *'/'*). Por último, los que contengan una *'e'*, como execle, permite pasar por parámetro el entorno que tendrá el nuevo proceso. En cualquier otro caso, este heredará el entorno del proceso original.

####¿Puede la llamada a exec(3) fallar? ¿Cómo se comporta la implementación de la shell en ese caso?
De acuerdo a la documentación, exec(3) puede fallar y retornará un *-1* en tal caso. De suceder, la implementación de shell propuesta imprimirá un mensaje de error por pantalla informando sobre el problema y abortará la ejecución del programa con un código de error de *2*. Adicionalmente, cualquier memoria reservada o file descriptor abierto por dicho proceso será liberada y cerrado respectivamente.

---

### Comandos built-in

####¿Entre cd y pwd, alguno de los dos se podría implementar sin necesidad de ser built-in? ¿Por qué? ¿Si la respuesta es sí, cuál es el motivo, entonces, de hacerlo como built-in? (para esta última pregunta pensar en los built-in como true y false).
El comando *cd* cambia la ruta sobre la cual está "parada" la shell a través de su entorno. De implementarlo como un proceso, se estaría cambiando el entorno del proceso hijo, por lo que la shell nunca se vería afectada por la ejecución del comando. En cambio, el comando *pwd* no altera el entorno de la shell, sino que simplemente accede a este para obtener información sobre la ruta actual. Es por esto que es totalmente posible implementar este comando sin que sea un built-in. La razón por la que sí lo es una shell como bash, es porque los built-ins ahorran tener que ejecutar procesos adicionales lo cual significaría un gasto considerable de recursos para comandos que se utilizan con mucha frecuencia.

---

### Variables de entorno temporarias

####¿Por qué es necesario hacerlo luego de la llamada a fork(2)?
Ya que sino, se va a estar realizando el cambio en el entorno del proceso padre.

####En algunos de los wrappers de la familia de funciones de exec(3) (las que finalizan con la letra e), se les puede pasar un tercer argumento (o una lista de argumentos dependiendo del caso), con nuevas variables de entorno para la ejecución de ese proceso. Supongamos, entonces, que en vez de utilizar setenv(3) por cada una de las variables, se guardan en un array y se lo coloca en el tercer argumento de una de las funciones de exec(3).
- ####¿El comportamiento resultante es el mismo que en el primer caso? Explicar qué sucede y por qué.
Al pasar como parámetro las variables de entorno que se settearían con setenv(3), lo que se está consiguiendo es que el nuevo proceso tenga acceso solamente a estas nuevas variables de entorno, ignorando las del proceso original, lo cual es similar al comportamiento buscado, pero no exactamente igual.
- ####Describir brevemente (sin implementar) una posible implementación para que el comportamiento sea el mismo.
La forma de solucionar esta problemática es agregar al array de variables de entorno las variables del proceso original y pasar todo el conjunto al wrapper de exec(3). De esta forma, el nuevo proceso heredará las variables de entorno del proceso original, y además tendrá las adicionales (que normalmente agregaríamos con setenv(3)).

---

### Procesos en segundo plano

####Detallar cuál es el mecanismo utilizado para implementar procesos en segundo plano.
En vez de obligar a mi shell a esperar a que el proceso que ejecuta el comando finalice para continuar, se pasa el parámetro de WNOHANG a waitpid(), el cual cambia el comportamiento de este último, haciendo que se devuelva la información sobre el estado del proceso de forma inmediata. Esto lo que permite es que el proceso padre realice el chequeo sobre el proceso hijo sin dejar a este último como zombie a pesar de que el padre no queda en suspensión hasta que termina la ejecución del hijo.

---

### Flujo estándar

####Investigar el significado de 2>&1, explicar cómo funciona su forma general y mostrar qué sucede con la salida de cat out.txt en el ejemplo. Luego repetirlo invertiendo el orden de las redirecciones. ¿Cambió algo?
El número 1 representa el stdout, mientras que el número 2 representa el stderr. La sintaxis 2>&1 redirecciona la salida de stderr hacia stdout. La razón por la que no se utiliza la sintaxis de '2>1' es porque esto significa que la salida de stderr se redireccione a un archivo llamado '1'.

La salida del archivo da como resultado:

````bash
$ cat out.txt
ls: cannot access '/noexiste': No such file or directory
/home:
gst-frenkel
````

---

### Tuberías Múltiples (pipes)

####Investigar qué ocurre con el exit code reportado por la shell si se ejecuta un pipe ¿Cambia en algo? ¿Qué ocurre si, en un pipe, alguno de los comandos falla? Mostrar evidencia (e.g. salidas de terminal) de este comportamiento usando bash. Comparar con la implementación del este lab.
El exit code devuelto por la shell es siempre aquel correspondiente al último comando perteneciente a la secuencia de pipes. En caso de que el último comando devuelva un error, el exit code reportado dependerá del tipo de error que se dio.

Bash:

```bash
ls -l | grep
Usage: grep [OPTION]... PATTERNS [FILE]...
Try 'grep --help' for more information.
```
```bash
aaa | ls -l | grep Doc | wc
zsh: command not found: aaa
      1       9      65
```
```bash
seq 5 | sleep
sleep: missing operand
Try 'sleep --help' for more information.
```

Implementación de shell:

```bash
$ ls -l | grep
Usage: grep [OPTION]... PATTERNS [FILE]...
Try 'grep --help' for more information.
```
```bash
$ aaa | ls -l | grep Doc | wc
Exec falló en exec command: No such file or directory
      1       9      65
```
```bash
$ seq 5 | sleep
sleep: missing operand
Try 'sleep --help' for more information.
```

---

### Pseudo-variables

####Investigar al menos otras tres variables mágicas estándar, y describir su propósito. Incluir un ejemplo de su uso en bash (u otra terminal similar).
$! - Devuelve el PID del último proceso.

```bash
sleep 30&
[1] 45128
echo $!
45128
[1]  + done       sleep 30
```

$$ - Devuelve el PID de la shell actual.

```bash
echo $$
27455

//En otra terminal...

echo $$
43775
```

$0 - Devuelve la ruta de la shell.

```bash
echo $0
/bin/zsh
```



---


