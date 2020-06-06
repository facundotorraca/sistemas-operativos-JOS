// Simple command-line kernel monitor useful for
// controlling the kernel and exploring the system interactively.

#include <inc/stdio.h>
#include <inc/string.h>
#include <inc/memlayout.h>
#include <inc/assert.h>
#include <inc/x86.h>

#include <kern/console.h>
#include <kern/monitor.h>
#include <kern/kdebug.h>

#define CMDBUF_SIZE	80	// enough for one VGA text line


struct Command {
	const char *name;
	const char *desc;
	// return -1 to force monitor to exit
	int (*func)(int argc, char** argv, struct Trapframe* tf);
};

static struct Command commands[] = {
	{ "help", "Display this list of commands", mon_help },
    { "backtrace", "Display the current backtrace", mon_backtrace },
	{ "kerninfo", "Display information about the kernel", mon_kerninfo },
};

/***** Implementations of basic kernel monitor commands *****/

int
mon_help(int argc, char **argv, struct Trapframe *tf)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(commands); i++)
		cprintf("%s - %s\n", commands[i].name, commands[i].desc);
	return 0;
}

int
mon_kerninfo(int argc, char **argv, struct Trapframe *tf)
{
	extern char _start[], entry[], etext[], edata[], end[];

	cprintf("Special kernel symbols:\n");
	cprintf("  _start                  %08x (phys)\n", _start);
	cprintf("  entry  %08x (virt)  %08x (phys)\n", entry, entry - KERNBASE);
	cprintf("  etext  %08x (virt)  %08x (phys)\n", etext, etext - KERNBASE);
	cprintf("  edata  %08x (virt)  %08x (phys)\n", edata, edata - KERNBASE);
	cprintf("  end    %08x (virt)  %08x (phys)\n", end, end - KERNBASE);
	cprintf("Kernel executable memory footprint: %dKB\n",
		ROUNDUP(end - entry, 1024) / 1024);
	return 0;
}

static void
print_backtrace_info(const uint32_t* ebp, uint32_t eip, const uint32_t* args,
                     const struct Eipdebuginfo* info)
{
    uint32_t eip_offset = eip - info->eip_fn_addr;

    cprintf("  ebp %08x  eip %08x  args %08x %08x %08x %08x %08x\n",
            ebp, eip, args[0], args[1], args[2], args[3], args[4]);

    cprintf("         %s:%d: %.*s+%d\n", info->eip_file, info->eip_line,
            info->eip_fn_namelen, info->eip_fn_name, eip_offset);
}

int
mon_backtrace(int argc, char **argv, struct Trapframe *tf)
{
    /*
     * The ebp value indicates the base pointer into the stack used by that
     * function.
     * After saving current ebp, we can  call any function inside, no matter if
     * ebp changes.
    */
     uint32_t* ebp = (uint32_t *)read_ebp();

     /*
      * return address is 4 bytes above ebp.
      * ebp's value just after entering the function, contains the memory
      * address of the top of the stack, then ebp +1 is return address direction.
     */
     uint32_t* stack_eip_addr = ebp + 1;

     /*
      * eip value is the function's return instruction pointer: the instruction
      * address to which control will return when the function returns.
     */
     uintptr_t eip = (uintptr_t)(*stack_eip_addr);

     uint32_t* args;
     struct Eipdebuginfo info;

     //first ebp is initialized with 0x0
     while (ebp != 0) {
         // get info about previous function
         debuginfo_eip(eip, &info);

         // load function args
         args = ebp + 2;

         print_backtrace_info(ebp, eip, args, &info);

         // ebp --> prev_ebp
         ebp = (uint32_t *)(*ebp);

         stack_eip_addr = ebp + 1;
         eip = (uintptr_t)(*stack_eip_addr);
     }

 	return 0;
}



/***** Kernel monitor command interpreter *****/

#define WHITESPACE "\t\r\n "
#define MAXARGS 16

static int
runcmd(char *buf, struct Trapframe *tf)
{
	int argc;
	char *argv[MAXARGS];
	int i;

	// Parse the command buffer into whitespace-separated arguments
	argc = 0;
	argv[argc] = 0;
	while (1) {
		// gobble whitespace
		while (*buf && strchr(WHITESPACE, *buf))
			*buf++ = 0;
		if (*buf == 0)
			break;

		// save and scan past next arg
		if (argc == MAXARGS-1) {
			cprintf("Too many arguments (max %d)\n", MAXARGS);
			return 0;
		}
		argv[argc++] = buf;
		while (*buf && !strchr(WHITESPACE, *buf))
			buf++;
	}
	argv[argc] = 0;

	// Lookup and invoke the command
	if (argc == 0)
		return 0;
	for (i = 0; i < ARRAY_SIZE(commands); i++) {
		if (strcmp(argv[0], commands[i].name) == 0)
			return commands[i].func(argc, argv, tf);
	}
	cprintf("Unknown command '%s'\n", argv[0]);
	return 0;
}

void
monitor(struct Trapframe *tf)
{
	char *buf;

	cprintf("Welcome to the JOS kernel monitor!\n");
	cprintf("Type 'help' for a list of commands.\n");


	while (1) {
		buf = readline("K> ");
		if (buf != NULL)
			if (runcmd(buf, tf) < 0)
				break;
	}
}
