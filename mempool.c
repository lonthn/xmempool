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
#include <strings.h>
#include <stdio.h>

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
  mp->chunknum = 0;
  LLIST_INIT(&mp->lhandle);
}

void xmempool_destroy(xmempool_t *mp) {
  xmchunk_t *item, *tmp;

  mp->blocksize = 0;
  item = mp->lhandle.next;
  while (item != (xmchunk_t *)&(mp->lhandle)) {
    tmp = item;
    item = item->next;
    free(tmp);
  }
}

// 单字节索引查找表
static const uint8_t idx_table[] = {
  0, 1, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
  6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
  7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
  7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
  7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
  7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
  8
};

int xmem_ffs_u64(uint64_t i) {
  if (i == 0) return 0;

  uint64_t x = i & -i;
  if (x < 0x81) return idx_table[x];
  if (x < 0xFF) return 8;

  if (x < 0xFFFF) return idx_table[x >> 8] + 8;
  if (x < 0xFFFFFF) return idx_table[x >> 16] + 16;
  if (x < 0xFFFFFFFF) return idx_table[x >> 24] + 24;
  if (x < 0xFFFFFFFFFF) return idx_table[x >> 32] + 32;
  if (x < 0xFFFFFFFFFFFF) return idx_table[x >> 40] + 40;
  if (x < 0xFFFFFFFFFFFFFF) return idx_table[x >> 48] + 48;

  return idx_table[x >> 56] + 56;
}

//void *xmempool_alloc(xmempool_t *mp, int n) {
//  xmchunk_t **chunk;
//  uint64_t i, bits;
//  uint64_t mask, maskres;
//
//  if (mp == NULL || n > XCHUNK_BCOUNT)
//    return NULL;
//
//  mask = (UINT64_MAX >> (XCHUNK_BCOUNT - n));
//  chunk = &mp->head;
//  while (*chunk != NULL) {
//    // This chunk has not enough memory!
//    if ((*chunk)->usebits == UINT64_MAX
//     || (*chunk)->freecount < n
//     || (*chunk)->freeindex >= XCHUNK_BCOUNT) {
//      chunk = &((*chunk)->next);
//      continue;
//    }
//
//    i = (*chunk)->freeindex;
//    // It's not easy to find 0, So let's find 1 instead.
//    bits = ~((*chunk)->usebits);
//
//    while (i == -1 || i+n < XCHUNK_BCOUNT) {
//      maskres = ((bits >> i)) & mask;
//      if (maskres == mask) {
//        // 将bit标记为已使用.
//        (*chunk)->usebits |= mask << i;
//        (*chunk)->freeindex = i+n;
//        (*chunk)->freecount -= n;
//        return XCHUNK_MEM(*chunk, i, mp->blocksize);
//      }
//
//      bits ^= maskres << i;
//
//      i = xmem_ffs_u64(bits) - 1;
//    }
//
//    chunk = &((*chunk)->next);
//  }
//
//  *chunk = malloc(sizeof(xmchunk_t) + mp->blocksize * XCHUNK_BCOUNT);
//  if (chunk == NULL)
//    return NULL;
//
//  (*chunk)->next = NULL;
//  (*chunk)->freeindex = n;
//  (*chunk)->freecount = XCHUNK_BCOUNT - n;
//  (*chunk)->usebits |= mask;
//  return XCHUNK_MEM(*chunk, 0, mp->blocksize);
//}

void *xmempool_alloc(xmempool_t *mp, int n) {
  xmchunk_t *chunk, *tmp;
  uint64_t i, bits;
  uint64_t mask, maskres;

  if (mp == NULL || n > XCHUNK_BCOUNT)
    return NULL;

  mask = (UINT64_MAX >> (XCHUNK_BCOUNT - n));
  chunk = mp->lhandle.next;
  while (chunk != &(mp->lhandle)) {
    // This chunk has not enough memory!
    if (chunk->usebits == UINT64_MAX
     || chunk->freecount < n) {
      break;
    }

//    if (chunk->freeindex >= XCHUNK_BCOUNT) {
//      chunk = chunk->next;
//      continue;
//    }

    // It's not easy to find 0, So let's find 1 instead.
    bits = ~(chunk->usebits);
    i = xmem_ffs_u64(bits) - 1;//chunk->freeindex;

    while (i == -1 || i+n <= XCHUNK_BCOUNT) {
      maskres = ((bits >> i)) & mask;
      if (maskres == mask) {
        // 将bit标记为已使用.
        chunk->usebits |= mask << i;
        chunk->freeindex = i+n;
        chunk->freecount -= n;

        // reshuffle by free count.
        tmp = chunk;
        while (tmp->next != &(mp->lhandle)
            && chunk->freecount < tmp->next->freecount) {
          tmp = tmp->next;
        }
        if (tmp != chunk) {
          LLIST_REMOVE(chunk);
          LLIST_INSERT(tmp, chunk);
        }
        return XCHUNK_MEM(chunk, i, mp->blocksize);
      }

      bits ^= maskres << i;

      i = xmem_ffs_u64(bits) - 1;
    }

    chunk = (chunk->next);
  }

  chunk = malloc(sizeof(xmchunk_t) + mp->blocksize * XCHUNK_BCOUNT);
  if (chunk == NULL)
    return NULL;

  mp->chunknum++;

  chunk->freeindex = n;
  chunk->freecount = XCHUNK_BCOUNT - n;
  chunk->usebits |= mask;
  LLIST_INSERT(&mp->lhandle, chunk);
  return XCHUNK_MEM(chunk, 0, mp->blocksize);
}

void xmempool_free(xmempool_t *mp, void *mem, int n) {
  xmchunk_t *chunk, *tmp;
  void *begin, *end;
  uint64_t idx;

  if (mp == NULL || n > XCHUNK_BCOUNT)
    return;

  idx = 0;
  chunk = mp->lhandle.next;
  while (chunk != &(mp->lhandle)) {
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

    tmp = chunk->prev;
    while (tmp != &(mp->lhandle)
        && tmp->freecount < chunk->freecount) {
      tmp = tmp->prev;
    }
    if (tmp != chunk->prev) {
      LLIST_REMOVE(chunk);
      LLIST_INSERT(tmp, chunk);
    }
    return;
  }
}
