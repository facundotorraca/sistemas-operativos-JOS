TP1: Memoria virtual en JOS
===========================

page2pa
-------

# boot_alloc_pos
--------------

## Un cálculo manual de la primera dirección de memoria que devolverá boot_alloc() tras el arranque

Imprime la posicion f0119000.

0xf0119000 es una direccion virtual.

Para entender el motivo de este valor partimos de que la direccion de end es:

end = 0xf0118950.
(Direccion obtenida mediante el comando nm obj/kern/kernel).

Luego, la primera vez que se llama a boot_alloc se inicaliza el valor de la variable
estatica nextfree con el la siguiente instruccion:

nextfree = ROUNDUP((char *) end, PGSIZE);
Es decir, el valor de end alineado a PGSIZE. PGSIZE tiene un tamano de 0x1000

0xf0118950 mod 0x1000 = 0x950

entonces redondeando hacia arriba -> nextfree = 0xf0118950 + 0x1000 - 0x950 = 0xf0119000

##Una sesión de GDB en la que, poniendo un breakpoint en la función boot_alloc(), se muestre el valor de end y nextfree al comienzo y fin de esa primera llamada a boot_alloc().

Al comienzo de la primer llamada, nextfree toma el valor 0 (ya que todas las variables estaticas en C se inicializan a ese valor por defecto). Al salir, nextfree queda actualizada con su valor al cual es inicializa (end) sumado a la cantidad de paginas necesarias para guardar el array de paginas libres.

nextfree = ROUNDUP((char *) end, PGSIZE) + PGSIZE;

Sabiendo que ROUNDUP((char *) end, PGSIZE) = 0xf0119000 y PGSIZE = 0x1000

nextfree debe valer --> nextfree = 0xf0119000 + 0x1000 = 0xf011a000

Ahora viendo con GDB vemos que:

b boot_alloc
b kern/pmap.c:128
c
p nextfree -> $1 = 0x0
c
p nextfree -> $2 = 0xf011a000

# page_alloc
----------

## ¿en qué se diferencia page2pa() de page2kva()?

Ambos reciben un puntero a struct* PageInfo referido a una pagina en memoria.
page2pa por su parte devuelve la direccion fisica de inicio donde esta mapeada la pagina, mientras que page2kva devuelve la direccion virtual vinculada a la direccion fisica de inicio de la pagina (la misma que devuelve page2pa).
