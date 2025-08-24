# bthelmer Braden T Helmer
#
# p1.Makefile
# ~~~~~~~~~~~
# Makefile for HW2 P1
CC := gcc -g

MY_MPI_SRC := my_mpi.c
MY_MPI_HEADER := my_mpi.h
MY_MPI_OBJ := my_mpi.o
RTT_SRC := my_rtt.c
OUT := my_rtt

PRUN := my_prun

all: $(OUT)

$(OUT): $(MY_MPI_OBJ) $(RTT_SRC)
	$(CC) -o $@ $^

$(MY_MPI_OBJ): $(MY_MPI_SRC) $(MY_MPI_HEADER)
	$(CC) -c -o $@ $< 

run: $(OUT)
	./$(PRUN) ./$(OUT)

clean:
	rm $(MY_MPI_OBJ) $(OUT)
