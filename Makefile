all:
	gcc -lncurses nchess.c engine.c -o ./chess
wasd:
	gcc -lncurses snailchess.c engine.c -o ./chess
