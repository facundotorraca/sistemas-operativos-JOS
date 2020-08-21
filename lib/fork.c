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
	void *addr = (void *)ROUNDDOWN(utf->utf_fault_va, PGSIZE);
	uint32_t err = utf->utf_err;

	// Check that the faulting access was (1) a write, and (2) to a
	// copy-on-write page.  If not, panic.
	// Hint:
	//   Use the read-only page table mappings at uvpt
	//   (see <inc/memlayout.h>).
    if (!(err & FEC_PR))
        panic("Page not mapped");

    if (!(err & FEC_WR))
        panic("Page fault for reading");

    int perm = uvpt[PGNUM((uintptr_t)addr)] & PTE_SYSCALL;

    if (!(perm & PTE_COW))
        panic("Page fault not copy on write");

	// Allocate a new page, map it at a temporary location (PFTEMP),
	// copy the data from the old page to the new page, then move the new
	// page to the old page's address.
	// Hint:
	//   You should make three system calls.
    int r;

    if ((r = sys_page_alloc(0, PFTEMP, (perm & ~PTE_COW) | PTE_W)) < 0)
        panic("sys_page_alloc: %e", r);

    memmove(PFTEMP, addr, PGSIZE);

    if ((r = sys_page_map(0, PFTEMP, 0, addr, (perm & ~PTE_COW) | PTE_W)) < 0)
        panic("sys_page_map: %e", r);

    if ((r = sys_page_unmap(0, PFTEMP)) < 0)
        panic("sys_page_unmap: %e", r);
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
	pte_t pg = uvpt[pn];
    void *va = (void *)(pn * PGSIZE);

	// Page not present
    if (!(pg & PTE_P))
        return 0;

    // Copy all perms without PTE_W
    int perm = pg & PTE_SYSCALL & ~PTE_W;

    if (pg & PTE_SHARE)
        perm |= (PTE_SHARE | (pg & PTE_W));

    else if ((pg & PTE_W) || (pg & PTE_COW))
        perm |= PTE_COW;

    int r;
    if ((r = sys_page_map(0, va, envid, va, perm)) < 0)
        panic("sys_page_map: %e", r);

    if (perm & PTE_COW) {
        if ((r = sys_page_map(0, va, 0, va, perm)) < 0)
            panic("sys_page_map: %e", r);
    }

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
    set_pgfault_handler(&pgfault);

    envid_t envid = sys_exofork();
    if (envid < 0)
        panic("sys_exofork: %e", envid);

    if (envid == 0) {
        thisenv = &envs[ENVX(sys_getenvid())];
        return 0;
    }

    uintptr_t ptva; // page table start va
    for (ptva = 0; ptva < UTOP; ptva += LG_PGSIZE) {
        if (uvpd[PDX(ptva)] & PTE_P) {

            uintptr_t maxVa = (ptva + LG_PGSIZE) < USTACKTOP ?
                              (ptva + LG_PGSIZE) : USTACKTOP;

            for (uintptr_t va = ptva; va < maxVa; va += PGSIZE)
                duppage(envid, PGNUM(va));
        }
    }

    int r;

    // The parent sets the user page fault entrypoint
    // for the child to look like its own.
    r = sys_page_alloc(envid, (void *)(UXSTACKTOP-PGSIZE), PTE_W | PTE_U | PTE_P);
    if (r < 0)
        panic("Child exception stack memory allocation failed");

    r = sys_env_set_pgfault_upcall(envid, thisenv->env_pgfault_upcall);
    if (r < 0)
        panic("Child set pgfault handler failed!");

    r = sys_env_set_status(envid, ENV_RUNNABLE);
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
