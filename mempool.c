// Copyright 2023 luo-zeqi
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "mempool.h"

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>

#define XCHUNK_MEM(ch, idx, bs) (((char *) ((ch)+1)) + (idx*bs))
#define XCHUNK_BCOUNT 64

xmempool_t *xmempool_create(short blocksize) {
  xmempool_t *mp;

  mp = (xmempool_t *) malloc(sizeof(xmempool_t));
  if (mp == NULL)
    return NULL;

  xmempool_init(mp, blocksize);

  return mp;
}

void xmempool_init(xmempool_t *mp, short blocksize) {
  mp->blocksize = blocksize;
  mp->head = NULL;
}

void xmempool_destroy(xmempool_t *mp) {
  xmchunk_t *tmp;

  mp->blocksize = 0;
  while (mp->head != NULL) {
    tmp = mp->head->next;
    free(mp->head);
    mp->head = tmp;
  }
}

void *xmempool_alloc(xmempool_t *mp, int n) {
  xmchunk_t **chunk;
  uint64_t i, bits;
  uint64_t mask;

  if (mp == NULL || n > XCHUNK_BCOUNT)
    return NULL;

  mask = (UINT64_MAX >> (XCHUNK_BCOUNT - n));
  chunk = &mp->head;
  while (*chunk != NULL) {
    // This chunk has not enough memory.
    if ((*chunk)->usebits == UINT64_MAX
     || (*chunk)->freecount < n) {
      chunk = &((*chunk)->next);
      continue;
    }

    i = (*chunk)->freeindex;
    bits = ~((*chunk)->usebits >> i);
    while (i+n <= XCHUNK_BCOUNT) {
      if ((bits & mask) == mask) {
        // 将bit标记为已使用.
        (*chunk)->usebits |= mask << i;
        (*chunk)->freeindex = i+n;
        (*chunk)->freecount -= n;
        return XCHUNK_MEM(*chunk, i, mp->blocksize);
      }
      bits >>= 0x1;
      i++;
    }

    chunk = &((*chunk)->next);
  }

  *chunk = malloc(sizeof(xmchunk_t) + mp->blocksize * XCHUNK_BCOUNT);
  if (chunk == NULL)
    return NULL;

  (*chunk)->next = NULL;
  (*chunk)->freeindex = n;
  (*chunk)->freecount = XCHUNK_BCOUNT - n;
  (*chunk)->usebits |= mask;
  return XCHUNK_MEM(*chunk, 0, mp->blocksize);
}

void xmempool_free(xmempool_t *mp, void *mem, int n) {
  xmchunk_t *chunk;
  void *begin, *end;
  uint64_t idx;

  if (mp == NULL || n > XCHUNK_BCOUNT)
    return;

  idx = 0;
  chunk = mp->head;
  while (chunk != NULL) {
    begin = XCHUNK_MEM(chunk, 0, mp->blocksize);
    end = begin + XCHUNK_BCOUNT * mp->blocksize;
    if (mem < begin || mem >= end) {
      chunk = chunk->next;
      continue;
    }

    if (mem != begin)
      idx = (mem - begin) / mp->blocksize;

    chunk->usebits ^= (UINT64_MAX >> (XCHUNK_BCOUNT - n)) << idx;
    chunk->freecount += n;
    if (idx < chunk->freeindex)
      chunk->freeindex = idx;

    return;
  }
}