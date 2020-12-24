// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.
//分配器的数据结构是由空闲物理页组成的链表。
//每个空闲页在列表里都是struct run。
struct run {
  struct run *next;
};
//这个空闲列表使用自旋锁进行保护。
struct kmem{
  struct spinlock lock;
  struct run *freelist;
};
//CPU的核数 为每个cpu都设置freelist
struct kmem kmems[NCPU];

int getcpu(){
  push_off();
  int cpu=cpuid();
  pop_off();
  return cpu;
}

void kinit()
{
  //为每个cpu都初始化锁
  for(int i=0;i<NCPU;i++)
    initlock(&kmems[i].lock, "kmem");//******
  freerange(end, (void*)PHYSTOP);
}
//freerange为所有运行freerange的CPU分配空闲的内存；
void freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p);
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void kfree(void *pa)//pa是那个要被释放的内存块
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");
  memset(pa, 1, PGSIZE);
  r = (struct run*)pa;
  int mn=getcpu();//******获取当前cpu编号
  acquire(&kmems[mn].lock);//******当前队列上锁
  r->next = kmems[mn].freelist;//******将内存块放入当前CPU的freelist中
  kmems[mn].freelist = r;//******
  release(&kmems[mn].lock);//******当前队列解锁
}

//跟别的cpu借内存块
void *borrow(){
  struct run * rs=0;
  //遍历每个cpu
  for(int i=0;i<NCPU;i++){
    acquire(&kmems[i].lock);
    //如果该cpu有空闲内存块，占用，rs指向该内存块，返回rs
    if(kmems[i].freelist!=0){
      rs=kmems[i].freelist;
      kmems[i].freelist=rs->next;
      release(&kmems[i].lock);
      return (void *)rs;
    }
    release(&kmems[i].lock);
  }
  return (void *)rs;
}
// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *kalloc(void)
{
  struct run *r;
  int mn=getcpu();//******获取当前cpu编号
  acquire(&kmems[mn].lock);//******当前队列上锁
  r = kmems[mn].freelist;//获取空闲队列头部内存块
  if(r)//如果当前cpu的空闲列表有空闲内存，占用
    kmems[mn].freelist = r->next;
  release(&kmems[mn].lock);//******当前队列解锁
  if(!r)//当前cpu没有空闲内存块
    r=borrow(mn);//跟别的cpu借内存块
  if(r)
    memset((char*)r, 5, PGSIZE);//填满这个内存块，假装写满了
  return (void*)r;
}
