#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <stdatomic.h>

#include "cstr_t.h"

volatile atomic_flag alock = ATOMIC_FLAG_INIT;

#define a_lock()\
({while (atomic_flag_test_and_set(&alock));})

#define a_unlock()\
({atomic_flag_clear(&(alock));})

#define cstr_cat_s(s, a) do{\
  a_lock();\
  cstr_cat((s), a);\
  a_unlock();} while (0)

#define cstr_grab_s(from, to) do{\
  a_lock();\
  (to) = cstr_grab(from);\
  a_unlock();} while (0)


static cstring cmp(cstring t) {
  CSTR_LITERAL(hello, "aaaaaaaa");
  CSTR_BUFFER(ret);
  cstr_cat(ret, cstr_equal(hello, t) ? "equal" : "not equal");
  return cstr_grab(CSTR_S(ret));
}

static void test_cstr(void *buf) {
  cstr_buffer *a = (cstr_buffer *)buf;
  cstr_cat_s((*a), "aaaa");
  //printf("1. %ld %s\n", pthread_self(), (*a)->str->cstr);
  cstr_grab_s(CSTR_S(*a), (*a)->str);
  //printf("2. %ld %s\n", pthread_self(), (*a)->str->cstr);
  cstr_cat_s((*a), "aaaa");
  //printf("3. %ld %s\n", pthread_self(), (*a)->str->cstr);
}

static void equal_or_not(cstr_buffer a){  
    CSTR_LITERAL(hello, "aaaaaaaaaaaaaaaa");
    if (!cstr_equal(hello, CSTR_S(a))) {
      printf("not equal : ");
      printf("%s\n", CSTR_S(a)->cstr);
    } //else {printf("equal\n\n");}
}

static void test0(void) {
#ifdef times
#undef times
#endif
#define times 1
  for (int i = 0; i < times; i++) {
    CSTR_BUFFER(a);
    //CSTR_BUFFER(b);
    pthread_t thread_0, thread_1;
    pthread_create(&thread_1, NULL, (void *)test_cstr, (void *)a);
    pthread_create(&thread_0, NULL, (void *)test_cstr, (void *)a);
    pthread_join(thread_0, NULL);
    pthread_join(thread_1, NULL);
    
    equal_or_not(a);
    //equal_or_not(b);
    CSTR_CLOSE(a);
  }
#undef times 
}

int main(int argc, char *argv[]) {
  test0();
  return 0; 
}