//
// Created by zeqi luo on 2023/10/21.
//

#include "mempool.h"

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>

#define XCHUNK_MEM(ch, idx, bs) (((char *) ((ch)+1)) + (idx*bs))

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

  if (mp == NULL || n > 64)
    return NULL;

  mask = (UINT64_MAX >> (64 - n));
  chunk = &mp->head;
  while (*chunk != NULL) {
    // The chunk has not memory
    if ((*chunk)->usebits == UINT64_MAX) {
      chunk = &((*chunk)->next);
      continue;
    }

    i = (*chunk)->freeidx;
    bits = ~((*chunk)->usebits >> i);
    while (i+n <= 64) {
      if ((bits & mask) == mask) {
        // 将bit标记为已使用.
        (*chunk)->usebits |= mask << i;
        (*chunk)->freeidx = i+n;
        return XCHUNK_MEM(*chunk, i, mp->blocksize);
      }
      bits >>= 0x1;
      i++;
    }

    chunk = &((*chunk)->next);
  }

  *chunk = malloc(sizeof(xmchunk_t) + mp->blocksize * 64);
  if (chunk == NULL)
    return NULL;

  (*chunk)->next = NULL;
  (*chunk)->freeidx = n;
  (*chunk)->usebits |= mask;
  return XCHUNK_MEM(*chunk, 0, mp->blocksize);
}

void xmempool_free(xmempool_t *mp, void *mem, int n) {
  xmchunk_t *chunk;
  void *begin, *end;
  uint64_t idx;

  if (mp == NULL || n > 64)
    return;

  chunk = mp->head;
  while (chunk != NULL) {
    begin = XCHUNK_MEM(chunk, 0, mp->blocksize);
    end = begin + 64 * mp->blocksize;
    if (mem >= begin && mem < end) {
      idx = mem == begin
            ? 0 : (mem - begin) / mp->blocksize;
      chunk->usebits ^= (UINT64_MAX >> (64 - n)) << idx;
      if (idx < chunk->freeidx)
        chunk->freeidx = idx;
      return;
    }
    chunk = chunk->next;
  }
}

//#pragma pack()