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

#include "lwip/pbuf.h"
#include "lwip/pbuf-rom.h"
#include "lwip/pbuf-ram.h"
#include "lwip/mem.h"

struct pbuf_manager pbuf_ref_manager = {pbuf_ref_alloc,
                                        pbuf_ref_realloc,
                                        pbuf_ref_free,
                                        pbuf_ref_header,
					pbuf_ref_take};

void
pbuf_ref_init(void){
  return;
}

struct pbuf *
pbuf_ref_alloc(u16_t offset, u16_t size, struct pbuf_manager *mgr, void *src){
  struct pbuf_ref *p;

  p = mem_malloc(MEM_ALIGN_SIZE(sizeof(struct pbuf_ref)));
  if (p == NULL) {
    return NULL;
  }

  p->next = NULL;
  p->payload = src;
  p->manager = mgr;
  p->tot_len = size;
  p->len = size;
  p->ref = 1;
#ifdef LWIP_DEBUG
  p->magic = PBUF_REF_MAGIC;
#endif

  return (struct pbuf*)p;
}

void
pbuf_ref_realloc(struct pbuf *p, u16_t size){
  /* Can't realloc a rom pbuf, so we just return. This is ok because 
   * pbuf_realloc only ever calls us to shrink the pbuf.
   */
  return;
}

void
pbuf_ref_free(struct pbuf *p){
#ifdef LWIP_DEBUG
  struct pbuf_ref *q = (struct pbuf_ref*)p;
  LWIP_ASSERT("pbuf_ref_free(): pbuf was not a ref pbuf!",
              (q->magic==PBUF_REF_MAGIC));
#endif
	mem_free(p);
}

u8_t
pbuf_ref_header(struct pbuf *p, s16_t header_size){
  return 1; /* fail - you can't resize a ref pbuf */
}

struct pbuf *
pbuf_ref_take(struct pbuf *p){
  struct pbuf *r;
  r = pbuf_alloc_new(0, p->len, &pbuf_ram_manager, p->payload);
  if(r==NULL)
    return NULL;

  /* instert r into the chain */
  r->next = p->next;
  r->tot_len = p->len;
  LWIP_ASSERT("pbuf_ref_take: pbuf_alloc returned wrong length\n",
              r->len==p->len);

  /* free p */
  p->manager->pbuf_free(p);

  return r;
}
