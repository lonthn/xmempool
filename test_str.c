//
// Created by zeqi luo on 2023/10/22.
//

#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <sys/time.h>
#include <stdlib.h>

#include "mempool.h"

#define ARR_LEN(arr, type) (sizeof(arr) / sizeof(type))

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

struct xstring {
  int block;
  char *ptr;
};

void xa();
void ma();

int main() {
  // 初始化
  (void *) (malloc(10000));

  xa();
  ma();
}

void xa() {
  xmempool_t mp;
  struct xstring strs[ARR_LEN(test_data, char *)];
  struct timeval t1, t2;

  gettimeofday(&t1, NULL);

  //
  xmempool_init(&mp, 16);

  for (int i = 0; i < ARR_LEN(test_data, char *); i++) {
    int block = (int) strlen(test_data[i]) / 16 + 1;
    strs[i].block = block;
    strs[i].ptr = xmempool_alloc(&mp, block);
    memcpy(strs[i].ptr, test_data[i], strlen(test_data[i]));
    assert(strs[i].ptr);
  }

  for (int i = 0; i < ARR_LEN(test_data, char *); i++) {
    xmempool_free(&mp, strs[i].ptr, strs[i].block);
  }

  gettimeofday(&t2, NULL);

  printf("xmalloc use time: %ld\n",
         (t2.tv_sec-t1.tv_sec) * 10000000
         + (t2.tv_usec-t1.tv_usec));

  xmempool_destroy(&mp);
}

void ma() {
  struct xstring strs[ARR_LEN(test_data, char *)];
  struct timeval t1, t2;

  gettimeofday(&t1, NULL);

  for (int i = 0; i < ARR_LEN(test_data, char *); i++) {
    strs[i].block = 0;
    strs[i].ptr = malloc(strlen(test_data[i]));
    memcpy(strs[i].ptr, test_data[i], strlen(test_data[i]));
    assert(strs[i].ptr);
  }

  for (int i = 0; i < ARR_LEN(test_data, char *); i++) {
    free((void *) strs[i].ptr);
  }

  gettimeofday(&t2, NULL);

  printf("malloc use time: %ld\n",
         (t2.tv_sec-t1.tv_sec) * 10000000
         + (t2.tv_usec-t1.tv_usec));
}