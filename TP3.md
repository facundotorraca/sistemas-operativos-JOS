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

RESPONDER

# ipc recv

RESPONDER

# try ipc send

RESPONDER (ver clase del martes 3/8)















