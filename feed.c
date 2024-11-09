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

    printf("INICIO FEED...\n");

    //VARIAVEIS
    char cmd[TAM], upid[20];
    int mp, up, n;
    pid_t pid = getpid();

    //USER-PIPE
    snprintf(upid, sizeof(upid), "%d", pid);     // Gera o nome do FIFO exclusivo usando o PID
    mkfifo(upid, 0600);                          // Cria o FIFO exclusivo para leitura de resposta
    up = open(upid, O_RDONLY);                   // Abre o FIFO exclusivo para receber mensagens do manager.c

    // MANAGER-PIPE
    mp = open(FIFO_CS, O_WRONLY);

    //ENVIA E RECEBE A RESPOSTA DO MANAGER
    write(mp, upid, strlen(upid) + 1);           // Envia o PID para o manager.c
    n = read(up, cmd, sizeof(cmd) - 1);          
    if (n > 0) {
        cmd[n] = '\0';
        printf("LI... '%s' (%d bytes)\n", cmd, n);
    }

    
    // LOOP DE MSG PARA O MANAGERS
    do {
        scanf("%s", cmd);
        write(mp, cmd, strlen(cmd) + 1);
    } while (strcmp(cmd, "fim") != 0);

    //FECHAR CENAS 
    close(mp);
    close(up);
    unlink(upid);//NAO FUNCIONA FALAR COM O PROF...

    printf("FIM FEED...\n");

    return 0;
}
