# Custom MPI Implementation

**Author:** Braden T Helmer  

## Overview

This project implements a custom Message Passing Interface (MPI) library from scratch, providing core MPI functionality including point-to-point communication, collective operations, and process management. The implementation is designed for educational purposes to understand the underlying mechanisms of parallel message passing systems.

## Features

### Core MPI Functions
- **Process Management:**
  - `MPI_Init()` - Initialize the MPI environment
  - `MPI_Comm_rank()` - Get process rank
  - `MPI_Comm_size()` - Get total number of processes
  - `MPI_Finalize()` - Clean up MPI environment

- **Point-to-Point Communication:**
  - `MPI_Send()` - Blocking send operation
  - `MPI_Recv()` - Blocking receive operation

- **Collective Operations:**
  - `MPI_Barrier()` - Synchronization barrier
  - `MPI_Gather()` - Gather data from all processes
  - `MPI_Bcast()` - Broadcast data to all processes

- **Supported Data Types:**
  - `MPI_INT32_T` - 32-bit integers
  - `MPI_PROCESS_T` - Process information structures

## Architecture

The implementation uses TCP sockets for inter-process communication and includes:
- Custom process launcher (`my_prun`) that works with SLURM
- Network-based communication using IP addresses and ports
- Support for both intra-node and inter-node communication

## Build Instructions

### Prerequisites
- GCC compiler
- Python 3 (for port allocation)
- SLURM environment (for multi-node execution)

### Building
```bash
make         # Build the executable
make run     # Build and run with my_prun launcher
make clean   # Clean build artifacts
```

## Usage

### Running the RTT Benchmark
The included Round-Trip Time (RTT) benchmark tests message passing performance:

```bash
# Basic execution (requires SLURM environment)
./my_prun ./my_rtt

# For Case 4 testing (inter/intra node comparison)
# Compile with -DCASE4 flag first
./my_prun ./my_rtt inter  # Test inter-node communication
./my_prun ./my_rtt intra  # Test intra-node communication
```

### Environment Variables
The `my_prun` launcher sets up the following environment variables:
- `MYMPI_ROOT_HOST` - Hostname of the root process
- `MYMPI_ROOT_PORT` - Port number for root process
- `MYMPI_PORT` - Port number for current process
- `MYMPI_RANK` - Process rank
- `MYMPI_NTASKS` - Total number of tasks
- `MYMPI_NNODES` - Number of nodes

## Benchmark Details

The RTT benchmark tests message passing latency across different message sizes:
- 32KB, 64KB, 128KB, 256KB, 512KB, 1MB, 2MB
- 100 iterations per message size for statistical accuracy
- Measures round-trip time for send-receive pairs
- Outputs timing data in scientific notation (nanoseconds)

## Performance Comparison to Standard MPI

In performance testing against standard MPI implementations, this custom implementation shows significantly higher latency:
- **Latency:** 100x to 1000x slower than optimized MPI libraries
- **Throughput:** Reduced due to lack of optimization techniques like:
  - Zero-copy transfers
  - Shared memory optimizations
  - Advanced networking protocols (InfiniBand, etc.)
  - Pipelined message transfers

This performance difference illustrates the complexity and optimization present in production MPI implementations.

## File Structure

- `my_mpi.h` - Header file with MPI function declarations and data structures
- `my_mpi.c` - Implementation of core MPI functions
- `my_rtt.c` - RTT benchmark application
- `my_prun` - Custom process launcher script for SLURM environments
- `Makefile` - Build configuration

## Technical Implementation Notes

- Uses TCP sockets for reliable message delivery
- Process discovery through environment variables set by launcher
- Simple blocking communication model
- Basic error handling and process coordination
- Compatible with SLURM job scheduler for multi-node execution

---

**Note:** This is an educational implementation designed to demonstrate MPI concepts. For production use, utilize optimized MPI libraries like OpenMPI or MPICH.

