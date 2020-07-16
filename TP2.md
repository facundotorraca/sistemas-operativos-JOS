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

...


# env_pop_tf
----------
## ¿Cómo determina la CPU (en x86) si hay un cambio de ring (nivel de privilegio)? Ayuda: Responder antes en qué lugar exacto guarda x86 el nivel de privilegio actual. ¿Cuántos bits almacenan ese privilegio?



gdb_hello
---------

...
