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

#ifndef __LWIP_PBUF_NEW_H__
#define __LWIP_PBUF_NEW_H__

#include "arch/cc.h"
#include "lwip/opt.h"

#define PBUF_TRANSPORT_HLEN 20
#define PBUF_IP_HLEN        20
/* PBUF_LINK_HLEN is defined by lwip/opt.h */

#define PBUF_RAW        0
#define PBUF_LINK       (PBUF_LINK_HLEN + PBUF_RAW)
#define PBUF_IP	        (PBUF_IP_HLEN + PBUF_LINK)
#define PBUF_TRANSPORT  (PBUF_TRANSPORT_HLEN + PBUF_IP)

/* These need to be abstracted:
 *   pbuf_alloc()
 *   pbuf_realloc()
 *   pbuf_header()
 *   pbuf_free()
 */

/*
 * These are generic:
 *   pbuf_clen()
 *   pbuf_ref() ???
 *   pbuf_chain()
 *   pbuf_queue()
 *   pbuf_dequeue()
 *   pbuf_take() - may not be needed anymore?
 *   pbuf_dechain()
 *   pbuf_ref_chain()
 */

struct pbuf_manager {
  struct pbuf * (*pbuf_alloc_new)   (u16_t offset, u16_t size,
                                     struct pbuf_manager *mgr, void *src);
  void (*pbuf_realloc)          (struct pbuf *p, u16_t size);
  void (*pbuf_free)             (struct pbuf *p);
  u8_t (*pbuf_header)           (struct pbuf *p, s16_t header_size);
};

struct pbuf {
  /* next pbuf in singly linked pbuf chain */
  struct pbuf *next;

  /* pointer to the actual data in the buffer */
  void *payload;

  /* allocator functions for the buffer */
  struct pbuf_manager *manager;

  /*
   * total length of this buffer and all next buffers in chain
   * belonging to the same packet.
   *
   * For non-queue packet chains this is the invariant:
   * p->tot_len == p->len + (p->next? p->next->tot_len: 0)
   */
  u16_t tot_len;
  
  /* length of this buffer (starting at payload) */
  u16_t len;

  /*
   * the reference count always equals the number of pointers
   * that refer to this pbuf. This can be pointers from an application,
   * the stack itself, or pbuf->next pointers from a chain.
   */
  u16_t ref;
};

/* pbuf_init():

   Initializes the pbuf module. The num parameter determines how many
   pbufs that should be allocated to the pbuf pool, and the size
   parameter specifies the size of the data allocated to those.  */
void pbuf_init(void);

/*
 * abstracted pbuf interface functions
 *   these can be replaced with #defines, but for the time being
 *   we use functions so that we can check for stupidity
 */

struct pbuf *pbuf_alloc_new(u16_t offset, u16_t size,
                            struct pbuf_manager *mgr, void *src);

#define pbuf_alloc(o,s,m) pbuf_alloc_new(o,s,m,NULL)

void pbuf_realloc(struct pbuf *p, u16_t size);

u8_t pbuf_free(struct pbuf *p);

u8_t pbuf_header(struct pbuf *p, s16_t header_size);

/*
 * generic pbuf interface functions
 */
void pbuf_ref_chain(struct pbuf *p);
void pbuf_ref(struct pbuf *p);
u8_t pbuf_clen(struct pbuf *p);
void pbuf_cat(struct pbuf *h, struct pbuf *t);
void pbuf_chain(struct pbuf *h, struct pbuf *t);
struct pbuf *pbuf_take(struct pbuf *f);
struct pbuf *pbuf_dechain(struct pbuf *p);

/* all of the following code should go away eventually, but for the time 
 * being is here for backward compatibility
 */

#include "pbuf-ram.h"
#include "pbuf-rom.h"
#include "pbuf-pool.h"

/* these replace the old allocator "flag" used by lwIP */
extern struct pbuf_manager pbuf_ref_manager;

/* these should be considered deprecated */
#define PBUF_REF   (&pbuf_rom_manager) /* use rom for the time being */

#endif /* __LWIP_PBUF_NEW_H__ */
