OBJ = pipe.o functions.o
all: pipe
pipe: $(OBJ)
	gcc $(OBJ) -o pipe
$(OBJ): functions.h


OBJ2= minicron.o
all:minicron
minicron: $(OBJ2)
	gcc $(OBJ2) -o minicron
.PHONY: clean cleanall sigusr1 sigusr2 sigint test_pipe1 test_pipe2 test_pipe3 test_mode0 test_mode1 test_mode2
clean: 
	@rm -f *.o
	@echo "Deleted *.o files"
cleanall:
	@rm -f *.o out* minicron pipe
	@echo "Deleted *.o, minicron, pipe and out* files"
sigusr1:
	@kill -10 `ps -e | grep minicron |  egrep -o '^.[0-9]{1,}'`
	@echo "SIGUSR1 was sent to deamon"
sigusr2:
	@kill -12 `ps -e | grep minicron |  egrep -o '^.[0-9]{1,}'`
	@echo "SIGUSR2 was sent to deamon"
sigint:
	@kill -2 `ps -e | grep minicron |  egrep -o '^.[0-9]{1,}'`
	@echo "SIGINT was sent to deamon"
test_pipe1:
	@echo "Testing pipe with 1 segment!"
	./pipe "ls -l" test.txt 2
	@cat test.txt
	@rm test.txt
	@echo "Test complete!"
test_pipe2:
	@echo "Testing pipe with 2 segments!"
	./pipe "ls -l | wc" test.txt 2
	@cat test.txt
	@rm test.txt
	@echo "Test complete!"
test_pipe3:
	@echo "Testing pipe with 3 segments!"
	./pipe "ls -l | wc | wc" test.txt 2
	@cat test.txt
	@rm test.txt
	@echo "Test complete!"
test_mode0:
	@echo "Testing pipe with mode 0"
	./pipe "cat ahndiisafing | echo Testujemy" test.txt 0
	@cat test.txt
	@rm test.txt
	@echo "Test complete!"
test_mode1:
	@echo "Testing pipe with mode 2"
	./pipe "cat ahndiisafing | echo Testujemy" test.txt 1
	@cat test.txt
	@rm test.txt
	@echo "Test complete!"
test_mode2:
	@echo "Testing pipe with mode 1"
	./pipe "cat ahndiisafing | echo Testujemy" test.txt 2
	@cat test.txt
	@rm test.txt
	@echo "Test complete!"