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

RESPONDER

# multicore init

RESPONDER

# ipc recv

RESPONDER

# try ipc send

RESPONDER (ver clase del martes 3/8)















