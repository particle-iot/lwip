/*
 * Copyright (c) 2003 Luke Macpherson.
 * All rights reserved. 
 * 
 * Redistribution and use in source and binary forms, with or without modification, 
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED 
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT 
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT 
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING 
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY 
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 * 
 * Author: Luke Macpherson <lukem@cse.unsw.edu.au>
 *
 */

#include "lwip/opt.h"

#include "lwip/stats.h"

#include "lwip/def.h"
#include "lwip/pbuf.h"
#include "lwip/pbuf-pool.h"

#include "lwip/sys.h"

#include "arch/perf.h"


struct pbuf_manager pbuf_pool_manager = {pbuf_pool_alloc,
                                        pbuf_pool_realloc,
                                        pbuf_pool_free,
                                        pbuf_pool_header};


static u8_t pbuf_pool_memory[(PBUF_POOL_SIZE *
                              MEM_ALIGN_SIZE(PBUF_POOL_BUFSIZE + 
                                             sizeof(struct pbuf_pool))
                             )];

#if !SYS_LIGHTWEIGHT_PROT
static volatile u8_t pbuf_pool_free_lock, pbuf_pool_alloc_lock;
static sys_sem_t pbuf_pool_free_sem;
#endif

static struct pbuf_pool *pbuf_pool = NULL;

/**
 * Initializes the pbuf-pool module.
 *
 * A large part of memory is allocated for holding the pool of pbufs.
 * The size of the individual pbufs in the pool is given by the size
 * parameter, and the number of pbufs in the pool by the num parameter.
 *
 * After the memory has been allocated, the pbufs are set up. The
 * ->next pointer in each pbuf is set up to point to the next pbuf in
 * the pool.
 *
 */
void
pbuf_pool_init(void){
  struct pbuf_pool *p, *q = NULL;
  mem_ptr_t i;

  pbuf_pool = (struct pbuf_pool *)&pbuf_pool_memory[0];
  LWIP_ASSERT("pbuf_pool_init: pool aligned",
              (mem_ptr_t)pbuf_pool % MEM_ALIGNMENT == 0);

#if PBUF_STATS
  lwip_stats.pbuf.avail = PBUF_POOL_SIZE;
#endif /* PBUF_STATS */

  /* Set up ->next pointers to link the pbufs of the pool together */
  p = pbuf_pool;

  for(i = 0; i < PBUF_POOL_SIZE; ++i) {
    p->next = (struct pbuf *)((u8_t *)p + PBUF_POOL_BUFSIZE +
                              sizeof(struct pbuf_pool));
    p->len = p->tot_len = PBUF_POOL_BUFSIZE;
    p->payload = MEM_ALIGN((void *)((u8_t *)p + sizeof(struct pbuf_pool)));
    p->manager = &pbuf_pool_manager;
    q = p;
    p = (struct pbuf_pool*)p->next;
  }

  /* The ->next pointer of last pbuf is NULL to indicate that there
     are no more pbufs in the pool */
  q->next = NULL;

#if !SYS_LIGHTWEIGHT_PROT
  pbuf_pool_alloc_lock = 0;
  pbuf_pool_free_lock = 0;
  pbuf_pool_free_sem = sys_sem_new(1);
#endif

  return;
}

/**
 * @internal only called from pbuf_pool_alloc()
 */
static struct pbuf_pool *
pbuf_pool_internal_alloc(void)
{
  struct pbuf_pool *p = NULL;

  SYS_ARCH_DECL_PROTECT(old_level);
  SYS_ARCH_PROTECT(old_level);

#if !SYS_LIGHTWEIGHT_PROT
  /* Next, check the actual pbuf pool, but if the pool is locked, we
     pretend to be out of buffers and return NULL. */
  if (pbuf_pool_free_lock) {
#if PBUF_STATS
    ++lwip_stats.pbuf.alloc_locked;
#endif /* PBUF_STATS */
    return NULL;
  }
  pbuf_pool_alloc_lock = 1;
  if (!pbuf_pool_free_lock) {
#endif /* SYS_LIGHTWEIGHT_PROT */
    p = pbuf_pool;
    if (p) {
      pbuf_pool = (struct pbuf_pool*)p->next;
    }
#if !SYS_LIGHTWEIGHT_PROT
#if PBUF_STATS
  } else {
    ++lwip_stats.pbuf.alloc_locked;
#endif /* PBUF_STATS */
  }
  pbuf_pool_alloc_lock = 0;
#endif /* SYS_LIGHTWEIGHT_PROT */

#if PBUF_STATS
  if (p != NULL) {
    ++lwip_stats.pbuf.used;
    if (lwip_stats.pbuf.used > lwip_stats.pbuf.max) {
      lwip_stats.pbuf.max = lwip_stats.pbuf.used;
    }
  }
#endif /* PBUF_STATS */

  SYS_ARCH_UNPROTECT(old_level);
  return p;
}

struct pbuf *
pbuf_pool_alloc(u16_t offset, u16_t size, struct pbuf_manager *mgr, void *src){
  struct pbuf_pool *p, *q, *r;
  s32_t rem_len; /* remaining length */

  /* allocate head of pbuf chain into p */
  p = pbuf_pool_internal_alloc();
  LWIP_DEBUGF(PBUF_DEBUG | DBG_TRACE | 3,
              ("pbuf_pool_alloc: allocated pbuf %p\n", (void *)p));
  if (p == NULL) {
#if PBUF_STATS
    ++lwip_stats.pbuf.err;
#endif /* PBUF_STATS */
    return NULL;
  }
  p->next = NULL;

  /* make the payload pointer point 'offset' bytes into pbuf data memory */
  p->payload = MEM_ALIGN((void *)((u8_t *)p + (sizeof(struct pbuf_pool) + offset)));
  LWIP_ASSERT("pbuf_pool_alloc: pbuf p->payload properly aligned",
              ((mem_ptr_t)p->payload % MEM_ALIGNMENT) == 0);
  p->manager = mgr;
#ifdef LWIP_DEBUG
  p->magic = PBUF_POOL_MAGIC;
#endif
  /* the total length of the pbuf chain is the requested size */
  p->tot_len = size;
  /* set the length of the first pbuf in the chain */
  p->len = size > PBUF_POOL_BUFSIZE - offset ?
           PBUF_POOL_BUFSIZE - offset: size;
  /* set reference count (needed here in case we fail) */
  p->ref = 1;

  /* copy len bytes into that buffer */
  if(src!=NULL && p->len>0){
    memcpy(p->payload, src, p->len);
    src = ((u8_t*)src) + p->len;
  }

  /* now allocate the tail of the pbuf chain */

  /* remember first pbuf for linkage in next iteration */
  r = p;
  /* remaining length to be allocated */
  rem_len = size - p->len;
  /* any remaining pbufs to be allocated? */
  while (rem_len > 0) {
    q = pbuf_pool_internal_alloc();
    if (q == NULL) {
     LWIP_DEBUGF(PBUF_DEBUG | 2, ("pbuf_pool_alloc: Out of pbufs in pool.\n"));
#if PBUF_STATS
      ++lwip_stats.pbuf.err;
#endif /* PBUF_STATS */
      /* free chain so far allocated */
      pbuf_pool_free((struct pbuf*)p);
      /* bail out unsuccesfully */
      return NULL;
    }
    q->next = NULL;
    /* make previous pbuf point to this pbuf */
    r->next = (struct pbuf*)q;
    /* set total length of this pbuf and next in chain */
    q->tot_len = rem_len;
    /* this pbuf length is pool size, unless smaller sized tail */
    q->len = rem_len > PBUF_POOL_BUFSIZE? PBUF_POOL_BUFSIZE: rem_len;
    q->payload = (void *)((u8_t *)q + sizeof(struct pbuf_pool));
    q->manager = mgr;
#ifdef LWIP_DEBUG
  p->magic = PBUF_POOL_MAGIC;
#endif
    LWIP_ASSERT("pbuf_pool_alloc: pbuf q->payload properly aligned",
                ((mem_ptr_t)q->payload % MEM_ALIGNMENT) == 0);
    q->ref = 1;

    /* copy len bytes into that buffer */
    if(src!=NULL && q->len>0){
      memcpy(q->payload, src, q->len);
      src = ((u8_t*)src) + q->len;
    }

    /* calculate remaining length to be allocated */
    rem_len -= q->len;
    /* remember this pbuf for linkage in next iteration */
    r = q;
  }
  /* end of chain */
  /*r->next = NULL;*/

  LWIP_DEBUGF(PBUF_DEBUG | DBG_TRACE | 3,
              ("pbuf_pool_alloc(size=%u) == %p\n", size, (void *)p));

  return (struct pbuf*)p;
}

void
pbuf_pool_realloc(struct pbuf *p, u16_t size){
#ifdef LWIP_DEBUG
  struct pbuf_pool *q = (struct pbuf_pool*)p;
  LWIP_ASSERT("pbuf_pool_free(): pbuf was not a pool pbuf!",
              (q->magic==PBUF_POOL_MAGIC));
  LWIP_ASSERT("pbuf_pool_free(): pbuf_pool_realloc can't grow!",
              (q->len >= size));
  LWIP_ASSERT("pbuf_pool_free(): pbuf was not last in chain!",
              (q->next!=NULL));
#endif
  /* shrink the length field for this buffer */
  p->len = size;
  return;
}

#if PBUF_STATS
#define DEC_PBUF_STATS do { --lwip_stats.pbuf.used; } while (0)
#else /* PBUF_STATS */
#define DEC_PBUF_STATS
#endif /* PBUF_STATS */

#define PBUF_POOL_FAST_FREE(p)  do {                                    \
                                  p->next = (struct pbuf*)pbuf_pool;    \
                                  pbuf_pool = p;                        \
                                  DEC_PBUF_STATS;                       \
                                } while (0)

#if SYS_LIGHTWEIGHT_PROT
#define PBUF_POOL_FREE(p)  do {                                         \
                                SYS_ARCH_DECL_PROTECT(old_level);       \
                                SYS_ARCH_PROTECT(old_level);            \
                                PBUF_POOL_FAST_FREE(p);                 \
                                SYS_ARCH_UNPROTECT(old_level);          \
                               } while (0)
#else /* SYS_LIGHTWEIGHT_PROT */
#define PBUF_POOL_FREE(p)  do {                                         \
                             sys_sem_wait(pbuf_pool_free_sem);          \
                             PBUF_POOL_FAST_FREE(p);                    \
                             sys_sem_signal(pbuf_pool_free_sem);        \
                           } while (0)
#endif /* SYS_LIGHTWEIGHT_PROT */


void
pbuf_pool_free(struct pbuf *p){
  struct pbuf_pool *q = (struct pbuf_pool*)p;
#ifdef LWIP_DEBUG
  LWIP_ASSERT("pbuf_pool_free(): pbuf was not a pool pbuf!",
              (q->magic==PBUF_POOL_MAGIC));
#endif
  q->len = q->tot_len = PBUF_POOL_BUFSIZE;
  q->payload = (void *)((u8_t *)q + sizeof(struct pbuf_pool));
  PBUF_POOL_FREE(q);
}

u8_t
pbuf_pool_header(struct pbuf *p, s16_t header_size){
  mem_ptr_t min, payload;

  LWIP_DEBUGF(PBUF_DEBUG, ("pbuf_pool_header(0x%p,%d) was called.\n",
                           p, (int)header_size));

  min = (mem_ptr_t)p + sizeof(struct pbuf_pool);
  payload = (mem_ptr_t)p->payload;

  if(payload - header_size >= min){
    p->payload = (u8_t*)p->payload - header_size;
    p->len += header_size;
    p->tot_len += header_size;
    return 0; /* success */
  }

  return 1; /* failure */
}
