TP2: Procesos de usuario
========================

# env_alloc

## ¿Qué identificadores se asignan a los primeros 5 procesos creados? (Usar base hexadecimal.)
Considerando que la lista de environments (envs) es inicializada completamente en 0 en mem_init(), la forma de generar 
un env_id para cada environment nuevo con env_alloc es la siguiente (suponiendo que no se vaya liberando ninguno):

 - Se toma el id del primer env libre (inicializado en env_id = 0).
 - Se genera una variable auxiliar (generation) que esta formada por una suma del id anterior (0) con una constante 
   (0x00001000).
 - A esa variable auxiliar se le realiza una negación de sus primeros 10 bits (generation & 0xfffffc00), 
   de forma que puedan ser usados para identificar de forma unica a cada uno de los NENV (1024).
 - (Si generation da negativa entonces se pisa directamente con el valor de 0x0001000).
 - Se define el env_id finalmente como un OR entre la variable auxiliar (generation) y la diferencia entre el puntero 
   al primer env libre y el primer env de la lista general de environments (es decir, n con n la cantidad de entornos 
   ya pedidos (no libres) con n a lo sumo NENV - 1).

Considerando lo mencionado entonces los primeros 5 procesos tendran los siguientes IDs:

 - 0x00001000
 - 0x00001001
 - 0x00001002
 - 0x00001003
 - 0x00001004

## Supongamos que al arrancar el kernel se lanzan NENV procesos a ejecución. A continuación se destruye el proceso asociado a envs[630] y se lanza un proceso que cada segundo muere y se vuelve a lanzar. ¿Qué identificadores tendrá este proceso en sus sus primeras cinco ejecuciones?

Como ya fueron lanzados todos los procesos, todos tienen el campo env_id inicializado (distinto de 0). Por su parte, al destruir el proceso asociado a envs[630] y relanzar uno nuevo, solo tendremos ese proceso destruido previamente en la lista de procesos libres, por los siguientes lanzamientos se utilizara el mismo struct para representar al proceso.

Para generar el ID del nuevo proceso, se utiliza al env_id del proceso que antiormente asociado a ese struct (explicado mas en detalle en la pregunta anterior).
En este caso, se relanzar el proceso partiendo del env_id 0x00001276. Los siguientes 5 env_id's seran:

 - 0x00002276
 - 0x00003276
 - 0x00004276
 - 0x00005276
 - 0x00006276

Como es posible ver en el comportamiento de los env_id's, los primeros 10 bits indican el offset respecto al inicio del 
vector "envs" (1024 opciones = 10 bits) y los 21 bits mas significativos inidican la cantidad de ejecuciones sobre un 
determinado struct Env.

# env_init_percpu

## ¿Cuántos bytes escribe la función lgdt, y dónde? ¿Qué representan esos bytes?

La función lgdt llamada en env_init_percpu escribe en total 48 bytes en el registro GDTR (Global Descriptor Table 
Register), es decir los que corresponden a los atributos de un struct Pseudodesc que serian el límite (tamaño) de una 
estructura Segdesc correspondiente a una GDT (global descriptor table, que contiene a su vez la base y el limite los 
segmentos del kernel) y la direccion de dicha estructura.


# env_pop_tf

## Dada la secuencia de instrucciones assembly en la función env_pop_tf(), describir qué contiene durante su ejecución:

### - El tope de la pila justo antes popal:

 - La función env_pop_tf() recibe como parametro un struct Trapframe por stack, primero se desapila la dirección del 
   trapframe (esp) y luego justo antes de hacer el popal el stack va a apuntar al primer elemento del struct Trapframe 
   el cual es el primer elemento de su atributo struct PushRegs, es decir el uint32_t "reg_edi".

### - El tope de la pila justo antes iret

 - Luego de ejecutar las instrucciones "popl %%es" y "popl %%ds" (que popean de la pila los elementos del trapframe 
   tf_es (y el padding1 debido a que el popl se trae 32 bits) y tf_ds (y el padding 2) y luego la instruccion 
   "addl $0x8,%%esp" que mueve el tope de la pila para que se saltee los elementos "tf_trapno" y "tf_err" del struct 
   Trapframe, justo antes de ejecutar iret la pila (el registro esp) apunta al elemento tf_eip del struct Trapframe 
   recibido como parametro.

### - El tercer elemento de la pila justo antes de iret

 - Analogamente se tiene que el tercer elemento de la pila justo antes de iret es tf_eflags.

## ¿Cómo determina la CPU (en x86) si hay un cambio de ring (nivel de privilegio)? Ayuda: Responder antes en qué lugar exacto guarda x86 el nivel de privilegio actual. ¿Cuántos bits almacenan ese privilegio?

La CPU guarda el nivel actual de privilegio (CPL) en los dos bits menos significativos del registro CS, por lo que para 
determinar un cambio de ring se compara con el DPL (descriptor privilege level) del segmento accedido. Esta información 
es posible obtenerla mediante la gdt (Estructura Segdesc, global descriptor table).


# gdb_hello

## Se pide arrancar el programa hello.c bajo GDB y mostrar una sesión de GDB con los siguientes pasos:

### 1 y 2 - Poner un breakpoint en env_pop_tf(), continuar la ejecución y mostrar las primeras 5 lineas de info registers:

eax            0x5c000	    376832
ecx            0xf005c000	-268058624
edx            0x21e	    542
ebx            0x10094	    65684
esp            0xf0118fbc	0xf0118fbc


### 3 -Impresion del valor del registro tf:

$1 = (struct Trapframe *) 0xf01c0000


### 4 - Imprimir, con x/Nx tf tantos enteros como haya en el struct Trapframe donde N = sizeof(Trapframe) / sizeof(int).

(gdb) print sizeof(struct Trapframe)/ sizeof(int)
$2 = 17

(gdb) x/17x tf
0xf01c0000:	0x00000000	0x00000000	0x00000000	0x00000000
0xf01c0010:	0x00000000	0x00000000	0x00000000	0x00000000
0xf01c0020:	0x00000023	0x00000023	0x00000000	0x00000000
0xf01c0030:	0x00800020	0x0000001b	0x00000000	0xeebfe000
0xf01c0040:	0x00000023

### 5 y 6 - Comprobar, con x/Nx $sp que los contenidos son los mismos que tf (donde N es el tamaño de tf).

(gdb) disas
Dump of assembler code for function env_pop_tf:
=> 0xf0102fbd <+0>:	push   %ebp
   0xf0102fbe <+1>:	mov    %esp,%ebp
   0xf0102fc0 <+3>:	sub    $0xc,%esp
   0xf0102fc3 <+6>:	mov    0x8(%ebp),%esp
   0xf0102fc6 <+9>:	popa   
   0xf0102fc7 <+10>:	pop    %es
   0xf0102fc8 <+11>:	pop    %ds
   0xf0102fc9 <+12>:	add    $0x8,%esp
   0xf0102fcc <+15>:	iret   
   0xf0102fcd <+16>:	push   $0xf01054d8
   0xf0102fd2 <+21>:	push   $0x1fa
   0xf0102fd7 <+26>:	push   $0xf010547a
   0xf0102fdc <+31>:	call   0xf01000a9 <_panic>
End of assembler dump.

(gdb) si 4
=> 0xf0102fc6 <env_pop_tf+9>:	popa   
0xf0102fc6	497		asm volatile("\tmovl %0,%%esp\n"

(gdb) x/17x $sp
0xf01c0000:	0x00000000	0x00000000	0x00000000	0x00000000
0xf01c0010:	0x00000000	0x00000000	0x00000000	0x00000000
0xf01c0020:	0x00000023	0x00000023	0x00000000	0x00000000
0xf01c0030:	0x00800020	0x0000001b	0x00000000	0xeebfe000
0xf01c0040:	0x00000023

Como puede apreciarse todos los valores son los mismos que los de tf.

### 7 - Describir cada uno de los valores. Para los valores no nulos, se debe indicar dónde se configuró inicialmente el valor, y qué representa.

Los valores indican cada uno de los elementos del struct trapframe, es decir: 

uint32_t reg_edi  = 0x00000000;<br/>
uint32_t reg_esi  = 0x00000000;<br/>
uint32_t reg_ebp  = 0x00000000;<br/>
uint32_t reg_oesp = 0x00000000;<br/>
uint32_t reg_ebx  = 0x00000000;<br/>
uint32_t reg_edx  = 0x00000000;<br/>
uint32_t reg_ecx  = 0x00000000;<br/>
uint32_t reg_eax  = 0x00000000;<br/>
##### uint16_t tf_es    = 0x0023;<br/>
Su valor es 0x23 ya que fue seteado en env_alloc() con GD_UD | 3 (donde GD_UD es 0x20, representa el global descriptor 
user data y el 3 es para indicar el modo usuario (ring 3)).<br/>

uint16_t tf_padding1 = 0x0000;<br/>
##### uint16_t tf_ds    = 0x0023;<br/>
Su valor es 0x23 ya que fue seteado en env_alloc() con GD_UD | 3 (GD_UD es 0x20).<br/>

uint16_t tf_padding2 = 0x0000;<br/>
uint32_t tf_trapno = 0x00000000;<br/>
uint32_t tf_err    = 0x00000000;<br/>
##### uintptr_t tf_eip   = 0x00800020;<br/>
Su valor es distinto de 0 ya que es el valor del entry point del elf (_start), que fue configurado en load_icode().<br/>

##### uint16_t tf_cs     = 0x001b;<br/>
Su valor es 0x1b ya que fue seteado en env_alloc() con GD_UT | 3 (donde GD_UT es 0x18, representa el global descriptor 
user text y el 3 es para indicar el modo usuario (ring 3)).<br/>

uint16_t tf_padding3 = 0x0000;<br/>
uint32_t tf_eflags = 0x00000000;<br/>
##### uintptr_t tf_esp   = 0xeebfe000;<br/>
Su valor es 0xeebfe000 ya que fue seteado en env_alloc() con USTACKTOP (definido en memlayout.h como 0xeebfe000).<br/>

##### uint16_t tf_ss     = 0x0023;<br/>
Su valor es 0x23 ya que fue seteado en env_alloc() con GD_UD | 3 (GD_UD es 0x20).<br/>

uint16_t tf_padding4 = 0x0000;<br/>


### 8 - Continuar hasta la instrucción iret, sin llegar a ejecutarla. Mostrar en este punto, de nuevo, las cinco primeras líneas de info registers en el monitor de QEMU. Explicar los cambios producidos.

(gdb) disas
Dump of assembler code for function env_pop_tf:
   0xf0102fbd <+0>:	push   %ebp
   0xf0102fbe <+1>:	mov    %esp,%ebp
   0xf0102fc0 <+3>:	sub    $0xc,%esp
   0xf0102fc3 <+6>:	mov    0x8(%ebp),%esp
=> 0xf0102fc6 <+9>:	popa   
   0xf0102fc7 <+10>:	pop    %es
   0xf0102fc8 <+11>:	pop    %ds
   0xf0102fc9 <+12>:	add    $0x8,%esp
   0xf0102fcc <+15>:	iret   
   0xf0102fcd <+16>:	push   $0xf01054d8
   0xf0102fd2 <+21>:	push   $0x1fa
   0xf0102fd7 <+26>:	push   $0xf010547a
   0xf0102fdc <+31>:	call   0xf01000a9 <_panic>
End of assembler dump.

(gdb) si 4
=> 0xf0102fcc <env_pop_tf+15>:	iret   
0xf0102fcc	497		asm volatile("\tmovl %0,%%esp\n"

(gdb) info registers
eax            0x0	        0
ecx            0x0	        0
edx            0x0	        0
ebx            0x0	        0
esp            0xf01c0030	0xf01c0030

Los cambios en los registros eax, ecx, edx y ebx a 0x0 se deben a que en la instruccion popa se tomaron los valores
de dichos registros guardados en el struct Trapframe, los cuales como fueron vistos en el punto anterior estaban
seteados en 0. 
En cambio el valor del %esp es 0xf01c0030 ya que fue modificado justo en la instruccion anterior
a popa (es decir mov 0x8(%ebp),%esp) y el valor representa la direccion donde está guardado el struct Trapframe de forma
que al ir popeando del stack principal se popeen los atributos de dicho Trapframe. 


### 9 - Ejecutar la instrucción iret. En ese momento se ha realizado el cambio de contexto y los símbolos del kernel ya no son válidos.


(gdb) si 1
=> 0x800020:	cmp    $0xeebfe000,%esp
0x00800020 in ?? ()

* imprimir el valor del contador de programa con p $pc o p $eip

(gdb) p $pc
$2 = (void (*)()) 0x800020

* luego de cargar los símbolos de hello

(gdb) add-symbol-file obj/user/hello 0x800020
add symbol table from file "obj/user/hello" at
	.text_addr = 0x800020
(y or n) y
Leyendo símbolos desde obj/user/hello...hecho.


* volver a imprimir el contador de programa

(gdb) p $pc
$3 = (void (*)()) 0x800020 <_start>

* Mostrar una última vez la salida de info registers en QEMU, y explicar los cambios producidos.

(gdb) info registers
eax            0x0	0
ecx            0x0	0
edx            0x0	0
ebx            0x0	0
esp            0xeebfe000	0xeebfe000
ebp            0x0	0x0
esi            0x0	0
edi            0x0	0
eip            0x800020	0x800020 <_start>
eflags         0x2	[ ]
cs             0x1b	27
ss             0x23	35
ds             0x23	35
es             0x23	35
fs             0x23	35
gs             0x23	35

Los cambios en los registros se deben a que fueron modificados en la funcion env_pop_tf y corresponden a los valores
de los atributos del trapframe que recibió esa funcion.  


### 10 - Poner un breakpoint temporal (tbreak, se aplica una sola vez) en la función syscall() y explicar qué ocurre justo tras ejecutar la instrucción int $0x30. Usar, de ser necesario, el monitor de QEMU.

(gdb) tbreak syscall
Punto de interrupción temporal 2 at 0x8009ea: syscall. (2 locations)

(gdb) disas
Dump of assembler code for function _start:
=> 0x00800020 <+0>:	cmp    $0xeebfe000,%esp
   0x00800026 <+6>:	jne    0x80002c <args_exist>
   0x00800028 <+8>:	push   $0x0
   0x0080002a <+10>:	push   $0x0
End of assembler dump.

(gdb) c
Continuando.
=> 0x8009ea <syscall+17>:	mov    0x8(%ebp),%ecx

Temporary breakpoint 2, syscall (num=0, check=-289415544, a1=4005551752, 
    a2=13, a3=0, a4=0, a5=0) at lib/syscall.c:23
23		asm volatile("int %1\n"

(gdb) disas
Dump of assembler code for function syscall:
   0x008009d9 <+0>:	push   %ebp
   0x008009da <+1>:	mov    %esp,%ebp
   0x008009dc <+3>:	push   %edi
   0x008009dd <+4>:	push   %esi
   0x008009de <+5>:	push   %ebx
   0x008009df <+6>:	sub    $0x1c,%esp
   0x008009e2 <+9>:	mov    %eax,-0x20(%ebp)
   0x008009e5 <+12>:	mov    %edx,-0x1c(%ebp)
   0x008009e8 <+15>:	mov    %ecx,%edx
=> 0x008009ea <+17>:	mov    0x8(%ebp),%ecx
   0x008009ed <+20>:	mov    0xc(%ebp),%ebx
   0x008009f0 <+23>:	mov    0x10(%ebp),%edi
   0x008009f3 <+26>:	mov    0x14(%ebp),%esi
   0x008009f6 <+29>:	int    $0x30
   0x008009f8 <+31>:	cmpl   $0x0,-0x1c(%ebp)
   0x008009fc <+35>:	je     0x800a02 <syscall+41>
   0x008009fe <+37>:	test   %eax,%eax
   0x00800a00 <+39>:	jg     0x800a0a <syscall+49>
   0x00800a02 <+41>:	lea    -0xc(%ebp),%esp
   0x00800a05 <+44>:	pop    %ebx
   0x00800a06 <+45>:	pop    %esi
   0x00800a07 <+46>:	pop    %edi
---Type <return> to continue, or q <return> to quit---q
Quit

(gdb) si 5
Se asume que la arquitectura objetivo es i8086
[f000:e05b]    0xfe05b:	cmpw   $0xffc8,%cs:(%esi)
0x0000e05b in ?? ()

(gdb) disas
El contador de programa no contiene funciones para el marco seleccionado.


Lo que sucede es que como todavía no esta implementado el handler de la interrupcion syscall 
(TRAPHANDLER_NOEC(trap_48, T_SYSCALL)) el sistema loopea en el booteo si se continua la ejecución con gdb y se 
detiene cada vez en el breakpoint seteado previamente en env_pop_tf.


# kern_idt


## ¿Cómo decidir si usar TRAPHANDLER o TRAPHANDLER_NOEC? ¿Qué pasaría si se usara solamente la primera?

Depende de si la interrupción devuelve un codigo de error, en caso de que devuelva un código de error este es pusheado 
por la cpu al stack, pero en el el caso de que no pushee codigo de error se debe usar TRAPHANDLER_NOEC que se encarga 
de pushear un 0 extra para rellenar la sección del struct trapframe correspondiente al codigo de error. Si se usara 
siempre la primera de las dos macros habría que tener cuidado de rellenar con el 0 el stack en el lugar del atributo 
"tf_err" para no desfasar el acceso a los registros (atributos del struct Trapframe).

## ¿Qué cambia, en la invocación de handlers, el segundo parámetro (istrap) de la macro SETGATE? ¿Por qué se elegiría un comportamiento u otro durante un syscall?

Ese parámetro indica si es posible o no el anidado de interrupciones. Uno de los principales motivos por los que 
conviene no permitir las interrupciones anidadas durante una syscall es la simplicidad en el código, pero el no 
permitir que se aniden las interrupciones puede causar que se pierdan algunas syscalls (o que se retrasen aún mas).

## Leer user/softint.c y ejecutarlo con make run-softint-nox. ¿Qué excepción se genera? Si es diferente a la que invoca el programa… ¿cuál es el mecanismo por el que ocurrió esto, y por qué motivos?

Al ejecutar el programa se ve que genera una excepcion "General protection", es decir que no ejecutó la que intentaría 
invocar el programa que es "Page Fault". Esto se debe a que el privilegio (DPL) necesario para lanzar esa interrupción 
es 0 y como se lanza en modo usuario (nivel de privilegio 3) el sistema detecta esta falta de permiso y lanza la 
excepcion de "General protection".


# user_evilhello

## ¿En qué se diferencia el código de la versión en evilhello.c del código de la página https://fisop.github.io/7508/jos/tp2/#tarea-user_evilhello ? ¿En qué cambia el comportamiento durante la ejecución? ¿Por qué? ¿Cuál es el mecanismo?

La diferencia es que en el código de evilhello.c no se lanza una excepción debido a que la syscall puts no verifica que 
la dirección virtual sea válida y además la syscall, al ejecutarse en modo kernel, puede acceder a direcciones de 
memoria del kernel (Para que esto no suceda, luego se implementa una verificación en la tarea "user_mem_check"). En 
cambio en el código de la página ocurre una excepcion de tipo "Page Fault". Esto sucede ya que se trata de acceder a 
una dirección del kernel (en la desrreferencia "*entry;", donde entry es un puntero a una dirección virtual del kernel) 
directamente desde el espacio de usuario, que no la tiene mapeada en su page directory.

