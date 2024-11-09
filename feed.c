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

    //FIFO QUE RECEBE MENSAGENS DO MANAGER
    snprintf(upid, sizeof(upid), "%d", pid); // Gera o nome do FIFO exclusivo usando o PID
    mkfifo(upid, 0600); // Cria o FIFO exclusivo para leitura de resposta

    // Abre o FIFO principal para comunicação com o gerenciador
    mp = open(FIFO_CS, O_WRONLY);

    // Envia o PID para o manager.c
    write(mp, upid, strlen(upid) + 1);

    // Abre o FIFO exclusivo para receber mensagens do `manager.c`
    up = open(upid, O_RDONLY);

    // Lê a resposta de confirmação do manager.c
    n = read(up, cmd, sizeof(cmd) - 1);
    if (n > 0) {
        cmd[n] = '\0';
        printf("LI... '%s' (%d bytes)\n", cmd, n);
    }

    // Loop de envio de comandos ao manager.c
    do {
        scanf("%s", cmd);
        write(mp, cmd, strlen(cmd) + 1);
    } while (strcmp(cmd, "fim") != 0);

    // Fecha e remove o FIFO
    close(mp);
    close(up);
    // Remove o FIFO
    if (unlink(upid) == -1) {
        perror("Erro ao remover o FIFO");
    } else {
        printf("FIFO '%s' removido com sucesso.\n", upid);
    }

    printf("FIM FEED...\n");

    return 0;
}
