TP1: Memoria virtual en JOS
===========================

# boot_map_region
-------

## ¿cuánta memoria se ahorró de este modo? ¿Es una cantidad fija, o depende de la memoria física de la computadora?

Se ahorro 4Kb (una Page Table completa) por cada "large page" mapeada directamente al "page directory". Se mapean un total de 64 large pages.

La funcion boot_map_region se llama 3 veces en mem_init:

* boot_map_region(kern_pgdir, UPAGES, PTSIZE, (uintptr_t)PADDR(pages), PTE_U | PTE_P);

En este caso no se agrega una large page, ya que UPAGES esta alineada pero la direccion fisica pages no lo esta (pa = 0x001aa000).

* boot_map_region(kern_pgdir, KSTACKTOP - KSTKSIZE, KSTKSIZE,                                (uintptr_t)PADDR(bootstack),PTE_W);

En este caso tampoco no se utiliza una large page ya que la cantidad de memoria mapeada es inferior a 4Mb (KSTKSIZE = 8*PGSIZE = 8*4096 = 32Kb).

* boot_map_region(kern_pgdir, KERNBASE, ROUNDUP(UINT_MAX - KERNBASE, PGSIZE), (uintptr_t)0, PTE_W);

En este caso si se utilizara large pages, ya que se piden mapear 256 MB de memoria, mapeando desde la direccion fisica 0x0 (que si esta alineada a 4Mb) en la direccion virtual KERNBASE (0xF0000000), que tambien esta alineada.

Al mapear directamente desde el Page Directory, nos ahorramos una Page Table por cada entrada.
Cada Page Table puede mapear a 1024 Paginas de 4Kb, es decir a 4Mb. Por lo tanto, por cada large page, se ahorra una page table (4kB). Como se usan 64 large pages (256Mb/4Mb), se ahorran un total de 64 * 4kb = 256 Kb.

Esta cantidad no varia, dado que por mas que tengamos una cantidad inferior a 256Mb de memoria fisica, el mapeo se realiza de igual manera, ya que sera en las funciones page_init() y page_alloc() mira la cantidad de memoria disponible, y page_alloc()

# boot_alloc_pos
--------------

## Un cálculo manual de la primera dirección de memoria que devolverá boot_alloc() tras el arranque

La direccion que devuelve el primer llamado a boot_alloc será 0xf0119000 (dirección virtual).

Para entender el motivo de este valor partimos de que la direccion de end es:

end = 0xf0118950 (Direccion obtenida mediante el comando nm obj/kern/kernel).

Luego, la primera vez que se llama a boot_alloc se inicaliza el valor de la variable
estatica nextfree con la siguiente instruccion:

nextfree = ROUNDUP((char *) end, PGSIZE);
Es decir, el valor de end alineado a PGSIZE. PGSIZE tiene un tamano de 0x1000

0xf0118950 mod 0x1000 = 0x950

Entonces redondeando hacia arriba -> nextfree = 0xf0118950 + 0x1000 - 0x950 = 0xf0119000

Finalmente boot_alloc() devuelve en su primer llamado el valor con que se inicializó nextfree (la direccion virtual mencionada 0xf0119000).

## Una sesión de GDB en la que, poniendo un breakpoint en la función boot_alloc(), se muestre el valor de end y nextfree al comienzo y fin de esa primera llamada a boot_alloc().

Al comienzo de la primer llamada, nextfree toma el valor 0 (ya que todas las variables estaticas en C se inicializan a ese valor por defecto). Al salir, nextfree queda actualizada con el valor de end (alineado a PGSIZE) sumado a la cantidad de paginas necesarias para guardar el array de paginas libres.

nextfree = ROUNDUP((char *) end, PGSIZE) + PGSIZE;

Sabiendo que ROUNDUP((char *) end, PGSIZE) = 0xf0119000 y PGSIZE = 0x1000

nextfree debe valer --> nextfree = 0xf0119000 + 0x1000 = 0xf011a000

Ahora viendo con GDB vemos que:

b boot_alloc
b kern/pmap.c:128
c
p/x nextfree -> $1 = 0x0
p/x end -> $2 = 0xf0119850
c
p/x nextfree -> $3 = 0xf011a000
p/x &end -> $4 = 0xf0118950


# page_alloc
----------

## ¿en qué se diferencia page2pa() de page2kva()?

Ambos reciben un puntero a struct* PageInfo referido a una pagina en memoria.
page2pa por su parte devuelve la direccion fisica de inicio donde esta mapeada la pagina, mientras que page2kva devuelve la direccion virtual vinculada a la direccion fisica de inicio de la pagina (la misma que devuelve page2pa).
