#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "x86.h"

static void startothers(void);
static void mpmain(void)  __attribute__((noreturn));
extern pde_t *kpgdir;
extern char end[]; // first address after kernel loaded from ELF file

// Bootstrap processor starts running C code here.
// Allocate a real stack and switch to it, first
// doing some setup required for memory allocator to work.
int
main(void)
{
  cprintf("[KERNEL] main() started\n");
  cprintf("[KERNEL] Bootstrap Processor (BSP) starting initialization\n");
  
  kinit1(end, P2V(4*1024*1024)); // phys page allocator
  kvmalloc();      // kernel page table
  cprintf("[KERNEL] Kernel page table initialized\n");
  
  mpinit();        // detect other processors
  cprintf("[KERNEL] Multiprocessor detection complete\n");
  
  lapicinit();     // interrupt controller
  cprintf("[KERNEL] Local APIC initialized\n");
  
  seginit();       // segment descriptors
  cprintf("[KERNEL] Segment descriptors initialized\n");
  
  picinit();       // disable pic
  cprintf("[KERNEL] Legacy PIC disabled\n");
  
  ioapicinit();    // another interrupt controller
  cprintf("[KERNEL] I/O APIC initialized\n");
  
  consoleinit();   // console hardware
  cprintf("[KERNEL] Console initialized\n");
  
  uartinit();      // serial port
  cprintf("[KERNEL] UART initialized\n");
  
  pinit();         // process table
  cprintf("[KERNEL] Process table initialized\n");
  
  tvinit();        // trap vectors
  cprintf("[KERNEL] Trap vectors initialized\n");
  
  binit();         // buffer cache
  cprintf("[KERNEL] Buffer cache initialized\n");
  
  fileinit();      // file table
  cprintf("[KERNEL] File table initialized\n");
  
  ideinit();       // disk 
  cprintf("[KERNEL] IDE disk controller initialized\n");
  
  cprintf("[KERNEL] Starting other processors (APs)...\n");
  startothers();   // start other processors
  cprintf("[KERNEL] All Application Processors (APs) started\n");
  
  kinit2(P2V(4*1024*1024), P2V(PHYSTOP)); // must come after startothers()
  cprintf("[KERNEL] Full physical memory allocator initialized\n");
  
  cprintf("[KERNEL] Creating first user process (init)...\n");
  userinit();      // first user process
  cprintf("[KERNEL] First user process created\n");
  
  cprintf("[KERNEL] Bootstrap Processor setup complete, entering scheduler\n");
  mpmain();        // finish this processor's setup
}

// Other CPUs jump here from entryother.S.
static void
mpenter(void)
{
  cprintf("[KERNEL] Application Processor (AP) %d starting\n", cpuid());
  switchkvm();
  seginit();
  lapicinit();
  mpmain();
}

// Common CPU setup code.
static void
mpmain(void)
{
  int cpu_id = cpuid();
  cprintf("[KERNEL] cpu%d: starting scheduler\n", cpu_id);
  idtinit();       // load idt register
  xchg(&(mycpu()->started), 1); // tell startothers() we're up
  cprintf("[KERNEL] cpu%d: entering scheduler\n", cpu_id);
  scheduler();     // start running processes
}

pde_t entrypgdir[];  // For entry.S

// Start the non-boot (AP) processors.
static void
startothers(void)
{
  extern uchar _binary_entryother_start[], _binary_entryother_size[];
  uchar *code;
  struct cpu *c;
  char *stack;

  // Write entry code to unused memory at 0x7000.
  // The linker has placed the image of entryother.S in
  // _binary_entryother_start.
  code = P2V(0x7000);
  memmove(code, _binary_entryother_start, (uint)_binary_entryother_size);
  cprintf("[KERNEL] AP bootstrap code copied to 0x7000\n");

  for(c = cpus; c < cpus+ncpu; c++){
    if(c == mycpu()) { // We've started already.
      cprintf("[KERNEL] Skipping BSP (cpu%d)\n", c->apicid);
      continue;
    }

    cprintf("[KERNEL] Starting AP cpu%d...\n", c->apicid);
    
    // Tell entryother.S what stack to use, where to enter, and what
    // pgdir to use. We cannot use kpgdir yet, because the AP processor
    // is running in low  memory, so we use entrypgdir for the APs too.
    stack = kalloc();
    cprintf("[KERNEL] Allocated stack for cpu%d at %p\n", c->apicid, stack);
    *(void**)(code-4) = stack + KSTACKSIZE;
    *(void(**)(void))(code-8) = mpenter;
    *(int**)(code-12) = (void *) V2P(entrypgdir);

    lapicstartap(c->apicid, V2P(code));

    // wait for cpu to finish mpmain()
    while(c->started == 0)
      ;
      
    cprintf("[KERNEL] AP cpu%d started successfully\n", c->apicid);
  }
}

// The boot page table used in entry.S and entryother.S.
// Page directories (and page tables) must start on page boundaries,
// hence the __aligned__ attribute.
// PTE_PS in a page directory entry enables 4Mbyte pages.

__attribute__((__aligned__(PGSIZE)))
pde_t entrypgdir[NPDENTRIES] = {
  // Map VA's [0, 4MB) to PA's [0, 4MB)
  [0] = (0) | PTE_P | PTE_W | PTE_PS,
  // Map VA's [KERNBASE, KERNBASE+4MB) to PA's [0, 4MB)
  [KERNBASE>>PDXSHIFT] = (0) | PTE_P | PTE_W | PTE_PS,
};

//PAGEBREAK!
// Blank page.
//PAGEBREAK!
// Blank page.
//PAGEBREAK!
// Blank page.
