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

#ifndef __LWIP_PBUF_REF_H__
#define __LWIP_PBUF_REF_H__

#include "lwip/pbuf.h"

#ifdef LWIP_DEBUG
#define PBUF_REF_MAGIC 0x8642
#endif

struct pbuf_ref {
  struct pbuf *next;
  void *payload;
  struct pbuf_manager *manager;
  u16_t tot_len;
  u16_t len;
  u16_t ref;

  /* rom-specific fields follow */
#ifdef LWIP_DEBUGF
  u16_t magic;  /* magic number for checking this is actually rom pbuf */
#endif
};

void pbuf_ref_init(void);

struct pbuf *pbuf_ref_alloc(u16_t offset, u16_t size,
                            struct pbuf_manager *mgr, void *src);

void pbuf_ref_realloc(struct pbuf *p, u16_t size);

void pbuf_ref_free(struct pbuf *p);

u8_t pbuf_ref_header(struct pbuf *p, s16_t header_size);

struct pbuf *pbuf_ref_take(struct pbuf *p);

/* these replace the old allocator "flag" used by lwIP */
extern struct pbuf_manager pbuf_ref_manager;

#define PBUF_REF   (&pbuf_ref_manager)

#endif /* __LWIP_PBUF_REF_H__ */
