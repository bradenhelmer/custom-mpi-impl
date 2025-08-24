// bthelmer Braden T Helmer
//
// my_mpi.h
// ~~~~~~~~
// Definitions for custom MPI interface
#include <netinet/ip.h>

#define ROOT 0

// Typedef something for the communication obj
typedef struct {
  uint16_t port;
  uint32_t rank;
  in_addr_t proc_addr;
} process;

// Global process information
static process proc;

typedef struct {
  process *process_list;
  uint16_t root_port;
  in_addr_t root_addr;
  uint32_t numtasks;
} MPI_Comm;

static MPI_Comm MPI_COMM_WORLD;
#define comm_w MPI_COMM_WORLD

// Setup and tear down definitions
int MPI_Init(int *argc, char ***argv);
int MPI_Comm_rank(MPI_Comm comm, int *rank);
int MPI_Comm_size(MPI_Comm comm, int *size);
int MPI_Finalize();

// Define an enum for representing MPI_Datatypes
typedef enum mpi_datatype {
  MPI_INT32_T,
  MPI_PROCESS_T,
  MPI_NULL_T
} MPI_Datatype;

// Send
int MPI_Send(const void *buf, int count, MPI_Datatype datatype, int dest,
             int tag, MPI_Comm comm);

// Similar enum definition for MPI_Status
typedef int MPI_Status;
static MPI_Status *MPI_STATUS_IGNORE;

// Receive
int MPI_Recv(void *buf, int count, MPI_Datatype datatype, int source, int tag,
             MPI_Comm comm, MPI_Status *status);

// MPI_Barrier - Needed for MPI_Finalize
int MPI_Barrier(MPI_Comm comm);

// MPI_Gather / MPI_Broadcast
int MPI_Gather(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
               void *recvbuf, int recvcount, MPI_Datatype recvtype, int root,
               MPI_Comm comm);

int MPI_Bcast(void *buffer, int count, MPI_Datatype datatype, int root,
              MPI_Comm comm);
