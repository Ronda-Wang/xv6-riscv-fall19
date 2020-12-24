// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"

#define NBUCKETS 13//设定的缓存数据块个数

struct {
  // Linked list of all buffers, through prev/next.
  // head.next is most recently used.
  struct spinlock lock[NBUCKETS];//自旋锁bcache.lock用于用户互斥访问
  struct buf buf[NBUF];//内存缓存块
  struct buf hashbucket[NBUCKETS];//所有对缓存块的访问都是通过bcache.head引用链表来实现的，而不是buf数组。
} bcache;


int bhash(int blockno){
  return blockno%NBUCKETS;
}

//在系统启动时，main()函数调用binit()来初始化缓存
void binit(void)
{
  struct buf *b;
  for(int i=0;i<NBUCKETS;i++){
    initlock(&bcache.lock[i], "bcache.bucket");//调用initlock()初始化bcache.lock
    b=&bcache.hashbucket[i];//创建各内存缓存区的连接表
    b->prev = b;
    b->next = b;
  }
  //循环遍历buf数组，采用头插法逐个链接到bcache.head后面。
  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    b->next = bcache.hashbucket[0].next;
    b->prev = &bcache.hashbucket[0];
    initsleeplock(&b->lock, "buffer");
    bcache.hashbucket[0].next->prev = b;
    bcache.hashbucket[0].next = b;
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)//字段dev是设备号，字段blockno是缓存数据块号
{
  struct buf *b;
  int h=bhash(blockno);
  //bget()在操作bcache数据结构（修改refcnt、dev、blockno、valid）时，
  //需要获取到自旋锁 bcache.lock，操作完成后再释放该锁。
  acquire(&bcache.lock[h]);
  for(b = bcache.hashbucket[h].next; b != &bcache.hashbucket[h]; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;//被引用数加1
      release(&bcache.lock[h]);
      //bget()在获取到缓存块（命中的缓存块，
      //或者，未命中时通过LRU算法替换出来缓存中的缓存块）后，调用acquiresleep()获取睡眠锁。
      //acquiresleep()：查询b.lock是否被锁，如果被锁了，就睡眠，让出CPU，直到wakeup()唤醒后，获取到锁，并将b.lock置1。
      acquiresleep(&b->lock);
      return b;
    }
  }
  //未命中时通过LRU算法替换出来缓存中的缓存块
  int nh=(h+1)%NBUCKETS; 
  for(;nh!=h;nh=(nh+1)%NBUCKETS){
    acquire(&bcache.lock[nh]);
    for(b = bcache.hashbucket[nh].prev; b != &bcache.hashbucket[nh]; b = b->prev){
      if(b->refcnt == 0) {
        b->dev = dev;
        b->blockno = blockno;
        b->valid = 0;
        b->refcnt = 1;
        b->next->prev=b->prev;
        b->prev->next=b->next;
        release(&bcache.lock[nh]);
        b->next=bcache.hashbucket[h].next;
        b->prev=&bcache.hashbucket[h];
        bcache.hashbucket[h].next->prev=b;
        bcache.hashbucket[h].next=b;
        release(&bcache.lock[h]);
        acquiresleep(&b->lock);
        return b;
      }
    }
    release(&bcache.lock[nh]);
  }
  panic("bget: no buffers");
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)//读缓存块
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b->dev, b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)//把b的内容写入磁盘
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b->dev, b, 1);
}

// Release a locked buffer.
// Move to the head of the MRU list.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);//释放锁，并调用wakeup()
  int h=bhash(b->blockno);
  acquire(&bcache.lock[h]);
  b->refcnt--;//修改被引用次数
  if (b->refcnt == 0) {
    // no one is waiting for it.
    b->next->prev = b->prev;
    b->prev->next = b->next;
    b->next = bcache.hashbucket[h].next;
    b->prev = &bcache.hashbucket[h];
    bcache.hashbucket[h].next->prev = b;
    bcache.hashbucket[h].next = b;
  }
  
  release(&bcache.lock[h]);
}

void
bpin(struct buf *b) {
  int h=bhash(b->blockno);
  acquire(&bcache.lock[h]);
  b->refcnt++;
  release(&bcache.lock[h]);
}

void
bunpin(struct buf *b) {
  int h=bhash(b->blockno);
  acquire(&bcache.lock[h]);
  b->refcnt--;
  release(&bcache.lock[h]);
}
