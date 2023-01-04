FLAGS = -g -Wall -Werror -pthread -std=gnu99

all: csmc

clean:
	rm -f csmc

csmc: csmc.c
	gcc csmc.c -o csmc $(FLAGS)
