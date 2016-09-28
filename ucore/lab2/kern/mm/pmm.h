#ifndef __KERN_MM_PMM_H__
#define __KERN_MM_PMM_H__

#include <defs.h>
#include <mmu.h>
#include <memlayout.h>
#include <atomic.h>
#include <assert.h>

// pmm_manager is a physical memory management class. A special pmm manager - XXX_pmm_manager
// only needs to implement the methods in pmm_manager class, then XXX_pmm_manager can be used
// by ucore to manage the total physical memory space.
// 这里是在用c来模拟类吗？
struct pmm_manager {
    const char *name;                                 // XXX_pmm_manager's name
    void (*init)(void);                               // initialize internal description&management data structure
                                                      // (free block list, number of free block) of XXX_pmm_manager 
    void (*init_memmap)(struct Page *base, size_t n); // setup description&management data structcure according to
                                                      // the initial free physical memory space 
	// 下面的函数是用来分配页面吗？
    struct Page *(*alloc_pages)(size_t n);            // allocate >=n pages, depend on the allocation algorithm 
	// 下面的函数用于释放页面
    void (*free_pages)(struct Page *base, size_t n);  // free >=n pages with "base" addr of Page descriptor structures(memlayout.h)
	// 确实很像一个类
    size_t (*nr_free_pages)(void);                    // return the number of free pages 
    void (*check)(void);                              // check the correctness of XXX_pmm_manager 
};

extern const struct pmm_manager *pmm_manager;
extern pde_t *boot_pgdir;
extern uintptr_t boot_cr3;

void pmm_init(void);

struct Page *alloc_pages(size_t n);
void free_pages(struct Page *base, size_t n);
size_t nr_free_pages(void);

#define alloc_page() alloc_pages(1)
#define free_page(page) free_pages(page, 1)

pte_t *get_pte(pde_t *pgdir, uintptr_t la, bool create);
struct Page *get_page(pde_t *pgdir, uintptr_t la, pte_t **ptep_store);
void page_remove(pde_t *pgdir, uintptr_t la);
int page_insert(pde_t *pgdir, struct Page *page, uintptr_t la, uint32_t perm);

void load_esp0(uintptr_t esp0);
void tlb_invalidate(pde_t *pgdir, uintptr_t la);

void print_pgdir(void);

/* *
 * PADDR - takes a kernel virtual address (an address that points above KERNBASE),
 * where the machine's maximum 256MB of physical memory is mapped and returns the
 * corresponding physical address.  It panics if you pass it a non-kernel virtual address.
 * */
// 输入的kva是一个虚拟地址
// 然后这个宏的作用是返回对应的物理地址
#define PADDR(kva) ({                                                   \
            uintptr_t __m_kva = (uintptr_t)(kva);                       \
            if (__m_kva < KERNBASE) {                                   \
                panic("PADDR called with invalid kva %08lx", __m_kva);  \
            }                                                           \
            __m_kva - KERNBASE;                                         \
        })

/* *
 * KADDR - takes a physical address and returns the corresponding kernel virtual
 * address. It panics if you pass an invalid physical address.
 * */
// pa是一个物理地址
// PPN是取得高22位的值，也就是所谓的页的索引
// 返回了这个物理地址对应的虚拟地址
#define KADDR(pa) ({                                                    \
            uintptr_t __m_pa = (pa);                                    \
            size_t __m_ppn = PPN(__m_pa);                               \
            if (__m_ppn >= npage) {                                     \
                panic("KADDR called with invalid pa %08lx", __m_pa);    \
            }                                                           \
            (void *) (__m_pa + KERNBASE);                               \
        })

extern struct Page *pages;
extern size_t npage; // 页的数目，是吧。

// 好吧，我们来看一下page2ppn，page是一个 虚拟地址，pages也是一个虚拟地址，
// 好吧，这个函数实际上是找出page在pages中的索引
static inline ppn_t
page2ppn(struct Page *page) {
    return page - pages;
}

// 好吧，我们来看一下这个page2pa吧，也就是完成page到physical address的转换
//  page2ppn(page)找到page在pages中的索引号，然后左移12位，得到对应的物理地址
static inline uintptr_t
page2pa(struct Page *page) {
    return page2ppn(page) << PGSHIFT;
}

// 这个函数主要用于将物理地址转行成page
static inline struct Page *
pa2page(uintptr_t pa) { // 将物理地址转换为page？
	// pa是一个物理地址,然后PPN(pa)返回这个地址在页表中的索引
    if (PPN(pa) >= npage) {
        panic("pa2page called with invalid pa");
    }
	// 然后返回对应的Page*的索引
    return &pages[PPN(pa)];
}

// 好吧，这也是非常重要的一个函数
// 具体的功能是实现从page转换到kernel visual address
static inline void *
page2kva(struct Page *page) {
    return KADDR(page2pa(page)); // page2pa(page)返回page指定的实际物理页的首地址（也是这个物理页物理地址）
}

static inline struct Page *
kva2page(void *kva) {
    return pa2page(PADDR(kva));
}

// 现在真是越来越奇怪了，真的，pte是page table entry
static inline struct Page *
pte2page(pte_t pte) {
    if (!(pte & PTE_P)) {
        panic("pte2page called with invalid pte");
    }
	// PTE_ADDR返回pte对应的基地址,是物理地址吗？
	// 好吧，返回对应的Page的地址
    return pa2page(PTE_ADDR(pte));
}

static inline struct Page *
pde2page(pde_t pde) {
    return pa2page(PDE_ADDR(pde));
}

static inline int
page_ref(struct Page *page) {
    return page->ref;
}

static inline void
set_page_ref(struct Page *page, int val) {
    page->ref = val;
}

static inline int
page_ref_inc(struct Page *page) {
    page->ref += 1;
    return page->ref;
}

static inline int
page_ref_dec(struct Page *page) {
    page->ref -= 1;
    return page->ref;
}

extern char bootstack[], bootstacktop[];

#endif /* !__KERN_MM_PMM_H__ */

