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

// xmempool 是一个内存池, 它被分为很多个 chunk 并通过 next 指针
// 链接起来, 每个 chunk 可分配 64 个内存块, 具体大小由用户初始化时
// 提供的 blocksize 决定; 在申请时可以获得 n*blocksize 个字节且
// 连续的内存.
//
// 下面是内存池的大致结构: Chunk chain
//
//  [next chunk ptr | [0,1,1,1,...] | 64 blocks ]
//   └--> [next chunk ptr | [1,1,1,0,...] | 64 blocks ]
//          └--> [next chunk ptr | [1,0,0,0,...] | 64 blocks ]
//                        ...

#ifndef XDOC_MEMPOOL_H
#define XDOC_MEMPOOL_H

#include "llist.h"

#include <stdint.h>

typedef struct xmchunk {
  DECLARE_LLIST_NODE(struct xmchunk)
  uint64_t usebits;
  uint8_t freecount;
  uint8_t freeindex;
} xmchunk_t;

typedef struct xmempool {
  short blocksize;
  int chunknum;
//  struct xmchunk *head;
  //llnode_t handle;
  xmchunk_t lhandle;
} xmempool_t;

/// 创建一个内存池, 用户申请的单个内存大小固定为blocksize.
xmempool_t *xmempool_create(short blocksize);

/// 初始化一个内存池, 用户申请的单个内存大小固定为blocksize.
void xmempool_init(xmempool_t *mp, short blocksize);

/// 销毁一个内存池.
void xmempool_destroy(xmempool_t *mp);

/// 向内存池申请内存, 内存大小为 n * blocksize
void *xmempool_alloc(xmempool_t *mp, int n);

/// 归还内存, n 必须与申请时相同.
void xmempool_free(xmempool_t *mp, void *mem, int n);

#endif //XDOC_MEMPOOL_H
