#include "includes.h"


// Comando 'topics': mostra todos os tópicos, se não existirem topicos da erro
void listar_topicos(int num_topicos) {
    if (num_topicos == 0) {
        printf("Não existem tópicos cadastrados.\n");
        return;
    }
    
   //resto do codigo para fazer o comando funcionar
}
*/



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

    // Abre o FIFO exclusivo para receber mensagens do manager.c
    up = open(upid, O_RDONLY);

    // Lê a resposta de confirmação do manager.c
    n = read(up, cmd, sizeof(cmd) - 1);
    if (n > 0) {
        cmd[n] = '\0';
        printf("LI... '%s' (%d bytes)\n", cmd, n);
    }

    // Loop de envio de comandos ao manager.c
    do {
        // Lê uma linha inteira (299 char)
        fgets(cmd, TAM+1, stdin);  
        // Remove o '\n' 
        cmd[strcspn(cmd, "\n")] = '\0';
        write(mp, cmd, strlen(cmd) + 1);
    } while (strcmp(cmd, "fim") != 0);

    // Fecha e remove o FIFO
    close(mp);
    close(up);
    if (unlink(upid) == -1) {
        perror("Erro ao remover o FIFO");
    } else {
        printf("FIFO '%s' removido com sucesso.\n", upid);
    }

    printf("FIM FEED...\n");

    return 0;
}
