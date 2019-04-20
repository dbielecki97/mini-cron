OBJ = pipe.o functions.o
all: pipe
pipe: $(OBJ)
	gcc $(OBJ) -o pipe
$(OBJ): functions.h


OBJ2= minicron.o
all:minicron
minicron: $(OBJ2)
	gcc $(OBJ2) -o minicron
.PHONY: clean cleanall
clean: 
	rm -f *.o 
cleanall:
	rm -f *.o out* minicron pipe
