# sched.md


##Scheduler
El scheduler elegido como alternativa al Round Robin fue una implementación de Stride. Este scheduler se basa en tres variables las cuales son el número de tickets de cada proceso (representación del consumo de CPU), el stride (inversamente proporcional al número de tickets), y el pass (medida de la prioridad de cada proceso). Con cada ciclo de CPU consumido, se actualiza el número de tickets, por ende se actualiza el stride, y el pass se incrementa en una medida de stride.

El pass o prioridad con la que inicia cada proceso es la máxima (0). Si bien la variable de tickets debería cambiar en tiempo de ejecución en función del uso de CPU, en nuestro caso se mantiene como una constante. A su vez, lo mismo sucede con el stride, ya que es un valor constante que se lo divide por el número de tickets. De poder obtener el uso de CPU de cada proceso, ambas variables cambiarían en tiempo de ejecución, y cómo varía el pass dependería de cada proceso y sería relativo al resto de procesos (si hay muchos procesos de baja prioridad, el uso relativo de CPU disminuye).

Adicionalmente, cada 50 llamados a nuestro scheduler, el pass de cada proceso ready to run se disminuye a la mitad, haciendo de forma de boost. La razón por la que se disminuye a la mitad es para penalizar a aquellos procesos que hayan utilizado demasiada CPU haciendo que necesiten varios boosts para recuperar la prioridad. Además, la prioridad relativa de cada proceso se mantiene ya que un proceso de baja prioridad, al ser aumentada al doble, siempre tendrá menor prioridad que uno que tenía prioridad baja, si bien el salto de prioridades será mayor (menor prioridad = números más grandes).

Cuando un proceso realiza fork, el proceso hijo generado herederá del padre su pass. Esto se realiza sobre la base de que no hay razón para penalizar al proceso hijo por las ejecuciones del padre, pero también ayuda a impedir el cheating. De empezar el pass de cada proceso hijo en 0, un programa puede abusar del fork para utilizar procesos hijos para aumentar la prioridad constantemente.


##Context switch
Las capturas con la información de los registros antes y después del context switch se encuentran en la carpeta "capturas-cs". Allí se muestra:
- Lo devuelto por info registers previo a la llamada a context-switch.
- El stack, también antes de la llamada al context-switch, que contiene la información que luego deberíamos encontrar al llamar a info registers (luego de que se realice el cambio de context)
- Info registers después de todos los pops y antes de retornar con iret.
