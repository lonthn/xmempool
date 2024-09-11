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

#ifndef XDOC_MEMPOOL_H
#define XDOC_MEMPOOL_H

#include <stdint.h>

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

typedef struct xmchunk {
  uint8_t flag;
  struct xmchunk *fnext;
  struct xmchunk *next;
  uint64_t usebits;
  uint8_t sign;
} xmchunk_t;

typedef struct xmempool {
  short count;
  short *blocksize;
  struct xmchunk *head;
  struct xmchunk **freechunks;
  unsigned int nelts;
} xmempool_t;

/// @brief 创建一个内存池, 并指定申请的单个内存大小.
/// 支持提供多个不同等级的大小，在申请内存时自适应选择
/// 合适的大小进行分配.
xmempool_t *xmempool_create(int count, ...);

/// 初始化一个内存池, 用户申请的单个内存大小固定为blocksize.
void xmempool_init(xmempool_t *mp, int count, ...);

/// 销毁一个内存池.
void xmempool_destroy(xmempool_t *mp);

/// 向内存池申请内存, 内存大小为 n * blocksize
void *xmempool_alloc(xmempool_t *mp, int size);

/// 归还内存, n 必须与申请时相同.
void xmempool_free(xmempool_t *mp, void *mem);

#endif //XDOC_MEMPOOL_H
