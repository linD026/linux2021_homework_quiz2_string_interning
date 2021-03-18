#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>

#include "cstr_t.h"

volatile atomic_flag alock = ATOMIC_FLAG_INIT;

#define a_lock()                                                               \
  ({                                                                           \
    while (atomic_flag_test_and_set(&alock))                                   \
      ;                                                                        \
  })

#define a_unlock() ({ atomic_flag_clear(&(alock)); })

#define cstr_cat_s(s, a)                                                       \
  do {                                                                         \
    a_lock();                                                                  \
    cstr_cat((s), a);                                                          \
    a_unlock();                                                                \
  } while (0)

#define cstr_grab_s(from, to)                                                  \
  do {                                                                         \
    a_lock();                                                                  \
    cstring temp = cstr_grab(from);                                            \
    (to) = temp;                                                               \
    a_unlock();                                                                \
  } while (0)

#define cstr_clone_s(from, to)                                                 \
  do {                                                                         \
    a_lock();                                                                  \
    (to) = cstr_clone((from)->cstr, ((from)->hash_size));                      \
    a_unlock();                                                                \
  } while (0)

#define cstr_release_s(target)                                                 \
  do {                                                                         \
    a_lock();                                                                  \
    cstr_release((target));                                                    \
    a_unlock();                                                                \
  } while (0)

static cstring cmp(cstring t) {
  CSTR_LITERAL(hello, "aaaaaaaa");
  CSTR_BUFFER(ret);
  cstr_cat(ret, cstr_equal(hello, t) ? "equal" : "not equal");
  return cstr_grab(CSTR_S(ret));
}

static void test_cstr(void *buf) {
  cstr_buffer *a = (cstr_buffer *)buf;
  cstr_cat_s((*a), "aaaa");
  // printf("1. %ld %s\n", pthread_self(), (*a)->str->cstr);
  cstr_grab_s(CSTR_S(*a), (*a)->str);
  // printf("2. %ld %s\n", pthread_self(), (*a)->str->cstr);
  cstr_cat_s((*a), "aaaa");
  // printf("3. %ld %s\n", pthread_self(), (*a)->str->cstr);
}

static void equal_or_not(cstr_buffer a) {
  CSTR_LITERAL(hello, "aaaaaaaaaaaaaaaa");
  if (!cstr_equal(hello, CSTR_S(a))) {
    printf("not equal : ");
    printf("%s\n", CSTR_S(a)->cstr);
  } // else {printf("equal\n\n");}
}

static void test0(void) {
#ifdef times
#undef times
#endif
#define times 1
  for (int i = 0; i < times; i++) {
    CSTR_BUFFER(a);
    // CSTR_BUFFER(b);
    pthread_t thread_0, thread_1;
    pthread_create(&thread_1, NULL, (void *)test_cstr, (void *)a);
    pthread_create(&thread_0, NULL, (void *)test_cstr, (void *)a);
    pthread_join(thread_0, NULL);
    pthread_join(thread_1, NULL);

    equal_or_not(a);
    // equal_or_not(b);
    CSTR_CLOSE(a);
  }
#undef times
}

//////////////////////////////////////////////////////////////////////////////////////

static void test_ref_create(void *buf) {
  cstr_buffer *a = (cstr_buffer *)buf;
  cstring temp;
  cstring temp_1;
  // string size 33
  cstr_grab_s(CSTR_S(*a), (*a)->str);
  //(*a)->str = temp;
  // printf("a = temp grab 1 ref: %d\n", (*a)->str->ref);
  cstr_grab_s(CSTR_S(*a), (*a)->str);
  // printf("temp_1   grab 2 ref: %d\n", temp_1->ref);
  //(*a)->str = temp_1;
}

static void test_ref_del(void *buf) {
  cstr_buffer *a = (cstr_buffer *)buf;
  cstr_release_s((*a)->str);
  //printf("a = temp grab 1 ref: %d\n", (*a)->str->ref);
  //cstr_release_s((*a)->str);
  //printf("a = temp grab 1 ref: %d\n", (*a)->str->ref);
}

static void test_ref_create_ns(void *buf) {
  cstr_buffer *a = (cstr_buffer *)buf;
  cstring temp;
  cstring temp_1;
  // string size 33
  (*a)->str = cstr_grab(CSTR_S(*a));
  //(*a)->str = temp;
  //printf("a = temp grab 1 ref: %d\n", (*a)->str->ref);
  (*a)->str = cstr_grab(CSTR_S(*a));
  //printf("temp_1   grab 2 ref: %d\n", (*a)->str->ref);
  //(*a)->str = temp_1;
}


static void test_ref_del_ns(void *buf) {
  cstr_buffer *a = (cstr_buffer *)buf;
  cstr_release((*a)->str);
  //printf("a = temp grab 1 ref: %d\n", (*a)->str->ref);
  //cstr_release_s((*a)->str);
  //printf("a = temp grab 1 ref: %d\n", (*a)->str->ref);
}


/**
 *  ref and mutilipe thread
 *  more than two
 */
static void test1(void) {
#ifdef times
#undef times
#endif
#define times 1
#define thread_n 10
#define ans (thread_n/2 + 1)
  for (int i = 0; i < times; i++) {
    CSTR_BUFFER(a);

    cstr_cat_s(a, "CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC");
    pthread_t thread[thread_n];

    //printf("ref: %d, type %d\n", a->str->ref, a->str->type);

    int j;
    for (j = 0; j < (thread_n/2); j++)
      pthread_create(&thread[j], NULL, (void *)test_ref_create_ns, (void *)a);

    //sleep(0.5);
    //printf("ref: %d, type %d\n", a->str->ref, a->str->type);


    for (;j < thread_n;j++)
      pthread_create(&thread[j], NULL, (void *)test_ref_del_ns, (void *)a);


    for (j = 0; j < thread_n; j++)
      pthread_join(thread[j], NULL);

    // printf("%s\n", a->str->cstr);
    //printf("ref: %d, type %d\n", a->str->ref, a->str->type);
    if (!(ans == a->str->ref)) {
      printf("ans: %d, ref: %d, type : %d (need = 0)\n", ans, a->str->ref, a->str->type);
      assert(0);
    }
    CSTR_CLOSE(a);
  }
#undef times
}

/////////////////////////////////////////////////////////////////////////////////////

int main(int argc, char *argv[]) {
  test1();
  return 0;
}