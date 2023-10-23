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

#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <sys/time.h>
#include <stdlib.h>

#include "mempool.h"

#define ARR_LEN(arr, type) (sizeof(arr) / sizeof(type))

#define USE_XMEMPOOL
#define XMEM_BLOCK_SIZE 16

#ifdef USE_XMEMPOOL
xmempool_t mp;
#endif

// The Data Generate by Chat-GPT.
const char *test_data[] = {
    "Hello World!",
    "Live and let live.",
    "Time is money.",
    "Practice makes perfect.",
    "All is fair in love and war.",
    "Actions speak louder than words.",
    "Beauty is in the eye of the beholder.",
    "Knowledge is power.",
    "Where there's smoke, there's fire.",
    "You can't have your cake and eat it too.",
    "It takes two to tango.",
    "Every cloud has a silver lining.",
    "The early bird catches the worm.",
    "Two heads are better than one.",
    "Look before you leap.",
    "A picture is worth a thousand words.",
    "Rome wasn't built in a day.",
    "Practice what you preach.",
    "When in Rome, do as the Romans do.",
    "Better late than never.",
    "Don't count your chickens before they hatch.",
    "When the going gets tough, the tough get going.",
    "All that glitters is not gold.",
    "The grass is always greener on the other side.",
    "No pain, no gain.",
    "Curiosity killed the cat.",
    "Haste makes waste.",
    "Don't put all your eggs in one basket.",
    "Necessity is the mother of invention.",
    "What goes up must come down.",
    "A bird in the hand is worth two in the bush.",
    "The pen is mightier than the sword.",
    "Actions speak louder than words.",
    "Love conquers all.",
    "Time heals all wounds.",
    "Beauty is only skin deep.",
    "A watched pot never boils.",
    "Better safe than sorry.",
    "Good things come to those who wait.",
    "Laughter is the best medicine.",
    "Where there's a will, there's a way.",
};

int64_t order_alloc() {
  int i;
  struct xstring {
    int block;
    char *ptr;
  } strs[ARR_LEN(test_data, char *)];

  struct timeval t1, t2;

  gettimeofday(&t1, NULL);

  for (i = 0; i < ARR_LEN(test_data, char *); i++) {
#ifdef USE_XMEMPOOL
    int block = (int) strlen(test_data[i]) / XMEM_BLOCK_SIZE + 1;
    strs[i].block = block;
    strs[i].ptr = xmempool_alloc(&mp, block);
#else
    strs[i].ptr = malloc(strlen(test_data[i]));
#endif
    assert(strs[i].ptr);

    memcpy(strs[i].ptr, test_data[i], strlen(test_data[i]));
  }

  for (i = 0; i < ARR_LEN(test_data, char *); i++) {
#ifdef USE_XMEMPOOL
    xmempool_free(&mp, strs[i].ptr, strs[i].block);
#else
    free(strs[i].ptr);
#endif
  }

  gettimeofday(&t2, NULL);

  return (t2.tv_sec-t1.tv_sec) * 1000000 + (t2.tv_usec - t1.tv_usec);
}

int64_t random_alloc() {
  typedef struct xstr {
    struct xstr *next;
    int block;
  } xstr_t;

  int i, j;
  int block;
  long datanum, allocflag, idx, count;
  struct timeval t1, t2;
  xstr_t head, *curr, *tmp, *tmp2;

  gettimeofday(&t1, NULL);

  curr = &head;
  count = 0;
  datanum = ARR_LEN(test_data, char *);
  for (i = 0; i < datanum * 2; i++) {
    allocflag = random() % 2;
    if (allocflag) {
      idx = random() % datanum;
#ifdef USE_XMEMPOOL
      block = (int) (strlen(test_data[idx]) + sizeof(xstr_t)) / XMEM_BLOCK_SIZE + 1;
      tmp = (xstr_t *) xmempool_alloc(&mp, block);
#else
      tmp = (xstr_t *) malloc(strlen(test_data[idx]) + sizeof(xstr_t));
#endif
      tmp->next = NULL;
      tmp->block = block;
      curr->next = tmp;
      curr = tmp;
      count++;
    } else {
      if (count == 0) continue;
      j = 0;
      idx = random() % count;
      tmp = &head;
      tmp2 = head.next;
      while (j++ != idx) {
        tmp = tmp2;
        tmp2 = tmp2->next;
      }
      tmp->next = tmp2->next;
      if (tmp2 == curr)
        curr = tmp;
#ifdef USE_XMEMPOOL
      xmempool_free(&mp, tmp2, tmp2->block);
#else
      free(tmp2);
#endif
      count--;
    }
  }

  curr = head.next;
  while (curr != NULL) {
    tmp = curr->next;
#ifdef USE_XMEMPOOL
    xmempool_free(&mp, curr, curr->block);
#else
    free(curr);
#endif
    curr = tmp;
  }

  gettimeofday(&t2, NULL);

  return (t2.tv_sec-t1.tv_sec) * 1000000 + (t2.tv_usec - t1.tv_usec);
}

int main() {
  int i;
  int count = 10000;
  int64_t tuse = 0;

  //
#ifdef USE_XMEMPOOL
  xmempool_init(&mp, XMEM_BLOCK_SIZE);
#endif

  for (i = 0; i < count; i++)
    tuse += order_alloc();

  printf("Average elapsed time: %ld\n", tuse / (int64_t) count);

#ifdef USE_XMEMPOOL
  xmempool_destroy(&mp);
#endif
}
