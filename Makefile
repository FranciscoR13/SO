# Como usar:
# make: compila tudo
# make clean: remove tudo

# Definir variaveis
CC = gcc                 # Compilador
CFLAGS = -Wall -g        # Flags de compilação (com warnings e debug)
OBJ_FEED = feed.o        # Arquivo objeto do feed.c
OBJ_MANAGER = manager.o  # Arquivo objeto do manager.c
EXEC_FEED = feed         # Nome do executável para feed.c
EXEC_MANAGER = manager   # Nome do executável para manager.c

# Compilar tudo
all: $(EXEC_FEED) $(EXEC_MANAGER)

# Regra para compilaro feed.c
$(EXEC_FEED): $(OBJ_FEED)
	$(CC) $(CFLAGS) -o $(EXEC_FEED) $(OBJ_FEED)

# Regra para compilar o manager.c
$(EXEC_MANAGER): $(OBJ_MANAGER)
	$(CC) $(CFLAGS) -o $(EXEC_MANAGER) $(OBJ_MANAGER)

# Regra para compilar o feed.o
feed.o: feed.c includes.h
	$(CC) $(CFLAGS) -c feed.c

# Regra para compilar o manager.o
manager.o: manager.c includes.h
	$(CC) $(CFLAGS) -c manager.c

# Limpar tudo
clean:
	rm -f $(OBJ_FEED) $(OBJ_MANAGER) $(EXEC_FEED) $(EXEC_MANAGER)
