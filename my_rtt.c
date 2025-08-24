// bthelmer Braden T Helmer
// my_rtt.c
// ~~~~~~~~
// Previous RTT implementation from HW1
#include "my_mpi.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Message sizes defines
#define S_32KB (1U << 15)
#define S_64KB (1U << 16)
#define S_128KB (1U << 17)
#define S_256KB (1U << 18)
#define S_512KB (1U << 19)
#define S_1MB (1U << 20)
#define S_2MB (1U << 21)
#define S_COUNT 7
#define ONE_BILLION 1e9
#define INTS_FROM_SIZE(SIZE) SIZE / sizeof(int32_t)

// Message sizes array
static const unsigned sizes[S_COUNT] = {S_32KB,  S_64KB, S_128KB, S_256KB,
                                        S_512KB, S_1MB,  S_2MB};

// Message array declarations
static int32_t a[INTS_FROM_SIZE(S_32KB)];
static int32_t b[INTS_FROM_SIZE(S_64KB)];
static int32_t c[INTS_FROM_SIZE(S_128KB)];
static int32_t e[INTS_FROM_SIZE(S_512KB)];
static int32_t d[INTS_FROM_SIZE(S_256KB)];
static int32_t f[INTS_FROM_SIZE(S_1MB)];
static int32_t g[INTS_FROM_SIZE(S_2MB)];

// Message pointer array
static int32_t *messages[S_COUNT] = {a, b, c, d, e, f, g};
// Value to fill message arrays with.
static const int32_t message_int = 0;

// Message Array initialization, only called for senders.
void init_messages() {
  for (unsigned size = 0; size < S_COUNT; size++) {
    memset(messages[size], message_int, INTS_FROM_SIZE(sizes[size]));
  }
}

// Options for printing.
typedef enum { INTER, INTRA } print_opt;

// Setter for printing option
static void set_print_opt(const char *arg, print_opt *opt) {
  if (!strcmp("inter", arg))
    *opt = INTER;
  if (!strcmp("intra", arg))
    *opt = INTRA;
  assert(*opt == INTER || *opt == INTRA);
}

int main(int argc, char **argv) {

  MPI_Init(&argc, &argv);
  int rank, size;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  const int half_size = size / 2;

// Logic for internode communications (Cases 1, 2, 3)
#ifndef CASE4

  int is_sender = rank < half_size;
  int is_receiver = !is_sender;
  int sender = rank - (!(is_sender)*half_size);
  int receiver = rank + (!(is_receiver)*half_size);

  // Sender only setup
  if (is_sender)
    init_messages();

  // Outer 100 times loop
  for (unsigned iter = 0; iter < 100; iter++) {

    // Inner iteration over different message sizes.
    for (unsigned curr_size = 0; curr_size < S_COUNT; curr_size++) {

      // Cache  integer count and pointer to message.
      const int int_count = INTS_FROM_SIZE(sizes[curr_size]);
      int32_t *const message_ptr = messages[curr_size];

      if (is_sender) {
        struct timespec start, end;
        clock_gettime(CLOCK_REALTIME, &start);
        MPI_Send(message_ptr, int_count, MPI_INT32_T, receiver, 0,
                 MPI_COMM_WORLD);
        MPI_Recv(message_ptr, int_count, MPI_INT32_T, receiver, 0,
                 MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        clock_gettime(CLOCK_REALTIME, &end);
        // Write latency to stdout : <time in nano seconds> / 1,000,000
        printf(curr_size == S_COUNT - 1 ? "%e\n" : "%e,",
               (double)((end.tv_nsec - start.tv_nsec) / (ONE_BILLION)));
      } else {
        MPI_Recv(message_ptr, int_count, MPI_INT32_T, sender, 0, MPI_COMM_WORLD,
                 MPI_STATUS_IGNORE);
        MPI_Send(message_ptr, int_count, MPI_INT32_T, sender, 0,
                 MPI_COMM_WORLD);
      }
    }
  }
  puts("");
#endif

// Logic for case 4, inter and intra node communications.
#ifdef CASE4

  assert(argc > 1 && "Must enter a print option (inter || intra)\n");
  print_opt opt;
  set_print_opt(argv[1], &opt);

  // Internode flags
  int is_internode_sender = (rank < (half_size / 2));
  int is_internode_receiver =
      ((rank - half_size) < (half_size / 2)) && ((rank - half_size) >= 0);

  // Intranode flags
  int is_intranode_sender = (abs(half_size - rank) == 2);
  int is_intranode_receiver =
      (!is_internode_sender && !is_intranode_sender && !is_internode_receiver);

  // Intranode Process Ids
  int internode_sender = rank - (!(is_internode_sender)*half_size);
  int internode_receiver = rank + (!(is_internode_receiver)*half_size);

  // Intranode process ids
  int intranode_sender = rank - !(is_intranode_sender);
  int intranode_receiver = rank + !(is_intranode_receiver);

  if (is_internode_sender || is_intranode_sender) {
    init_messages();
  }

  // Outer 100 iteration loop
  for (unsigned iter = 0; iter < 100; iter++) {
    // Inner iteration over differing message sizes.
    for (unsigned curr_size = 0; curr_size < S_COUNT; curr_size++) {

      // Cache  integer count and pointer to message.
      const int int_count = INTS_FROM_SIZE(sizes[curr_size]);
      int32_t *const message_ptr = messages[curr_size];

      if (is_internode_sender) {
        struct timespec start, end;
        clock_gettime(CLOCK_REALTIME, &start);
        MPI_Send(message_ptr, int_count, MPI_INT32_T, internode_receiver, 0,
                 MPI_COMM_WORLD);
        MPI_Recv(message_ptr, int_count, MPI_INT32_T, internode_receiver, 0,
                 MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        clock_gettime(CLOCK_REALTIME, &end);
        if (opt == INTER) {
          printf(curr_size == S_COUNT - 1 ? "%e\n" : "%e,",
                 (double)((end.tv_nsec - start.tv_nsec) / (ONE_BILLION)));
        }
      } else if (is_internode_receiver) {
        MPI_Recv(message_ptr, int_count, MPI_INT32_T, internode_sender, 0,
                 MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Send(message_ptr, int_count, MPI_INT32_T, internode_sender, 0,
                 MPI_COMM_WORLD);
      } else if (is_intranode_sender) {
        struct timespec start, end;
        clock_gettime(CLOCK_REALTIME, &start);
        MPI_Send(message_ptr, int_count, MPI_INT32_T, intranode_receiver, 0,
                 MPI_COMM_WORLD);
        MPI_Recv(message_ptr, int_count, MPI_INT32_T, intranode_receiver, 0,
                 MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        clock_gettime(CLOCK_REALTIME, &end);
        if (opt == INTRA) {
          printf(curr_size == S_COUNT - 1 ? "%e\n" : "%e,",
                 (double)((end.tv_nsec - start.tv_nsec) / (ONE_BILLION)));
        }
      } else {
        MPI_Recv(message_ptr, int_count, MPI_INT32_T, intranode_sender, 0,
                 MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Send(message_ptr, int_count, MPI_INT32_T, intranode_sender, 0,
                 MPI_COMM_WORLD);
      }
    }
  }
#endif
  MPI_Finalize();
  return 0;
}
