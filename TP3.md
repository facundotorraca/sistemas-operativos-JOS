# env_return

## al terminar un proceso su función umain() ¿dónde retoma la ejecución el kernel? Describir la secuencia de llamadas desde que termina umain() hasta que el kernel dispone del proceso.

Una vez que sale de umain() se llama a la funcion exit de la libmain, luego de entrar a exit llama a la syscall sys_env_destroy() con el parametro 0, indicando que se liberara el propio proceso que esta corriendo. Luego se llama a la syscall syscall() del kernel, a partir de ese momento ya se esta en el lado del kernel. La syscall syscall del kernel llama a la syscall sys_env_destroy pasandole el parametro 0, y sys_env_destroy llama a env_destroy que que libera al proceso y luego se llama a sched_yield() que finalmente llamara al scheduler para buscar un nuevo proceso que ejecutar.


## ¿en qué cambia la función env_destroy() en este TP, respecto al TP anterior?

En el tp anterior la funcion env_destroy destruía el único environment que corria en el sistema y luego se devolvia al monitor de JOS, en cambio en este TP al haber un scheduler luego de
destruir el environment indicado, o de setearlo como ENV_DYING en caso de que esté siendo ejecutado en otra CPU, si el environment que se quiere destruir es el proceso que se estaba ejecutando en ese momento se va a llamar a sched_yield, permitiendo ejecutar algun otro proceso.


# sys_yield

## mostrar y explicar la salida de make qemu-nox al correr 3 instancias del programa user/yield.c

SMP: CPU 0 found 1 CPU(s)
enabled interrupts: 1 2
[00000000] new env 00001000
[00000000] new env 00001001
[00000000] new env 00001002
Hello, I am environment 00001000.
Hello, I am environment 00001001.
Hello, I am environment 00001002.
Back in environment 00001000, iteration 0.
Back in environment 00001001, iteration 0.
Back in environment 00001002, iteration 0.
Back in environment 00001000, iteration 1.
Back in environment 00001001, iteration 1.
Back in environment 00001002, iteration 1.
Back in environment 00001000, iteration 2.
Back in environment 00001001, iteration 2.
Back in environment 00001002, iteration 2.
Back in environment 00001000, iteration 3.
Back in environment 00001001, iteration 3.
Back in environment 00001002, iteration 3.
Back in environment 00001000, iteration 4.
All done in environment 00001000.
[00001000] exiting gracefully
[00001000] free env 00001000
Back in environment 00001001, iteration 4.
All done in environment 00001001.
[00001001] exiting gracefully
[00001001] free env 00001001
Back in environment 00001002, iteration 4.
All done in environment 00001002.
[00001002] exiting gracefully
[00001002] free env 00001002
No runnable environments in the system!

El programa yield.c consiste en que el proceso manualmente le pida al kernel que haga un cambio de contexto para que avance con el siguiente proceso de su lista de procesos (siguiendo la
mecanica de round robin). Esto va a hacerse 4 veces para luego finalizar la ejecución, y como se lanzan 3 instancias del programa yield.c se va a ver de forma intercalada como
como los procesos van ejecutando cada uno su iteracion, en orden segun el round robin.

# envid2env

## Responder qué ocurre:
### - en JOS, si un proceso llama a sys_env_destroy(0)
Lo que sucede es que se va a destruir el proceso que este siendo ejecutado actualmente.

### - en Linux, si un proceso llama a kill(0, 9)
Lo que sucede es que va a enviar la señal de interrupcion 9 (que indica que se quiere matar a la aplicacion y que esto no sea manejado por el proceso, sino por el scheduler) 
para todos los procesos que pertenezcan al grupo del proceso que llamó a kill.

### - JOS: sys_env_destroy(-1)
Debido a que -1 se toma como el numero hexadecimal 0xFFFFFFFF, se intentara matar al proceso 1023 y solo lo va a lograr en caso de que ese proceso coincida su env_id (que como se vio en el tp anterior, depende de la cantidad de ejecuciones que hayan ocurrido sobre ese env). Por lo tanto en general se va a fallar.

### - Linux: kill(-1, 9)
Lo que sucede es que el proceso que llame a kill va enviar la señal 9 a todos los procesos a los que tenga permiso, sin incluir al proceso 1 ya que es el proceso init.

# dumbfork

## 1. Si, antes de llamar a dumbfork(), el proceso se reserva a sí mismo una página con sys_page_alloc() ¿se propagará una copia al proceso hijo? ¿Por qué?
Si, ya que dumbfork va a copiar todas las todas las paginas que se encuentren mapeadas entre UTEXT (direccion virtual en la que comienza el HEAP y el DATA SEGMENT del proceso) y USTACKTOP
en el directorio de paginas del proceso indicado, por lo que si se reservo justo antes una pagina con page alloc, esta será copiada.

## 2. ¿Se preserva el estado de solo-lectura en las páginas copiadas? Mostrar, con código en espacio de usuario, cómo saber si una dirección de memoria es modificable por el proceso, o no. (Ayuda: usar las variables globales uvpd y/o uvpt.)

No se preserva, ya que todas las copias de paginas generadas se les asignan los mismos permisos (bit de usuario, bit de presencia y bit de escritura permitida). Para saber si una direccion es modificable por el proceso se pueden usar los arreglos auxiliares de solo lectura uvpd (que guarda las direcciones fisicas de las tablas de paginas del proceso, asi como los permisos) y uvpt (que si se le indica el numero de una pagina especifica, a partir de una direccion virtual, permite obtener una entrada de una tabla de pagina especifica, asi como los permisos de la misma). Entonces para ver si la direccion va es modificable por el proceso basta con verificar que: 
	uvpd[PDX(va)] & PTE_P
sea igual a PTE_P y luego que: 
	uvpt[PGNUM(va)] & (PTE_P | PTE_U | PTE_W) 
sea igual a (PTE_P | PTE_U | PTE_W).


## 3. Describir el funcionamiento de la función duppage().

Consiste en:
 - Reservar una pagina vacia en una direccion virtual del proceso hijo, con los permisos (PTE_U | PTE_P | PTE_W)
 - Mapear una direccion virtual auxiliar indicada del proceso padre a la pagina vacia reservada en directorio del hijo
 - Copiar la pagina en la direccion virtual indicada en el directorio del padre a la pagina en la direccion auxiliar, logrando que la direccion virtual del hijo que tambien apuntaba a esa pagina vacia ahora apunte a la copia de la pagina del padre.
 - Desmapear la direccion virtual auxiliar en el directorio del padre, haciendo que el unico vinculo a la pagina nueva sea el de la direccion virtual en el directorio del hijo.

## 4. Supongamos que se añade a duppage() un argumento booleano que indica si la página debe quedar como solo-lectura en el proceso hijo:

### - indicar qué llamada adicional se debería hacer si el booleano es true
Se deberia llamar al final a sys_page_map(dstenv, addr, dstenv, addr, PTE_P|PTE_U), de forma de remapear la pagina a la misma direccion pero quitandole el permiso del PTE_W.

### - describir un algoritmo alternativo que no aumente el número de llamadas al sistema, que debe quedar en 3 (1 × alloc, 1 × map, 1 × unmap).
El codigo puede hacerse de la siguiente manera, de forma de reservar una pagina vacia en una direccion auxiliar del proceso padre y mapear la direccion virtual del hijo como solo lectura a esa pagina vacia del padre, logrando mapear una pagina que en principio tiene permiso de escritura como una de solo lectura a los ojos del proceso hijo:

    int perm = PTE_P | PTE_U;
    if ((uvpd[PDX((uintptr_t)addr)] & PTE_P) == PTE_P && (uvpt[PGNUM((uintptr_t)addr)] & (PTE_P | PTE_U | PTE_W)) == (PTE_P | PTE_U | PTE_W))
        perm |= PTE_W;
    if ((r = sys_page_alloc(0, UTEMP, PTE_P|PTE_U|PTE_W)) < 0)
        panic("sys_page_alloc: %e", r);
    if ((r = sys_page_map(0, UTEMP, dstenv, addr, perm)) < 0)
        panic("sys_page_map: %e", r);
    memmove(UTEMP, addr, PGSIZE);
    if ((r = sys_page_unmap(0, UTEMP)) < 0)
        panic("sys_page_unmap: %e", r);


## 5. ¿Por qué se usa ROUNDDOWN(&addr) para copiar el stack? ¿Qué es addr y por qué, si el stack crece hacia abajo, se usa ROUNDDOWN y no ROUNDUP?
Debido a que las paginas se copian desde la direccion ROUNDDOWN(va, PGSIZE) hasta ROUNDDOWN(va, PGSIZE) + PGSIZE, por lo que si se redondeara para arriba se copiaría.


# multicore init

## 1. ¿Qué código copia, y a dónde, la siguiente línea de la función boot_aps()?
### memmove(code, mpentry_start, mpentry_end - mpentry_start);
Esta linea se encarga de copiar el codigo escrito en mpentry.s que se encuentra entre las direcciones cargadas inicialmente en memoria entre mpentry_start y mpentry_end en la direccion física 0x00007000 (que justamente esta cargada en la variable code como la direccion virtual 0xf0007000).

## 2. ¿Para qué se usa la variable global mpentry_kstack? ¿Qué ocurriría si el espacio para este stack se reservara en el archivo kern/mpentry.S, de manera similar a bootstack en el archivo kern/entry.S?
Se usa para asignar la direccion de inicio de cada stack para el kernel en cada uno de los CPUs, ya que cada CPU necesita tener su propio stack para poder hacer uso del kernel de manera simultanea. 
Si la direccion se mapeara directamente en el codigo de mpentry.s, los NCPU - 1 (los CPUs que no son el CPU 0) tendrian cada todos mapeado su Kstack a la misma dirección.

## 3. Cuando QEMU corre con múltiples CPUs, éstas se muestran en GDB como hilos de ejecución separados. Mostrar una sesión de GDB en la que se muestre cómo va cambiando el valor de la variable global

mpentry_kstack:

$ make qemu-nox-gdb CPUS=4
...

// En otra terminal:
$ make gdb
(gdb) watch mpentry_kstack
Hardware watchpoint 1: mpentry_kstack

(gdb) continue
Continuing.
The target architecture is assumed to be i386
=> 0xf0100186 <boot_aps+110>:	mov    %esi,%ecx

Thread 1 hit Hardware watchpoint 1: mpentry_kstack

Old value = (void *) 0x0
New value = (void *) 0xf0252000 <percpu_kstacks+65536>
boot_aps () at kern/init.c:107
107			lapic_startap(c->cpu_id, PADDR(code));

(gdb) bt
#0  boot_aps () at kern/init.c:107
#1  0xf010023c in i386_init () at kern/init.c:56
#2  0xf01064c6 in ?? ()
#3  0xf0100047 in relocated () at kern/entry.S:89

(gdb) info threads
Id   Target Id                    Frame 
* 1    Thread 1.1 (CPU#0 [running]) boot_aps () at kern/init.c:107
  2    Thread 1.2 (CPU#1 [halted ]) 0x000fd08c in ?? ()
  3    Thread 1.3 (CPU#2 [halted ]) 0x000fd08c in ?? ()
  4    Thread 1.4 (CPU#3 [halted ]) 0x000fd08c in ?? ()

(gdb) continue
Continuing.
=> 0xf0100186 <boot_aps+110>:	mov    %esi,%ecx

Thread 1 hit Hardware watchpoint 1: mpentry_kstack

Old value = (void *) 0xf0252000 <percpu_kstacks+65536>
New value = (void *) 0xf025a000 <percpu_kstacks+98304>
boot_aps () at kern/init.c:107
107			lapic_startap(c->cpu_id, PADDR(code));

(gdb) info threads
Id   Target Id                    Frame 
* 1    Thread 1.1 (CPU#0 [running]) boot_aps () at kern/init.c:107
  2    Thread 1.2 (CPU#1 [running]) 0xf010604c in holding (lock=0x1)
    at kern/spinlock.c:42
  3    Thread 1.3 (CPU#2 [halted ]) 0x000fd08c in ?? ()
  4    Thread 1.4 (CPU#3 [halted ]) 0x000fd08c in ?? ()

(gdb) thread 2
[Switching to thread 2 (Thread 1.2)]
#0  0xf010604c in holding (lock=0x1) at kern/spinlock.c:42
42		return lock->locked && lock->cpu == thiscpu;

(gdb) bt
#0  0xf010604c in holding (lock=0x1) at kern/spinlock.c:42
#1  0xfee00000 in ?? ()
#2  0xf0106096 in spin_lock (lk=0x0) at kern/spinlock.c:64
#3  0x00000000 in ?? ()

(gdb) p cpunum()
$1 = 1

(gdb) thread 1
[Switching to thread 1 (Thread 1.1)]
#0  boot_aps () at kern/init.c:107
107			lapic_startap(c->cpu_id, PADDR(code));

(gdb) p cpunum()
$2 = 0

(gdb) continue
=> 0xf0100186 <boot_aps+110>:	mov    %esi,%ecx

Thread 1 hit Hardware watchpoint 1: mpentry_kstack

Old value = (void *) 0xf025a000 <percpu_kstacks+98304>
New value = (void *) 0xf0262000 <percpu_kstacks+131072>
boot_aps () at kern/init.c:107
107			lapic_startap(c->cpu_id, PADDR(code));


## 4. En el archivo kern/mpentry.S se puede leer:

#####  # We cannot use kern_pgdir yet because we are still
#####  # running at a low EIP.
#####  movl $(RELOC(entry_pgdir)), %eax

### - ¿Qué valor tendrá el registro %eip cuando se ejecute esa línea? (Responder con redondeo a 12 bits, justificando desde qué región de memoria se está ejecutando este código)

Como la funcion en la que se encuentra esa línea comienza en la direccion 0x7000 y esa linea se encuentra a solo 19 instrucciones de distancia del principio de la función, se tiene que el valor del registro %eip si se lo redondea a 12 bits cuando llegue a "movl $(RELOC(entry_pgdir)), %eax" va a seguir siendo 0x7000 debido a que el offset entre el inicio del codigo y la instruccion se puede representar en menos de 12 bits.

### - ¿Se detiene en algún momento la ejecución si se pone un breakpoint en mpentry_start? ¿Por qué?

No se detiene, ya que la ejecución del codigo de mpentry_start va a pasar por la direccion virtual 0xf0007000. Como esa direccion fue remapeada, al momento de setear el breakpoint mpentry_start apunta a una dirección virtual distinta.


## 5. Con GDB, mostrar el valor exacto de %eip y mpentry_kstack cuando se ejecuta la instrucción anterior en el último AP:

(gdb) b *0x7000
Breakpoint 1 at 0x7000

(gdb) c
Continuing.

Thread 2 received signal SIGTRAP, Trace/breakpoint trap.
[Switching to Thread 1.2]
The target architecture is assumed to be i8086
[ 700:   0]    0x7000:	cli    
0x00000000 in ?? ()

(gdb) disable 1

(gdb) si
[ 700:   1]    0x7001:	xor    %eax,%eax
0x00000001 in ?? ()

(gdb) enable 1

(gdb) c
Continuing.

Thread 3 received signal SIGTRAP, Trace/breakpoint trap.
[Switching to Thread 1.3]
[ 700:   0]    0x7000:	cli    
0x00000000 in ?? ()

(gdb) info threads
  Id   Target Id                    Frame 
  1    Thread 1.1 (CPU#0 [running]) lapic_startap (apicid=-268406784, 
    addr=4027715544) at kern/lapic.c:174
  2    Thread 1.2 (CPU#1 [running]) spin_lock (lk=0x0) at kern/spinlock.c:71
* 3    Thread 1.3 (CPU#2 [running]) 0x00000000 in ?? ()
  4    Thread 1.4 (CPU#3 [halted ]) 0x000fd08c in ?? ()

(gdb) dis 1

(gdb) si
[ 700:   1]    0x7001:	xor    %eax,%eax
0x00000001 in ?? ()

(gdb) en 1

(gdb) c
Continuing.

Thread 4 received signal SIGTRAP, Trace/breakpoint trap.
[Switching to Thread 1.4]
[ 700:   0]    0x7000:	cli    
0x00000000 in ?? ()

(gdb) info threads
  Id   Target Id                    Frame 
  1    Thread 1.1 (CPU#0 [running]) boot_aps () at kern/init.c:109
  2    Thread 1.2 (CPU#1 [running]) spin_lock (lk=0x0) at kern/spinlock.c:71
  3    Thread 1.3 (CPU#2 [running]) spin_lock (lk=0x0) at kern/spinlock.c:71
* 4    Thread 1.4 (CPU#3 [running]) 0x00000000 in ?? ()

(gdb) si 10
The target architecture is assumed to be i386
=> 0x7020:	mov    $0x10,%ax
0x00007020 in ?? ()

(gdb) x/10i $eip
=> 0x7020:	mov    $0x10,%ax
   0x7024:	mov    %eax,%ds
   0x7026:	mov    %eax,%es
   0x7028:	mov    %eax,%ss
   0x702a:	mov    $0x0,%ax
   0x702e:	mov    %eax,%fs
   0x7030:	mov    %eax,%gs
   0x7032:	mov    $0x121000,%eax
   0x7037:	mov    %eax,%cr3
   0x703a:	mov    %cr4,%eax

(gdb) watch $eax == 0x121000
Watchpoint 2: $eax == 0x121000

(gdb) c
Continuing.
=> 0x7037:	mov    %eax,%cr3

Thread 4 hit Watchpoint 2: $eax == 0x121000

Old value = 0
New value = 1
0x00007037 in ?? ()

(gdb) p $eip
$1 = (void (*)()) 0x7037

(gdb) p mpentry_kstack 
$2 = (void *) 0x0

(gdb) x/10i $eip
=> 0x7037:	mov    %eax,%cr3
   0x703a:	mov    %cr4,%eax
   0x703d:	or     $0x10,%eax
   0x7040:	mov    %eax,%cr4
   0x7043:	mov    %cr0,%eax
   0x7046:	or     $0x80010001,%eax
   0x704b:	mov    %eax,%cr0
   0x704e:	mov    0xf0240e84,%esp
   0x7054:	mov    $0x0,%ebp
   0x7059:	mov    $0xf0100250,%eax

(gdb) x/10i $eip
=> 0x7037:	mov    %eax,%cr3
   0x703a:	mov    %cr4,%eax
   0x703d:	or     $0x10,%eax
   0x7040:	mov    %eax,%cr4
   0x7043:	mov    %cr0,%eax
   0x7046:	or     $0x80010001,%eax
   0x704b:	mov    %eax,%cr0
   0x704e:	mov    0xf0240e84,%esp
   0x7054:	mov    $0x0,%ebp
   0x7059:	mov    $0xf0100250,%eax

(gdb) info threads
  Id   Target Id                    Frame 
  1    Thread 1.1 (CPU#0 [running]) boot_aps () at kern/init.c:109
  2    Thread 1.2 (CPU#1 [running]) spin_lock (lk=0x0) at kern/spinlock.c:71
  3    Thread 1.3 (CPU#2 [running]) spin_lock (lk=0x0) at kern/spinlock.c:71
* 4    Thread 1.4 (CPU#3 [running]) 0x00007037 in ?? ()

(gdb) si
=> 0x703a:	mov    %cr4,%eax
0x0000703a in ?? ()

(gdb) p mpentry_kstack 
$3 = (void *) 0x0

(gdb) x/10i $eip
=> 0x703a:	mov    %cr4,%eax
   0x703d:	or     $0x10,%eax
   0x7040:	mov    %eax,%cr4
   0x7043:	mov    %cr0,%eax
   0x7046:	or     $0x80010001,%eax
   0x704b:	mov    %eax,%cr0
   0x704e:	mov    0xf0240e84,%esp
   0x7054:	mov    $0x0,%ebp
   0x7059:	mov    $0xf0100250,%eax
   0x705e:	call   *%eax

(gdb) si
=> 0x703d:	or     $0x10,%eax

Thread 4 hit Watchpoint 2: $eax == 0x121000

Old value = 1
New value = 0
0x0000703d in ?? ()

(gdb) si
=> 0x7040:	mov    %eax,%cr4
0x00007040 in ?? ()

(gdb) si
=> 0x7043:	mov    %cr0,%eax
0x00007043 in ?? ()

(gdb) si
=> 0x7046:	or     $0x80010001,%eax
0x00007046 in ?? ()

(gdb) si
=> 0x704b:	mov    %eax,%cr0
0x0000704b in ?? ()

(gdb) si
=> 0x704e:	mov    0xf0240e84,%esp
0x0000704e in ?? ()

(gdb) x/10i $eip
=> 0x704e:	mov    0xf0240e84,%esp
   0x7054:	mov    $0x0,%ebp
   0x7059:	mov    $0xf0100250,%eax
   0x705e:	call   *%eax
   0x7060:	jmp    0x7060
   0x7062:	xchg   %ax,%ax
   0x7064:	add    %al,(%eax)
   0x7066:	add    %al,(%eax)
   0x7068:	add    %al,(%eax)
   0x706a:	add    %al,(%eax)

(gdb) si
=> 0x7054:	mov    $0x0,%ebp
0x00007054 in ?? ()

(gdb) p mpentry_kstack 
$4 = (void *) 0xf0262000 <percpu_kstacks+131072>

Finalmente mpentry_kstack se mapea en la direccion 0xf0262000


# ipc recv

## Un proceso podría intentar enviar el valor númerico -E_INVAL vía ipc_send(). ¿Cómo es posible distinguir si es un error, o no? En estos casos:

// Versión A
envid_t src = -1;
int r = ipc_recv(&src, 0, NULL);

if (r < 0)
  if (/* ??? */)
    puts("Hubo error.");
  else
    puts("Valor negativo correcto.")

y

// Versión B
int r = ipc_recv(NULL, 0, NULL);

if (r < 0)
  if (/* ??? */)
    puts("Hubo error.");
  else
    puts("Valor negativo correcto.")

En el caso A habria que verificar si src es 0 o no, en caso de que el valor enviado sea efectivamente -E_INVAL la variable src va a tomar el valor del envid del proceso que envio dicho valor, en caso de error y lo que devuelva ipc_recv sea -E_INVAL el valor de src será asignado como 0.

Para el caso B habria que hacer un paso extra ademas de la comprobacion en el if, para asegurarnos de que el valor obtenido haya sido efectivamente enviado por otro proceso tenemos que asignar el atributo thisenv->env_ipc_value en 0 (o en algun valor distinto de -E_INVAL) y en el if verificar si efectivamente thisenv->env_ipc_value != -E_INVAL (para el caso en el que hubo error) y si se ve que thisenv->env_ipc_value es igual a -E_INVAL entonces tuvo que haber sido el valor enviado por otro proceso.


# try ipc send

## ¿Cómo se podría hacer bloqueante esta llamada? Esto es: qué estrategia de implementación se podría usar para que, si un proceso A intenta a enviar a B, pero B no está esperando un mensaje, el proceso A sea puesto en estado ENV_NOT_RUNNABLE, y sea despertado una vez B llame a ipc_recv().

Una forma de hacer esto es asignarle al proceso B una lista (inicialmente vacia) con los procesos que intenten enviarles algo, de forma tal que:
- Si el proceso A quiere enviarle algo a B y B no esta esperando, el proceso A se agregue a la lista de procesos "enviadores" de B y luego A se setee como ENV_NOT_RUNNABLE, de forma tal que cuando se reanude su ejecucion vuelva a verificar si B esta esperando o no y en el caso positivo le mande el dato.
- Si B quiere recibir algo primero verifique si su lista de procesos enviadores esta vacia y en caso de que no lo este setee al primero de estos procesos como ENV_RUNNABLE y lo elimine de su lista, de forma tal que ese proceso pueda continuar su ejecucion y enviarle el dato que quisiera.
















