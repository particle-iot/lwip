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
#include "lwip/pbuf-ram.h"
#include "lwip/mem.h"

struct pbuf_manager pbuf_ram_manager = {pbuf_ram_alloc,
                                        pbuf_ram_realloc,
                                        pbuf_ram_free,
                                        pbuf_ram_header,
					pbuf_ram_take};

void
pbuf_ram_init(void){
  return;
}

struct pbuf *
pbuf_ram_alloc(u16_t offset, u16_t size, struct pbuf_manager *mgr, void *src){
  struct pbuf_ram *p;

  p = mem_malloc(MEM_ALIGN_SIZE(sizeof(struct pbuf_ram) + offset) +
                 MEM_ALIGN_SIZE(size));
  if (p == NULL) {
    UserPrint("pbuf_ram_alloc: mem_malloc failed!\n");
    return NULL;
  }

  p->next = NULL;
  p->payload = MEM_ALIGN((void*)((u8_t*)p + sizeof(struct pbuf_ram) + offset));
  p->manager = mgr;
  p->tot_len = size;
  p->len = size;
  p->ref = 1;
#ifdef LWIP_DEBUG
  p->magic = PBUF_RAM_MAGIC;
#endif

  /* if source data is specified, copy it to the payload */
  if(src!=NULL && size>0){
    memcpy(p->payload, src, size);
  }

  return (struct pbuf*)p;
}

void
pbuf_ram_realloc(struct pbuf *p, u16_t size){
  mem_realloc(p, ((u8_t*)p->payload - (u8_t*)p) + size);
  return;
}

void
pbuf_ram_free(struct pbuf *p){
#ifdef LWIP_DEBUG
  struct pbuf_ram *q = (struct pbuf_ram*)p;
  LWIP_ASSERT("pbuf_ram_free(): pbuf was not a ram pbuf!",
              (q->magic==PBUF_RAM_MAGIC));

  /* make faults more obvious */
  p->manager = NULL;
  p->payload = NULL;

#endif
	mem_free(p);
}

u8_t
pbuf_ram_header(struct pbuf *p, s16_t header_size){
  mem_ptr_t min, payload;

  LWIP_DEBUGF(PBUF_DEBUG, ("pbuf_ram_header(0x%p,%d) was called.\n",
                           p, (int)header_size));

  min = (mem_ptr_t)p + sizeof(struct pbuf_ram);
  payload = (mem_ptr_t)p->payload;

  if(payload - header_size >= min){
    p->payload = (u8_t*)p->payload - header_size;
    p->len += header_size;
    p->tot_len += header_size;
    return 0; /* success */
  }

  return 1; /* failure */
}

struct pbuf *
pbuf_ram_take(struct pbuf *p){
  return p; /* don't need to copy ram pbufs */
}
