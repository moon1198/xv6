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

//NBUf = 30;
struct {
  struct spinlock lock;
  struct buf buf[NBUF];

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
} bcache;

#define NBUCKET 13
struct bucket {
  struct spinlock lock;
  struct buf head;
} hashtable[NBUCKET];

static char htblnames[NBUCKET][16];

int 
hash (uint dev, uint blockno) {
    return ((dev * 50) + blockno) % NBUCKET;
}


void
binit(void)
{
  struct buf *b;

  initlock(&bcache.lock, "bcache");
  b = bcache.buf;
  int mean = NBUF / NBUCKET;

  for (int i = 0; i < NBUCKET; ++ i) {
      snprintf(htblnames[i], 16 - 1, "hashtable%d", i);
      initlock(&hashtable[i].lock, htblnames[i]);
      struct buf *tmp = b;
      hashtable[i].head.next = &hashtable[i].head;
      hashtable[i].head.prev = &hashtable[i].head;
      for (tmp = b; tmp < b + mean; tmp ++) {
          initsleeplock(&tmp->lock, "buffer");
          tmp->idx = i;
          tmp->next = hashtable[i].head.next;
          tmp->prev = &hashtable[i].head;
          hashtable[i].head.next->prev = tmp;
          hashtable[i].head.next = tmp;
      }
      b = tmp;
  }
  for (; b < bcache.buf + NBUF; ++ b) {
    initsleeplock(&b->lock, "buffer");
    b->idx = 0;
    b->next = hashtable[0].head.next;
    b->prev = &hashtable[0].head;
    hashtable[0].head.next->prev = b;
    hashtable[0].head.next = b;
  }

}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;

  int idx = hash(dev, blockno);

  // Is the block already cached?
  struct bucket *cur_table = &hashtable[idx];

  acquire(&cur_table->lock);
  for(b = cur_table->head.next; b != &cur_table->head; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&cur_table->lock);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // Not cached.
  //find free buffer from current bucket
  for(b = cur_table->head.next; b != &cur_table->head; b = b->next){
      if (b->refcnt == 0) {
      b->dev = dev;
      b->blockno = blockno;
      b->valid = 0;
      b->refcnt = 1;
      release(&cur_table->lock);
      acquiresleep(&b->lock);
      return b;
      }
  }
  release(&cur_table->lock);

  //find free buffer from other bucket
  //acquire(&bcache.lock);
  for (b = bcache.buf; b < bcache.buf + NBUF; ++ b) {
      if (b->refcnt == 0) {
          acquire(&hashtable[b->idx].lock);
          if (b->refcnt == 0) {
              b->prev->next = b->next;
              b->next->prev = b->prev;
              release(&hashtable[b->idx].lock);

              b->dev = dev;
              b->blockno = blockno;
              b->valid = 0;
              b->refcnt = 1;

              acquire(&hashtable[idx].lock);
              b->prev = &hashtable[idx].head;
              b->next = hashtable[idx].head.next;
              hashtable[idx].head.next->prev = b;
              hashtable[idx].head.next = b;

              b->idx = idx;
              release(&hashtable[idx].lock);

              acquiresleep(&b->lock);
              //release(&bcache.lock);
              return b;
          } else {
              release(&hashtable[b->idx].lock);
          }
      }
  }
  //release(&bcache.lock);

  panic("bget: no buffers");
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);
  int idx = b->idx;

  acquire(&hashtable[idx].lock);
  b->refcnt--;
  if (b->refcnt == 0) {
    // no one is waiting for it.
    //放在最前面
    b->next->prev = b->prev;
    b->prev->next = b->next;
    b->next = hashtable[idx].head.next;
    b->prev = &hashtable[idx].head;
    hashtable[idx].head.next->prev = b;
    hashtable[idx].head.next = b;
  }
  
  release(&hashtable[idx].lock);
}

void
bpin(struct buf *b) {
  int idx = b->idx;
  acquire(&hashtable[idx].lock);
  b->refcnt++;
  release(&hashtable[idx].lock);
}

void
bunpin(struct buf *b) {
  int idx = b->idx;
  acquire(&hashtable[idx].lock);
  b->refcnt--;
  release(&hashtable[idx].lock);
}


