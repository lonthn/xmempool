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
#include <string.h>
#include <stdarg.h>
#include <strings.h>

#define XCHUNK_MEM(ch, idx, bs) (((char *) ((ch)+1)) + ((idx)*(bs)))
#define XCHUNK_BCOUNT 64

xmempool_t *xmempool_create(int count, ...) {
  xmempool_t *mp;

  mp = (xmempool_t *) malloc(sizeof(xmempool_t));
  if (mp == NULL)
    return NULL;

  va_list args;
  va_start(args, count);
  xmempool_init(mp, count, args);
  va_end(args);
  return mp;
}

void xmempool_init(xmempool_t *mp, int count, ...) {
  mp->head = NULL;
  mp->count = (short) count;
  mp->blocksize = malloc(sizeof(short) * count);
  mp->freechunks = malloc(sizeof(struct xchunk_t *) * count);

  va_list args;
  va_start(args, count);

  for (int i = 0; i < count; i++) {
    mp->blocksize[i] = va_arg(args, int);
    mp->freechunks[i] = NULL;
  }

  va_end(args);
}

void xmempool_destroy(xmempool_t *mp) {
  xmchunk_t *tmp;

  if (mp->blocksize != NULL) {
    free(mp->blocksize);
    free(mp->freechunks);
  }

  while (mp->head != NULL) {
    tmp = mp->head->next;
    free(mp->head);
    mp->head = tmp;
  }
}

int xmem__ffs_u64(uint64_t i) {
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
      8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
      8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
      8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
      8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
      8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
      8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
      8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
      8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8
  };

  if (i == 0) return 0;

  uint64_t x = i & -i;

  if (x < 0xFF) return idx_table[x];
  if (x < 0xFFFF) return idx_table[x >> 8] + 8;
  if (x < 0xFFFFFF) return idx_table[x >> 16] + 16;
  if (x < 0xFFFFFFFF) return idx_table[x >> 24] + 24;
  if (x < 0xFFFFFFFFFF) return idx_table[x >> 32] + 32;
  if (x < 0xFFFFFFFFFFFF) return idx_table[x >> 40] + 40;
  if (x < 0xFFFFFFFFFFFFFF) return idx_table[x >> 48] + 48;

  return idx_table[x >> 56] + 56;
}

xmchunk_t **xmem__select_chunk(xmempool_t *mp, int size, int *bsidx) {
  if (size > mp->blocksize[mp->count - 1])
    return NULL;

  for (short i = 0; i < mp->count; i++) {
    if (size <= mp->blocksize[i]) {
      *bsidx = i;
      return &(mp->freechunks[i]);
    }
  }

  // Unreached code.
  return NULL;
}

#define CHUNK_FLAG (0xAA)

#define CHUNK_SIGN(chunk, flag, sign) \
  (sign) = ((((uint64_t)(chunk)) & 0xFF) ^ (flag))

#define CHUNK_CHECK(chunk, flag, sign) \
  (((flag) == CHUNK_FLAG)              \
  && (((((uint64_t)(chunk)) & 0xFF) ^ CHUNK_FLAG) == (sign)))

void *xmempool_alloc(xmempool_t *mp, int size) {
  xmchunk_t **chunk, *use;
  uint64_t i, bits;
  int bsidx, blocksize;
  char *mem;

  if (mp == NULL || mp->count == 0)
    return NULL;

  chunk = xmem__select_chunk(mp, size, &bsidx); //&mp->head;
  if (chunk == NULL)
    return NULL;

  i = 0;
  blocksize = 1 + mp->blocksize[bsidx];

  if (*chunk != NULL) {
    // It's not easy to find 0, So let's find 1 instead.
    bits = ~((*chunk)->usebits);
    if ((i = xmem__ffs_u64(bits) - 1) == -1) {
      // assert(false);
    }

    use = *chunk;
    // 将bit标记为已使用.
    (*chunk)->usebits |= (uint64_t) 0x1 << i;
    // This chunk has not enough memory!
    // We need remove out the chunk from freechunks.
    if ((*chunk)->usebits == UINT64_MAX) {
      *chunk = (*chunk)->fnext;
    }
  } else {
    use = *chunk = malloc(sizeof(xmchunk_t) + blocksize * XCHUNK_BCOUNT);
    if (*chunk == NULL)
      return NULL;

    (*chunk)->usebits = 0x1;
    (*chunk)->flag = CHUNK_FLAG;
    (*chunk)->fnext = NULL;
    (*chunk)->next = mp->head;
    mp->head = *chunk;
    CHUNK_SIGN(*chunk, (*chunk)->flag, (*chunk)->sign);
  }

  mem = XCHUNK_MEM(use, i, blocksize);
  *mem = (char) i;
  return mem + 1;
}

void xmempool_free(xmempool_t *mp, void *mem) {
  xmchunk_t *chunk;
  uint64_t bidx;
  char *block;
  int i, blocksize;

  if (mp == NULL || mem == NULL)
    return;

  block = (char *) mem - 1;

  bidx = (uint64_t) *block;
  for (i = 0; i < mp->count; i++) {
    blocksize = 1 + mp->blocksize[i];
    chunk = (xmchunk_t *) (block - (bidx * blocksize + sizeof(xmchunk_t)));
    if (CHUNK_CHECK(chunk, chunk->flag, chunk->sign)) {
      if (chunk->usebits == UINT64_MAX) {
        chunk->fnext = mp->freechunks[i];
        mp->freechunks[i] = chunk;
      }

      chunk->usebits &= ~(0x1 << bidx);
      return;
    }
  }
  // Note: 如果代码走到这里，说明程序内存已经遭到破坏！
}