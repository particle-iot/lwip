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

#ifndef __LWIP_PBUF_RAM_H__
#define __LWIP_PBUF_RAM_H__

#include "lwip/pbuf.h"

#ifdef LWIP_DEBUG
#define PBUF_RAM_MAGIC 0x1234
#endif

struct pbuf_ram {
  struct pbuf *next;
  void *payload;
  struct pbuf_manager *manager;
  u16_t tot_len;
  u16_t len;
  u16_t ref;

  /* ram-specific fields follow */
#ifdef LWIP_DEBUGF
  u16_t magic;  /* magic number for checking this is actually ram pbuf */
#endif
};

void pbuf_ram_init(void);

struct pbuf *pbuf_ram_alloc(u16_t offset, u16_t size,
                            struct pbuf_manager *mgr, void *src);

void pbuf_ram_realloc(struct pbuf *p, u16_t size);

void pbuf_ram_free(struct pbuf *p);

u8_t pbuf_ram_header(struct pbuf *p, s16_t header_size);

/* these replace the old allocator "flag" used by lwIP */
extern struct pbuf_manager pbuf_ram_manager;

#define PBUF_RAM   (&pbuf_ram_manager)

#endif /* __LWIP_PBUF_RAM_H__ */
