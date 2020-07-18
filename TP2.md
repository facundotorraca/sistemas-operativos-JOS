TP2: Procesos de usuario
========================

# env_alloc
---------

## ¿Qué identificadores se asignan a los primeros 5 procesos creados? (Usar base hexadecimal.)
Considerando que la lista de environments (envs) es inicializada completamente en 0 en mem_init(), la forma de generar un env_id para cada environment nuevo con env_alloc es la siguiente (suponiendo que no se vaya liberando ninguno):
 - Se toma el id del primer env libre (inicializado en env_id = 0).
 - Se genera una variable auxiliar (generation) que esta formada por una suma del id anterior (0) con una constante (0x00001000).
 - A esa variable auxiliar se le realiza una negacion de sus primeros 10 bits (generation & 0xfffffc00), de forma que puedan ser usados para identificar de forma unica a cada uno de los NENV (1024).
 - (Si generation da negativa entonces se pisa directamente con el valor de 0x0001000)
 - Se define el env_id finalmente como un OR entre la variable auxiliar (generation) y la diferencia entre el puntero al primer env libre y el primer env de la lista general de environments (es decir, n con n la cantidad de entornos ya pedidos (no libres) con n a lo sumo NENV - 1).

Considerando lo mencionado entonces los primeros 5 procesos tendran los siguientes IDs:

 - 0x 0000 1000
 - 0x 0000 1001
 - 0x 0000 1002
 - 0x 0000 1003
 - 0x 0000 1004

## Supongamos que al arrancar el kernel se lanzan NENV procesos a ejecución. A continuación se destruye el proceso asociado a envs[630] y se lanza un proceso que cada segundo muere y se vuelve a lanzar. ¿Qué identificadores tendrá este proceso en sus sus primeras cinco ejecuciones?

Como ya fueron lanzados todos los procesos, todos tienen el campo env_id inicializado (distinto de 0). Por su parte, al destruir el proceso asociado a envs[630] y relanzar uno nuevo, solo tendremos ese proceso destruido previamente en la lista de procesos libres, por los siguientes lanzamientos se utilizara el mismo struct para representar al proceso.

Para generar el ID del nuevo proceso, se utiliza al env_id del proceso que antiormente asociado a ese struct (explicado mas en detalle en la pregunta anteior).
En este caso, se relanzar el proceso partiendo del env_id 0x 0000 1276. Los siguientes 5 env_id's seran:

0x 0000 2276
0x 0000 3276
0x 0000 4276
0x 0000 5276
0x 0000 6276

Como es posible ver en el comportamiento de los env_id's, los primeros 10 bits indican el offset respecto al inicio del vector "envs" (1024 opciones = 10 bits)
y los 21 bits mas significatios inidican la cantidad de ejecuciones sobre un determinado struct Env.

# env_init_percpu
---------------



########### HACERLA!





# env_pop_tf
----------

## Dada la secuencia de instrucciones assembly en la función env_pop_tf(), describir qué contiene durante su ejecución:

### - el tope de la pila justo antes popal:

 - La función env_pop_tf() recibe como parametro un struct Trapframe por stack, primero se desapila la direccion del trapframe (esp) y luego justo antes de hacer el popal el stack va a apuntar al primer elemento del struct Trapframe el cual es el primer elemento de su atributo struct PushRegs, es decir el uint32_t "reg_edi".

### - el tope de la pila justo antes iret

 - Luego de ejecutar las instrucciones "popl %%es" y "popl %%ds" (que popean de la pila los elementos del trapframe tf_es (y el padding1 debido a que el popl se trae 32 bits) y tf_ds (y el padding 2) y luego la instruccion "addl $0x8,%%esp" que mueve el tope de la pila para que se saltee los elementos "tf_trapno" y "tf_err" del struct Trapframe, justo antes de ejecutar iret la pila (el registro esp) apunta al elemento tf_eip del struct Trapframe recibido como parametro.

### - el tercer elemento de la pila justo antes de iret

 - Analogamente se tiene que el tercer elemento de la pila justo antes de iret es tf_eflags.

## ¿Cómo determina la CPU (en x86) si hay un cambio de ring (nivel de privilegio)? Ayuda: Responder antes en qué lugar exacto guarda x86 el nivel de privilegio actual. ¿Cuántos bits almacenan ese privilegio?

La CPU guarda el nivel actual de privilegio (CPL) en los ultimos dos bits del registro CS, por lo que para determinar un cambio de ring se analizan...



######### TODO: SEGUIRLA!




# gdb_hello
---------



########### HACERLA!




# kern_idt
---------

## ¿Cómo decidir si usar TRAPHANDLER o TRAPHANDLER_NOEC? ¿Qué pasaría si se usara solamente la primera?

Depende de si la interrupción devuelve un codigo de error, en caso de que devuelva un código de error este es pusheado por la cpu al stack, pero en el el caso de que no pushee codigo de error se debe usar TRAPHANDLER_NOEC que se encarga de pushear un 0 extra para rellenar la seccion del struct trapframe correspondiente al codigo de error. Si se usara siempre la primera de las dos macros habria que tener cuidado de rellenar con el 0 el stack en el lugar del atributo "tf_err" para no desfasar el acceso a los registros (atributos del struct Trapframe).

## ¿Qué cambia, en la invocación de handlers, el segundo parámetro (istrap) de la macro SETGATE? ¿Por qué se elegiría un comportamiento u otro durante un syscall?

Ese parámetro indica si es posible o no el anidado de interrupciones. Uno de los principales motivos por los que conviene no permitir las interrupciones anidadas durante una syscall es la simplicidad en el código, pero el no permitir que se aniden las interrupciones puede causar que se pierdan algunas syscalls (o que se retrasen aún mas).

## Leer user/softint.c y ejecutarlo con make run-softint-nox. ¿Qué excepción se genera? Si es diferente a la que invoca el programa… ¿cuál es el mecanismo por el que ocurrió esto, y por qué motivos?

Al ejecutar el programa se ve que genera una excepcion "General protection", es decir que no ejecutó la que intentaría invocar el programa que es "Page Fault". Esto se debe a que el privilegio (DPL) necesario para lanzar esa interrupción es 0 y como se lanza en modo usuario (nivel de privilegio 3) el sistema detecta esta falta de permiso y lanza la excepcion de "General protection".


# user_evilhello
-----------

## ¿En qué se diferencia el código de la versión en evilhello.c del código de la página https://fisop.github.io/7508/jos/tp2/#tarea-user_evilhello ? ¿En qué cambia el comportamiento durante la ejecución? ¿Por qué? ¿Cuál es el mecanismo?

La diferencia es que en el código de evilhello.c no se lanza una excepción debido a que la syscall puts no verifica que la dirección virtual sea válida y además la syscall, al 
ejecutarse en modo kernel, puede acceder a direcciones de memoria del kernel (Para que esto no suceda, luego se implementa una verificación en la tarea "user_mem_check"). En cambio en el código de la página ocurre una excepcion de tipo "Page Fault". Esto sucede ya que se trata de acceder a una dirección del kernel (en la desrreferencia "*entry;", donde entry es un puntero a una dirección virtual del kernel) directamente desde el espacio de usuario, que no la tiene mapeada en su page directory.








...
