#include "lwip/opt.h"
#include "lwip/pbuf.h"
#include "lwip/sys.h"
#include "arch/perf.h"

void
pbuf_init(void){
  return;
}

/*
 * abstracted pbuf interface functions
 *   these can be replaced with #defines, but for the time being
 *   we use functions so that we can check for stupidity
 */
struct pbuf *
pbuf_alloc_new(u16_t offset, u16_t size, struct pbuf_manager *mgr, void *src){
  LWIP_ASSERT("pbuf_alloc(): NULL pbuf manager", mgr!=NULL);
  return mgr->pbuf_alloc_new(offset, size, mgr, src);
}

void
pbuf_realloc(struct pbuf *p, u16_t new_len){
  LWIP_ASSERT("pbuf_realloc(): NULL pbuf", p!=NULL);
  LWIP_ASSERT("pbuf_realloc(): NULL pbuf manager", p->manager!=NULL);
  struct pbuf *q;
  u16_t rem_len; /* remaining length */
  s16_t grow;

  /* desired length larger than current length? */
  if (new_len >= p->tot_len) {
    /* enlarging not yet supported */
    return;
  }

  /* the pbuf chain grows by (new_len - p->tot_len) bytes
   * (which may be negative in case of shrinking) */
  grow = new_len - p->tot_len;

  /* first, step over any pbufs that should remain in the chain */
  rem_len = new_len;
  q = p;
  /* should this pbuf be kept? */
  while (rem_len > q->len) {
    /* decrease remaining length by pbuf length */
    rem_len -= q->len;
    /* decrease total length indicator */
    q->tot_len += grow;
    /* proceed to next pbuf in chain */
    q = q->next;
  }
  /* we have now reached the new last pbuf (in q) */
  /* rem_len == desired length for pbuf q */

  /* shrink allocated memory */
  /* (may only adjust their length fields) */

  /* reallocate and adjust the length of the pbuf that will be split */
  p->manager->pbuf_realloc(q, rem_len);

  /* adjust length fields for new last pbuf */
  q->len = rem_len;
  q->tot_len = q->len;

  /* any remaining pbufs in chain? */
  if (q->next != NULL) {
    /* free remaining pbufs in chain */
    pbuf_free(q->next);
  }
  /* q is last packet in chain */
  q->next = NULL;
}

u8_t
pbuf_header(struct pbuf *p, s16_t header_size){
  LWIP_ASSERT("pbuf_header(): NULL pbuf", p!=NULL);
  LWIP_ASSERT("pbuf_header(): NULL pbuf manager", p->manager!=NULL);
  return p->manager->pbuf_header(p, header_size);
}

u8_t
pbuf_free(struct pbuf *p){
  struct pbuf *q;
  u8_t count;

  if (p == NULL) {
    LWIP_DEBUGF(PBUF_DEBUG | DBG_TRACE | 2,
                ("pbuf_free(p == NULL) was called.\n"));
    return 0;
  }

  LWIP_DEBUGF(PBUF_DEBUG | DBG_TRACE | 3, ("pbuf_free(%p)\n", (void *)p));

  PERF_START;

  count = 0;

  /* Since decrementing ref cannot be guaranteed to be a single machine 
   * operation we must protect it. Also, the later test of ref must be 
   * protected.
   */
  SYS_ARCH_PROTECT(old_level);

  /* de-allocate all consecutive pbufs from the head of the chain that
   * obtain a zero reference count after decrementing
   */
  while (p != NULL) {
    /* all pbufs in a chain are referenced at least once */
    LWIP_ASSERT("pbuf_free: p->ref > 0", p->ref > 0);

    /* decrease reference count (number of pointers to pbuf) */
    p->ref--;

    /* this pbuf is no longer referenced to? */
    if (p->ref == 0) {
      /* remember next pbuf in chain for next iteration */
      q = p->next;
      LWIP_DEBUGF(PBUF_DEBUG | 2, ("pbuf_free: deallocating %p\n", (void *)p));

      /* call the pbuf-specific free function */
      p->manager->pbuf_free(p);

      count++;

      /* proceed to next pbuf */
      p = q;

    } else {
      /* p->ref > 0, this pbuf is still referenced to */
      /* (and so the remaining pbufs in chain as well) */

      LWIP_DEBUGF(PBUF_DEBUG | 2, ("pbuf_free: %p has ref %u, ending here.\n",
                  (void *)p, (unsigned int)p->ref));

      /* stop walking through chain */
      p = NULL;
    }
  }

  SYS_ARCH_UNPROTECT(old_level);

  PERF_STOP("pbuf_free");

  /* return number of de-allocated pbufs */
  return count;
}


/*
 * generic pbuf interface functions
 */

void
pbuf_ref_chain(struct pbuf *p){

}

void
pbuf_ref(struct pbuf *p){
  SYS_ARCH_DECL_PROTECT(old_level);
  /* pbuf given? */
  if (p != NULL) {
    SYS_ARCH_PROTECT(old_level);
    ++(p->ref);
    SYS_ARCH_UNPROTECT(old_level);
  }
}

u8_t
pbuf_clen(struct pbuf *p){
  u8_t len;

  len = 0;
  while (p != NULL) {
    ++len;
    p = p->next;
  }

  return len;
}

void
pbuf_cat(struct pbuf *h, struct pbuf *t){
  struct pbuf *p;

  LWIP_ASSERT("h != NULL", h != NULL);
  LWIP_ASSERT("t != NULL", t != NULL);

  if (t == NULL)
    return;

  /* proceed to last pbuf of chain */
  for (p = h; p->next != NULL; p = p->next) {
    /* add total length of second chain to all totals of first chain */
    p->tot_len += t->tot_len;
  }

  /* p is last pbuf of first h chain */
  LWIP_ASSERT("p->tot_len == p->len (of last pbuf in chain)",
              p->tot_len == p->len);

  /* add total length of second chain to last pbuf total of first chain */
  p->tot_len += t->tot_len;

  /* chain last pbuf of head (p) with first of tail (t) */
  p->next = t;
}

void
pbuf_chain(struct pbuf *h, struct pbuf *t){
  pbuf_cat(h, t);
  /* t is now referenced to one more time */
  pbuf_ref(t);
  LWIP_DEBUGF(PBUF_DEBUG | DBG_FRESH | 2,
              ("pbuf_chain: %p references %p\n", (void *)h, (void *)t));
}

struct pbuf *
pbuf_take(struct pbuf *f){
  LWIP_ASSERT("pbuf_take() unimplemented", 0);
  return NULL;
}

struct pbuf *
pbuf_dechain(struct pbuf *p){
  struct pbuf *q;
  u8_t tail_gone = 1;
  /* tail */
  q = p->next;
  /* pbuf has successor in chain? */
  if (q != NULL) {
    /* assert tot_len invariant:
     * (p->tot_len == p->len + (p->next? p->next->tot_len : 0)
     */
    LWIP_ASSERT("p->tot_len == p->len + q->tot_len",
                q->tot_len == p->tot_len - p->len);

    /* enforce invariant if assertion is disabled */
    q->tot_len = p->tot_len - p->len;

    /* decouple pbuf from remainder */
    p->next = NULL;

    /* total length of pbuf p is its own length only */
    p->tot_len = p->len;

    /* q is no longer referenced by p, free it */
    LWIP_DEBUGF(PBUF_DEBUG | DBG_STATE,
                ("pbuf_dechain: unreferencing %p\n", (void *)q));
                
    tail_gone = pbuf_free(q);
    if (tail_gone > 0)
      LWIP_DEBUGF(PBUF_DEBUG | DBG_STATE,
                  ("pbuf_dechain: deallocated %p (no longer referenced)\n",
                   (void *)q));
  }

  /* assert tot_len invariant:
   * (p->tot_len == p->len + (p->next? p->next->tot_len: 0)
   */
  LWIP_ASSERT("p->tot_len == p->len", p->tot_len == p->len);

  return (tail_gone > 0? NULL: q);
}
