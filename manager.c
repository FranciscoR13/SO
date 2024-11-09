#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#define TAM 50
#define FIFO_CS "M-PIPE"

int main(int argc, char* argv[]) {
    if (argc != 1) {
        return 1;
    }

    printf("INICIO MANAGER...\n");

    // VARIAVEIS
    char cmd[TAM], upid[10];  
    int mp, up, n;

    printf("ESPERANDO USERS...\n");

    // MANAGER PIPE
    mkfifo(FIFO_CS, 0600);
    mp = open(FIFO_CS, O_RDONLY);
    n = read(mp, upid, sizeof(upid) - 1);         // LÊ O PID DO USER
    upid[n] = '\0';                               // Null-terminator
    printf("USER  CHEGOU: %s\n", upid);

    //USER PIPE
    up = open(upid, O_WRONLY);
    if (up == -1) {
        perror("Erro ao abrir FIFO do usuário");
        close(mp);
        unlink(FIFO_CS);
        return 1;
    }

    // MAN->USER
    strcpy(cmd, "LOGIN COM SUCESSO.");
    write(up, cmd, strlen(cmd) + 1);              // Envia uma mensagem de confirmação

    // Loop de leitura de mensagens do feed.c
    do {
        n = read(mp, cmd, TAM - 1);
        if (n > 0) {
            cmd[n] = '\0';
            printf("MSG: '%s'\n", cmd);
        }
    } while (strncmp(cmd, "quit", 4) != 0);

    // Fecha e remove o FIFO
    close(mp);
    close(up);
    unlink(FIFO_CS);

    printf("FIM MANAGER...\n");

    return 0;
}
