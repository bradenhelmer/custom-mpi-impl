// bthelmer Braden T Helmer
//
// my_mpi.c
// ~~~~~~~~
// MPI Interface Implementation
//
// ALL FUNCTIONS PREFIXED WITH '__' ARE DEEMED PRIVATE (not part of the my_mpi.h
// header API)
#include "my_mpi.h"
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define CONNECT_LIMIT 64
#define PRINT_RANK printf("Rank %d\n", proc.rank)
#define PRINT_PROCS                                                            \
  for (int i = 0; i < comm_w.numtasks; i++) {                                  \
    process p = comm_w.process_list[i];                                        \
    struct in_addr ia;                                                         \
    ia.s_addr = p.proc_addr;                                                   \
    const char *ip = inet_ntoa(ia);                                            \
    printf("Process rank %d at %s:%d\n", p.rank, ip, p.port);                  \
  }

// Output an error to stderr with a newline and exit.
static void __errorln(const char *msg) {
  fprintf(stderr, "%s\n", msg);
  exit(EXIT_FAILURE);
}

// SOCKET FUNCTIONALITY
// ~~~~~~~~~~~~~~~~~~~~
// This will only be defined in the c file as
// it can be kept private from the my_mpi API.

// Process specific socket items
static int proc_server_sock;
static int *incoming_connections;
static int *outgoing_connections;
static struct sockaddr_in proc_addr;
static struct sockaddr_in root_addr;

// Helper for connecting new sockets to root
static inline void __connect_to_root(int sock) {
  int connected;
  do {
    connected = connect(sock, (struct sockaddr *)&root_addr, sizeof(root_addr));
  } while (connected != 0);
}

static inline void __connect_to_proc(int sock, process *proc) {
  struct sockaddr_in new_proc_addr;
  new_proc_addr.sin_port = htons(proc->port);
  new_proc_addr.sin_family = AF_INET;
  new_proc_addr.sin_addr.s_addr = proc->proc_addr;

  int connected;
  do {
    connected =
        connect(sock, (struct sockaddr *)&new_proc_addr, sizeof(new_proc_addr));
  } while (connected != 0);
}

// Handles socket initialization/binding etc.
static void __init_server() {

  // Get new socket for process
  proc_server_sock = socket(AF_INET, SOCK_STREAM, 0);
  if (proc_server_sock < 0) {
    perror("socket");
    __errorln("Error opening socket!");
  }

  // Init address information for root and proc.
  proc_addr.sin_port = htons(proc.port);
  proc_addr.sin_family = AF_INET;
  proc_addr.sin_addr.s_addr = INADDR_ANY;

  // Root
  root_addr.sin_port = htons(comm_w.root_port);
  root_addr.sin_family = AF_INET;
  root_addr.sin_addr.s_addr = comm_w.root_addr;

  // Set sockopt
  int optlevel = 1;
  int setter = setsockopt(proc_server_sock, SOL_SOCKET, SO_REUSEADDR, &optlevel,
                          sizeof(optlevel));
  if (setter < 0)
    perror("setsockopt failed");

  // Bind
  if (bind(proc_server_sock, (struct sockaddr *)&proc_addr, sizeof(proc_addr)) <
      0) {
    perror("bind");
    __errorln("Error binding socket");
  }
}

// Closes outstanding socket connection.
static void __close_sockets() {
  for (int i = 0; i < comm_w.numtasks; i++) {
    if (incoming_connections[i] != -1) {
      close(incoming_connections[i]);
    }
    if (outgoing_connections[i] != -1) {
      close(outgoing_connections[i]);
    }
  }
}

// Initialization function to collect all required information.
int MPI_Init(int *argc, char ***argv) {

  // Execution globals
  uint32_t node_count = atoi(getenv("MYMPI_NNODES"));
  uint32_t proc_count = atoi(getenv("MYMPI_NTASKS"));

  // Proc specific items
  uint32_t proc_rank = atoi(getenv("MYMPI_RANK"));
  uint16_t proc_port = atoi(getenv("MYMPI_PORT"));

  // Root info
  uint16_t root_port = atoi(getenv("MYMPI_ROOT_PORT"));

  // Initialization
  proc.port = proc_port;
  proc.rank = proc_rank;

  comm_w.process_list = (process *)malloc(sizeof(process) * proc_count);
  comm_w.numtasks = proc_count;
  comm_w.root_port = root_port;

  // Allocate space for connections, set to -1;
  incoming_connections = (int *)malloc(sizeof(int) * comm_w.numtasks);
  outgoing_connections = (int *)malloc(sizeof(int) * comm_w.numtasks);
  for (int i = 0; i < comm_w.numtasks; i++) {
    incoming_connections[i] = -1;
    outgoing_connections[i] = -1;
  }

  // Get host information
  struct hostent *he_root = gethostbyname2(getenv("MYMPI_ROOT_HOST"), AF_INET);
  bcopy((char *)he_root->h_addr, (char *)&comm_w.root_addr, he_root->h_length);

  // Get proc information
  struct hostent *he_proc = gethostbyname2(getenv("MYMPI_NODENAME"), AF_INET);
  bcopy((char *)he_proc->h_addr, (char *)&proc.proc_addr, he_proc->h_length);

  // Initialize socket information for proc and root
  __init_server();

  // Set connection fds to self;
  incoming_connections[proc.rank] = proc_server_sock;
  outgoing_connections[proc.rank] = proc_server_sock;

  if (proc.rank == ROOT) {

    // Do initial gather of processes.
    comm_w.process_list[0] = proc;
    MPI_Gather(NULL, 0, MPI_PROCESS_T, comm_w.process_list, comm_w.numtasks - 1,
               MPI_PROCESS_T, ROOT, comm_w);

    // Once found, broadcast information down to child processes.
    MPI_Bcast(comm_w.process_list, comm_w.numtasks, MPI_PROCESS_T, ROOT,
              comm_w);
  } else {

    // Subsequent gather and broadcast calls for children.
    MPI_Gather(&proc, 1, MPI_PROCESS_T, NULL, 0, MPI_PROCESS_T, ROOT, comm_w);
    MPI_Bcast(comm_w.process_list, comm_w.numtasks, MPI_PROCESS_T, ROOT,
              comm_w);
  }

  return 0;
}

int MPI_Comm_rank(MPI_Comm comm, int *rank) {
  *rank = proc.rank;
  return 0;
}

int MPI_Comm_size(MPI_Comm comm, int *size) {
  *size = comm_w.numtasks;
  return 0;
}

int MPI_Finalize() {
  MPI_Barrier(comm_w);
  free(comm_w.process_list);
  free(incoming_connections);
  free(outgoing_connections);
  __close_sockets();
  close(proc_server_sock);
  return 0;
}

// private function for sending ints
int __MPI_Send_INT32T(int32_t *buf, int count, int dest, MPI_Comm comm) {
  int bytesToSend = count * sizeof(int32_t);
  int bytesSent = 0;

  while (bytesSent < bytesToSend) {
    bytesSent +=
        send(outgoing_connections[dest], &buf[bytesSent / sizeof(int32_t)],
             bytesToSend - bytesSent, 0);
    if (bytesSent == -1) {
      perror("send");
      exit(1);
    }
  }

  return 0;
}

int MPI_Send(const void *buf, int count, MPI_Datatype datatype, int dest,
             int tag, MPI_Comm comm) {

  // If no connection, start one up.
  if (outgoing_connections[dest] == -1) {
    int new_fd = socket(AF_INET, SOCK_STREAM, 0);
    outgoing_connections[dest] = new_fd;
    __connect_to_proc(new_fd, &comm_w.process_list[dest]);
  }

  switch (datatype) {
  case MPI_INT32_T:
    return __MPI_Send_INT32T((int32_t *)buf, count, dest, comm);
  default:
    return 0;
  }

  return 0;
}

// private function for receiving ints

int __MPI_Recv_INT32T(int32_t *buf, int count, int source, MPI_Comm comm) {
  int bytesToReceive = count * sizeof(int32_t);
  int bytesReceived = 0;
  while (bytesReceived < bytesToReceive) {
    bytesReceived += recv(incoming_connections[source],
                          &buf[bytesReceived / sizeof(int32_t)],
                          bytesToReceive - bytesReceived, 0);
    if (bytesReceived == -1) {
      perror("recv");
      exit(1);
    }
  }

  return 0;
}

int MPI_Recv(void *buf, int count, MPI_Datatype datatype, int source, int tag,
             MPI_Comm comm, MPI_Status *status) {

  // If no incoming connection, listen for one
  listen(proc_server_sock, 1);
  if (incoming_connections[source] == -1) {
    struct sockaddr_in new_addr;
    socklen_t new_addr_len = sizeof(new_addr);
    int new_fd =
        accept(proc_server_sock, (struct sockaddr *)&new_addr, &new_addr_len);
    incoming_connections[source] = new_fd;
  }

  switch (datatype) {
  case MPI_INT32_T:
    return __MPI_Recv_INT32T((int32_t *)buf, count, source, comm);
  default:
    return 0;
  }

  return 0;
}
int MPI_Barrier(MPI_Comm comm) {
  // Simple empty broadcast to act as a barrier.
  return MPI_Bcast(NULL, 0, MPI_NULL_T, ROOT, comm);
}

// Private function for sending a process struct.
void __send_process(int fd, process *sending_proc) {

  // Get references for possible byte conversion later.
  uint32_t net_proc_addr = sending_proc->proc_addr;
  uint32_t net_rank = sending_proc->rank;
  uint16_t net_port = sending_proc->port;

  // Send
  send(fd, &net_proc_addr, sizeof(uint32_t), 0);
  send(fd, &net_rank, sizeof(uint32_t), 0);
  send(fd, &net_port, sizeof(uint16_t), 0);
}

// Private function for receiving processes.
void __recv_process(int fd, process *receiving_proc) {

  // Variables for reception
  uint32_t host_proc_addr;
  uint32_t host_rank;
  uint16_t host_port;

  // Receive
  recv(fd, &host_proc_addr, sizeof(uint32_t), 0);
  recv(fd, &host_rank, sizeof(uint32_t), 0);
  recv(fd, &host_port, sizeof(uint16_t), 0);

  // Populate
  receiving_proc->proc_addr = host_proc_addr;
  receiving_proc->port = host_port;
  receiving_proc->rank = host_rank;
}

// Private function for gathering processes.
int __MPI_Gather_PROCESS(process *sendbuf, int sendcount, process *recvbuf,
                         int recvcount, int root, MPI_Comm comm) {
  // Root do gathering receive.
  if (proc.rank == root) {
    process curr_proc;
    struct sockaddr_in curr_proc_addr;
    socklen_t curr_proc_len = sizeof(curr_proc_addr);
    listen(proc_server_sock, recvcount);
    for (int i = 0; i < recvcount; i++) {

      // Accept connections
      int acceptance = accept(
          proc_server_sock, (struct sockaddr *)&curr_proc_addr, &curr_proc_len);

      // Receive and populate
      __recv_process(acceptance, &curr_proc);
      memcpy(((process *)recvbuf) + curr_proc.rank, &curr_proc,
             sizeof(curr_proc));

      // Update client connections
      if (incoming_connections[curr_proc.rank] == -1) {
        incoming_connections[curr_proc.rank] = acceptance;
      } else {
        close(acceptance);
      }
    }
  }

  // Child do send
  else {

    // Create new client socket if none
    if (outgoing_connections[root] == -1) {
      int new_fd = socket(AF_INET, SOCK_STREAM, 0);
      outgoing_connections[root] = new_fd;
      __connect_to_root(new_fd);
    }
    // Do send
    __send_process(outgoing_connections[root], (process *)sendbuf);
  }
  return 0;
}

int MPI_Gather(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
               void *recvbuf, int recvcount, MPI_Datatype recvtype, int root,
               MPI_Comm comm) {

  // Invoke handlers for each datatype.
  switch (sendtype) {
  case MPI_PROCESS_T:
    return __MPI_Gather_PROCESS((process *)sendbuf, sendcount,
                                (process *)recvbuf, recvcount, root, comm);
  default:
    return 0;
  }
}

// Private function for broadcasting processes.
int __MPI_Bcast_PROCESS(process *buffer, int count, int root, MPI_Comm comm) {

  // Root, send
  if (proc.rank == root) {

    // For each child process
    for (int i = 1; i < comm.numtasks; i++) {
      if (outgoing_connections[i] == -1) {
        int new_fd = socket(AF_INET, SOCK_STREAM, 0);
        outgoing_connections[i] = new_fd;
        __connect_to_proc(new_fd, &comm.process_list[i]);
      }

      // For each process in list
      for (int j = 0; j < comm.numtasks; j++) {
        __send_process(outgoing_connections[i], &buffer[j]);
      }
    }
  }
  // Child, receive
  else {
    // Establish connection with root if not done so already.
    listen(proc_server_sock, 1);
    if (incoming_connections[root] == -1) {
      struct sockaddr_in root_proc_addr;
      socklen_t root_proc_len = sizeof(root_proc_addr);
      int acceptance = accept(
          proc_server_sock, (struct sockaddr *)&root_proc_addr, &root_proc_len);
      incoming_connections[root] = acceptance;
    }
    // For each process needing reception
    process curr_proc;
    for (int i = 0; i < count; i++) {

      // Receive and populate list
      __recv_process(incoming_connections[root], &curr_proc);
      memcpy(&buffer[curr_proc.rank], &curr_proc, sizeof(curr_proc));
    }
  }

  return 0;
}

int __MPI_Bcast_NULL(int root) {
  if (proc.rank == root) {

    // For each child process
    for (int i = 1; i < comm_w.numtasks; i++) {
      if (outgoing_connections[i] == -1) {
        int new_fd = socket(AF_INET, SOCK_STREAM, 0);
        outgoing_connections[i] = new_fd;
        __connect_to_proc(new_fd, &comm_w.process_list[i]);
      }
      const int zero = 0;
      send(outgoing_connections[i], &zero, sizeof(const int), 0);
    }
  }
  // Child, receive
  else {
    // Establish connection with root if not done so already.
    listen(proc_server_sock, 1);
    if (incoming_connections[root] == -1) {
      struct sockaddr_in root_proc_addr;
      socklen_t root_proc_len = sizeof(root_proc_addr);
      int acceptance = accept(
          proc_server_sock, (struct sockaddr *)&root_proc_addr, &root_proc_len);
      incoming_connections[root] = acceptance;
    }
    int zero;
    recv(incoming_connections[root], &zero, sizeof(int), 0);
  }
  return 0;
}

int MPI_Bcast(void *buffer, int count, MPI_Datatype datatype, int root,
              MPI_Comm comm) {
  switch (datatype) {
  case MPI_PROCESS_T:
    return __MPI_Bcast_PROCESS((process *)buffer, count, root, comm);
  case MPI_NULL_T:
    return __MPI_Bcast_NULL(root);
  default:
    return 0;
  }
}
