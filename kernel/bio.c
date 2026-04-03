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

struct {
  struct spinlock lock;
  struct buf buf[NBUF];

  struct spinlock bucket_locks[NBUCKET];
  struct buf buckets[NBUCKET];
} bcache;

void
binit(void)
{
  struct buf *b;

  initlock(&bcache.lock, "bcache");

  for (int i = 0; i < NBUCKET; i++) {
    char lock_name[16];
    snprintf(lock_name, 16, "bcache_%d", i);
    initlock(&bcache.bucket_locks[i], lock_name);
    
    bcache.buckets[i].next = &bcache.buckets[i];
    bcache.buckets[i].prev = &bcache.buckets[i];
  }

  // Create linked list of buffers
  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    b->next = bcache.buckets[0].next;
    b->prev = &bcache.buckets[0];
    initsleeplock(&b->lock, "buffer");
    bcache.buckets[0].next->prev = b;
    bcache.buckets[0].next = b;
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;

  // Is the block already cached?
  int id = blockno % NBUCKET;
  acquire(&bcache.bucket_locks[id]);

  for(b = bcache.buckets[id].next; b != &bcache.buckets[id]; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.bucket_locks[id]);
      acquiresleep(&b->lock);
      return b;
    }
  }
  release(&bcache.bucket_locks[id]);

  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  acquire(&bcache.lock);
  acquire(&bcache.bucket_locks[id]);
  for(b = bcache.buckets[id].next; b != &bcache.buckets[id]; b = b->next){
     if(b->dev == dev && b->blockno == blockno){
       b->refcnt++;
       release(&bcache.lock);
       release(&bcache.bucket_locks[id]);
       acquiresleep(&b->lock);
       return b;
     }
  }
  release(&bcache.bucket_locks[id]);

  uint min_timestamp = ~0;
  int old_id = -1;
  struct buf *evict_b = 0;

retry_eviction:
  min_timestamp = ~0;
  old_id = -1;
  evict_b = 0;
  for (int i = 0; i < NBUCKET; i++) {
    acquire(&bcache.bucket_locks[i]);
    for(b = bcache.buckets[i].next; b != &bcache.buckets[i]; b = b->next){
      if(b->refcnt == 0) {
        if (min_timestamp > b->timestamp) {
          evict_b = b;
          min_timestamp = b->timestamp;
          old_id = i;
        }
      }
    }
    release(&bcache.bucket_locks[i]);
  }

  if (!evict_b)
    panic("bget: no buffers");

  acquire(&bcache.bucket_locks[old_id]);
  if (evict_b->refcnt) {
    release(&bcache.bucket_locks[old_id]);
    goto retry_eviction;
  }

  evict_b->dev = dev;
  evict_b->blockno = blockno;
  evict_b->valid = 0;
  evict_b->refcnt = 1;

  if (old_id != id) {
    evict_b->next->prev = evict_b->prev;
    evict_b->prev->next = evict_b->next;

    acquire(&bcache.bucket_locks[id]);

    evict_b->next = bcache.buckets[id].next;
    evict_b->prev = &bcache.buckets[id];
    bcache.buckets[id].next->prev = evict_b;
    bcache.buckets[id].next = evict_b;

    release(&bcache.bucket_locks[id]);
  }
  release(&bcache.lock);
  release(&bcache.bucket_locks[old_id]);

  acquiresleep(&evict_b->lock);
  return evict_b;
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

  int id = b->blockno & NBUCKET;

  acquire(&bcache.bucket_locks[id]);
  b->refcnt--;
  if (b->refcnt == 0) {
    // no one is waiting for it.
    b->timestamp = ticks;
  }
  release(&bcache.bucket_locks[id]);
}

void
bpin(struct buf *b) {
  int id = b->blockno % NBUCKET;
  acquire(&bcache.bucket_locks[id]);
  b->refcnt++;
  release(&bcache.bucket_locks[id]);
}

void
bunpin(struct buf *b) {
  int id = b->blockno % NBUCKET;
  acquire(&bcache.bucket_locks[id]);
  b->refcnt--;
  release(&bcache.bucket_locks[id]);
}


