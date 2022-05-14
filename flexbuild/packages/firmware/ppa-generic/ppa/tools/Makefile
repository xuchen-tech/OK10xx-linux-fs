CC=gcc
OBJ = relocate-rela.o 

%.o: %.c 
	$(CC) -c -o $@ $< 

relocate-rela: $(OBJ)
	$(CC) -o $@ $^ 

.PHONY: clean

clean:
	rm -f *.o *.exe relocate-rela
