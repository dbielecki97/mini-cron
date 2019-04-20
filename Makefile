OBJ = pipe.o functions.o
all: pipe
pipe: $(OBJ)
	gcc $(OBJ) -o pipe
$(OBJ): functions.h


OBJ2= demon.o
all:demon
demon: $(OBJ2)
	gcc $(OBJ2) -o demon
.PHONY: clean cleanall
clean: 
	rm -f *.o 
cleanall:
	rm -f *.o out* demon pipe
