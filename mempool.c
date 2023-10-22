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
  uint64_t i, found, bits;

  if (mp == NULL || n > 64)
    return NULL;

  chunk = &mp->head;
  while (*chunk != NULL) {
    // The chunk has not memory
    if ((*chunk)->usebits == UINT64_MAX) {
      chunk = &((*chunk)->next);
      continue;
    }

    i = 0, found = 0;
    bits = ~(*chunk)->usebits;
    while (i < 64) {
      if (bits & 0x1)
        found++;
      else
        found = 0;
      if (found == n) {
        // 将bit标记为已使用.
        (*chunk)->usebits |= (UINT64_MAX >> (64 - n)) << (i + 1 - n);
        return XCHUNK_MEM(*chunk, (i + 1 - n), mp->blocksize);
      }
      bits >>= 1;
      i++;
    }

    chunk = &((*chunk)->next);
  }

  *chunk = malloc(sizeof(xmchunk_t) + mp->blocksize * 64);
  if (chunk == NULL)
    return NULL;

  (*chunk)->next = NULL;
  (*chunk)->usebits |= (UINT64_MAX >> (64 - n));
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
      return;
    }
    chunk = chunk->next;
  }
}

//#pragma pack()