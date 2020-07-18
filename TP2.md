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

## ¿Cuántos bytes escribe la función lgdt, y dónde? ¿Qué representan esos bytes?

La funcion lgdt llamada en env_init_percpu escribe en total 48 bytes en el registro GDTR (Global Descriptor Table Register), es decir los que corresponden a los atributos de un struct Pseudodesc que serian el límite (tamaño) de una estructura Segdesc correspondiente a una GDT (global descriptor table, que contiene a su vez la base y el limite los segmentos del kernel) y la direccion de dicha estructura.


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

La CPU guarda el nivel actual de privilegio (CPL) en los dos bits menos significativos del registro CS, por lo que para determinar un cambio de ring se compara con el DPL (descriptor privilege level) del segmento accedido. Esta información es posible obtenerla mediante la gdt (Estructura Segdesc, global descriptor table).


# gdb_hello
---------

## Primeras 5 lineas de info registers:

eax            0x5c000             376832
ecx            0xf005c000          -268058624
edx            0x21e               542
ebx            0x10094             65684
esp            0xf011afbc          0xf011afbc

## Impresion del valor del registro tf:

$1 = (struct Trapframe *) 0xf01c9000

## Imprimir, con x/Nx tf tantos enteros como haya en el struct Trapframe donde N = sizeof(Trapframe) / sizeof(int).

(gdb) print sizeof(struct Trapframe)/ sizeof(int)
$2 = 17

(gdb) x/17x tf
0xf01c9000:	0x00000000	0x00000000	0x00000000	0x00000000
0xf01c9010:	0x00000000	0x00000000	0x00000000	0x00000000
0xf01c9020:	0x00000023	0x00000023	0x00000000	0x00000000
0xf01c9030:	0x00800020	0x0000001b	0x00000000	0xeebfe000
0xf01c9040:	0x00000023

## Comprobar, con x/Nx $sp que los contenidos son los mismos que tf (donde N es el tamaño de tf).

(gdb) disas
Dump of assembler code for function env_pop_tf:
=> 0xf0103091 <+0>:	endbr32
   0xf0103095 <+4>:	push   %ebp
   0xf0103096 <+5>:	mov    %esp,%ebp
   0xf0103098 <+7>:	sub    $0xc,%esp
   0xf010309b <+10>:	mov    0x8(%ebp),%esp
   0xf010309e <+13>:	popa   
   0xf010309f <+14>:	pop    %es
   0xf01030a0 <+15>:	pop    %ds
   0xf01030a1 <+16>:	add    $0x8,%esp
   0xf01030a4 <+19>:	iret   
   0xf01030a5 <+20>:	push   $0xf0105b38
   0xf01030aa <+25>:	push   $0x1fa
   0xf01030af <+30>:	push   $0xf0105ada
   0xf01030b4 <+35>:	call   0xf01000ad <_panic>
End of assembler dump.

(gdb) si 5
=> 0xf010309e <env_pop_tf+13>:	popa   
0xf010309e in env_pop_tf (tf=0x0) at kern/env.c:497
497		asm volatile("\tmovl %0,%%esp\n"

(gdb) x/17x $sp
0xf01c9000:	0x00000000	0x00000000	0x00000000	0x00000000
0xf01c9010:	0x00000000	0x00000000	0x00000000	0x00000000
0xf01c9020:	0x00000023	0x00000023	0x00000000	0x00000000
0xf01c9030:	0x00800020	0x0000001b	0x00000000	0xeebfe000
0xf01c9040:	0x00000023

Como puede apreciarse todos los valores son los mismos que los de tf.

## Describir cada uno de los valores. Para los valores no nulos, se debe indicar dónde se configuró inicialmente el valor, y qué representa.

Los valores indican cada uno de los elementos del struct trapframe, es decir: 

uint32_t reg_edi  = 0x00000000;
uint32_t reg_esi  = 0x00000000;
uint32_t reg_ebp  = 0x00000000;
uint32_t reg_oesp = 0x00000000;		/* Useless */
uint32_t reg_ebx  = 0x00000000;
uint32_t reg_edx  = 0x00000000;
uint32_t reg_ecx  = 0x00000000;
uint32_t reg_eax  = 0x00000000;

uint16_t tf_es    = 0x00000023;     Direccion del extra segment
uint16_t tf_padding1 (16 bits mas altos del anterior);

uint16_t tf_ds    = 0x00000023;     Direccion del data segment
uint16_t tf_padding2 (16 bits mas altos del anterior);
uint32_t tf_trapno = 0x00000000;
/* below here defined by x86 hardware */
uint32_t tf_err    = 0x00000000;
uintptr_t tf_eip   = 0x00800020;    Instruction pointer
uint16_t tf_cs     = 0x0000001b;    Code segment
uint16_t tf_padding3 (16 bits mas altos del anterior);
uint32_t tf_eflags = 0x00000000;
/* below here only when crossing rings, such as from user to kernel */
uintptr_t tf_esp   = 0xeebfe000;    Stack pointer del proceso del user que estaba antes de llamar a la interrupcion
uint16_t tf_ss     = 0x00000023;    Stack segment del proc del user.
uint16_t tf_padding4 (16 bits mas altos del anterior);


HACERLA



## Continuar hasta la instrucción iret, sin llegar a ejecutarla. Mostrar en este punto, de nuevo, las cinco primeras líneas de info registers en el monitor de QEMU. Explicar los cambios producidos.

(gdb) info registers
eax            0x0                 0
ecx            0x0                 0
edx            0x0                 0
ebx            0x0                 0
esp            0xf01c9030          0xf01c9030


## Ejecutar la instrucción iret. En ese momento se ha realizado el cambio de contexto y los símbolos del kernel ya no son válidos.

* imprimir el valor del contador de programa con p $pc o p $eip

(gdb) p $pc
$1 = (void (*)()) 0x800020

* luego de cargar los símbolos de hello, volver a imprimir el contador de programa

(gdb) p $pc
$2 = (void (*)()) 0x800020 <_start>

* Mostrar una última vez la salida de info registers en QEMU, y explicar los cambios producidos.

(gdb) info registers
eax            0x0                 0
ecx            0x0                 0
edx            0x0                 0
ebx            0x0                 0
esp            0xeebfe000          0xeebfe000



EXPLICAR CAMBIOS PRODUCIDOS



## Poner un breakpoint temporal (tbreak, se aplica una sola vez) en la función syscall() y explicar qué ocurre justo tras ejecutar la instrucción int $0x30. Usar, de ser necesario, el monitor de QEMU.

se llama al handler:
TRAPHANDLER_NOEC(trap_48, T_SYSCALL)






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
