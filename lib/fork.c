// implement fork from user space

#include <inc/string.h>
#include <inc/lib.h>

#define LG_PGSIZE 4194304 //4*1024*1024 4mb

// PTE_COW marks copy-on-write page table entries.
// It is one of the bits explicitly allocated to user processes (PTE_AVAIL).
#define PTE_COW 0x800

//
// Custom page fault handler - if faulting page is copy-on-write,
// map in our own private writable copy.
//
static void
pgfault(struct UTrapframe *utf)
{
	void *addr = (void *) utf->utf_fault_va;
	uint32_t err = utf->utf_err;
	int r;

	// Check that the faulting access was (1) a write, and (2) to a
	// copy-on-write page.  If not, panic.
	// Hint:
	//   Use the read-only page table mappings at uvpt
	//   (see <inc/memlayout.h>).

	// LAB 4: Your code here.

	// Allocate a new page, map it at a temporary location (PFTEMP),
	// copy the data from the old page to the new page, then move the new
	// page to the old page's address.
	// Hint:
	//   You should make three system calls.

	// LAB 4: Your code here.

	panic("pgfault not implemented");
}

//
// Map our virtual page pn (address pn*PGSIZE) into the target envid
// at the same virtual address.  If the page is writable or copy-on-write,
// the new mapping must be created copy-on-write, and then our mapping must be
// marked copy-on-write as well.  (Exercise: Why do we need to mark ours
// copy-on-write again if it was already copy-on-write at the beginning of
// this function?)
//
// Returns: 0 on success, < 0 on error.
// It is also OK to panic on error.
//
static int
duppage(envid_t envid, unsigned pn)
{
	int r;

	// LAB 4: Your code here.
	panic("duppage not implemented");
	return 0;
}

static void
dup_or_share(envid_t dstenv, void *va, int perm)
{
    if (!(perm & PTE_P))
        return;

    int r;
    if ((perm & PTE_W) == 0) {
        if ((r = sys_page_map(0, va, dstenv, va, perm)) < 0)
            panic("sys_page_map: %e", r);
        return;
    }

    if ((r = sys_page_alloc(dstenv, va, perm)) < 0)
        panic("sys_page_alloc: %e", r);

    if ((r = sys_page_map(dstenv, va, 0, UTEMP, perm)) < 0)
        panic("sys_page_map: %e", r);

    memmove(UTEMP, va, PGSIZE);

    if ((r = sys_page_unmap(0, UTEMP)) < 0)
        panic("sys_page_unmap: %e", r);
}

static envid_t
fork_v0(void)
{
    envid_t envid = sys_exofork();

    if (envid < 0)
        panic("sys_exofork: %e", envid);

    if (envid == 0) {
        thisenv = &envs[ENVX(sys_getenvid())];
        return 0;
    }

    uintptr_t va;
    for (va = 0; va < UTOP; va += PGSIZE) {
        if ((uvpd[PDX(va)] & PTE_P) && (uvpt[PGNUM(va)] & PTE_P))
            dup_or_share(envid, (void *)va, PGOFF(uvpt[PGNUM(va)]) & PTE_SYSCALL);
    }

    int r = sys_env_set_status(envid, ENV_RUNNABLE);
    if (r < 0)
        return r;

    return envid;
}

//
// User-level fork with copy-on-write.
// Set up our page fault handler appropriately.
// Create a child.
// Copy our address space and page fault handler setup to the child.
// Then mark the child as runnable and return.
//
// Returns: child's envid to the parent, 0 to the child, < 0 on error.
// It is also OK to panic on error.
//
// Hint:
//   Use uvpd, uvpt, and duppage.
//   Remember to fix "thisenv" in the child process.
//   Neither user exception stack should ever be marked copy-on-write,
//   so you must allocate a new page for the child's user exception stack.
//
envid_t
fork(void)
{
    set_pgfault_handler(pgfault);

    envid_t envid = sys_exofork();

    if (envid < 0)
        panic("sys_exofork: %e", envid);

    if (envid == 0) {
        thisenv = &envs[ENVX(sys_getenvid())];
        set_pgfault_handler(pgfault);
        return 0;
    }

    uintptr_t ptva; // page table start va
    for (ptva = 0; ptva < UTOP; ptva += LG_PGSIZE) {
        if ((uvpd[PDX(ptva)] & PTE_P)) {

            uintptr_t maxVa = (ptva + LG_PGSIZE) < USTACKTOP ?
                              (ptva + LG_PGSIZE) : USTACKTOP;

            for (uintptr_t va = ptva; va < maxVa; va += PGSIZE)
                dup_or_share(envid, (void *)va, PGOFF(uvpt[PGNUM(va)]) & PTE_SYSCALL);
        }
    }

    int r = sys_env_set_status(envid, ENV_RUNNABLE);
    if (r < 0)
        return r;

    return envid;
}

// Challenge!
int
sfork(void)
{
	panic("sfork not implemented");
	return -E_INVAL;
}
