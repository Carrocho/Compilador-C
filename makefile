all:
	gcc main.c lexer.c parser.c -o compilador -Wall
	./compilador teste.txt

run:
	./compilador teste.txt

clean:
	rm -f compilador