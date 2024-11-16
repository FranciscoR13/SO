#include "includes.h"


int main(int argc, char* argv[]) {
    if (argc != 1) {
        return 1;
    }

    printf("INICIO MANAGER...\n");

    char cmd[TAM], upid[10];  // Buffer para upid maior
    int mp, up, n;

    printf("ESPERANDO USERS...\n");

    // Cria o FIFO principal para comunicação
    mkfifo(FIFO_CS, 0600);
    mp = open(FIFO_CS, O_RDONLY);

    // Lê o PID enviado pelo `feed.c`
    n = read(mp, upid, sizeof(upid) - 1);
    upid[n] = '\0';  // Null-terminator

    printf("CHEGOU UM USER...\n");

    // Cria caminho para FIFO exclusivo do `feed.c`
    printf("Recebi PID: %s\n", upid);
    up = open(upid, O_WRONLY);
    if (up == -1) {
        perror("Erro ao abrir FIFO do usuário");
        close(mp);
        unlink(FIFO_CS);
        return 1;
    }

    // Envia uma mensagem de confirmação
    strcpy(cmd, "Recebi o teu pid.");
    write(up, cmd, strlen(cmd) + 1);

    // Loop de leitura de mensagens do feed.c
    do {
        n = read(mp, cmd, TAM - 1);
        if (n > 0) {
            cmd[n] = '\0';
            printf("MSG: '%s' (%d bytes)\n", cmd, n);
        }
    } while (strncmp(cmd, "quit", 4) != 0);

    // Fecha e remove o FIFO
    close(mp);
    close(up);
    unlink(FIFO_CS);

    printf("FIM MANAGER...\n");

    return 0;
}
