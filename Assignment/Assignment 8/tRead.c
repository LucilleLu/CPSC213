#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/errno.h>
#include <assert.h>
#include "queue.h"
#include "disk.h"
#include "uthread.h"

unsigned long sum = 0;

/**
 * Queue of threads blocked awaiting a block
 */
queue_t prq;


struct pending_read {
    char* buf;
    int   blockno;
    
};

/**
 * Called automatically when an interrupt occurs to signal that a read has completed.
 */
void interrupt_service_routine () {
    // TODO
    uthread_unblock(queue_dequeue(&prq)); //wakeup when the disk read has completed
}

/**
 * Handle read by ensuring the buffer stores the correct data by
 *  (1) asserting that the first 4 bytes store the correct blockno; and
 *  (2) summing the value in the next 4 bytes as indicated
 * Marking is based partially on having the right value of sum when the
 * program completes.  Note that value written in the second 4 bytes may
 * be different when the assignment is marked.
 */
void handle_read (char* buf, int blockno) {
  assert (*((int*) buf) == blockno);
  sum = sum * 1.1 + *(((int*) buf) + 1);
}

/**
 * Read block number blockno and process the block by calling handle_read
 *
 * This is the inner part of the run loop in the other two versions,
 * abstracted here into a procedure ... why?
 */
void* read (void* blockno_v) {
  // TODO
    struct pending_read* pr = (struct pending_read*)blockno_v;
    disk_schedule_read (pr->buf, sizeof(pr->buf), pr->blockno);
    uthread_block();
    handle_read(pr->buf, pr->blockno);
  return NULL;
}

/**
 * Read a count of num_blocks and wait for all reads to complete
 */
void run (int num_blocks) {
    // TODO
    uthread_t threads[num_blocks];
    char buf[num_blocks][8];
    for (int i=0; i < num_blocks; i++) {
        struct pending_read pr;
        pr.buf = buf[i];
        pr.blockno = i;
        threads[i] = uthread_create(read, &pr);
        queue_enqueue (&prq, threads[i]);
    }
    for (int i=0; i<num_blocks; i++) {
        uthread_join(threads[i], 0);
    }
}

int main (int argc, char** argv) {
  static const char* usage = "usage: tRead numBlocks";
  int num_blocks = 0;

  if (argc == 2)
    num_blocks = strtol (argv [1], NULL, 10);
  if (argc != 2 || (num_blocks == 0 && errno == EINVAL)) {
    printf ("%s\n", usage);
    return EXIT_FAILURE;
  }
  
  uthread_init (1);
  disk_start   (interrupt_service_routine);
  queue_init   (&prq);
  
  run (num_blocks);
  
  printf ("%ld\n", sum);
}
