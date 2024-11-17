#include "includes.h"


int main(int argc, char* argv[]) {
    if (argc != 1) {
        return 1;
    }

    // VARIAVEIS
    char fifo_feed[20];
    int man_pipe, feed_pipe, tam;
    RESPOSTA r;
    MENSAGEM m;

    //EXISTE SERVIDOR ON?(SO PODE EXISTIR UM)
    if(access(FIFO_SERV,F_OK)==0) {
        printf("[ERRO]-SERVER ON\n");
        exit(3);
    }

    printf("INICIO MANAGER...\n");

    printf("ESPERANDO USERS...\n");

    // Cria o FIFO principal para comunicação
    mkfifo(FIFO_SERV, 0600);
    man_pipe = open(FIFO_SERV, O_RDONLY);

    do {
        tam = read(man_pipe, &m, sizeof(MENSAGEM));
        if (tam == sizeof(MENSAGEM)) {
            printf("\nMENSAGEM DE [%d] | TOPICO[%s] | MSG[%s]\n", m.pid, m.topic, m.str);

            // Construir o FIFO e enviar resposta
            sprintf(fifo_feed, FIFO_CLI, m.pid);
            feed_pipe = open(fifo_feed, O_WRONLY);

            strcpy(r.str, "OK");
            tam = write(feed_pipe, &r, sizeof(RESPOSTA));
            printf("ENVIEI... '%s'\n", r.str);
            close(feed_pipe);
        }
        else{
            break;
        }

    } while (strcmp(m.topic, "quit") != 0);


    // Fecha e remove o FIFO
    close(man_pipe);
    unlink(FIFO_SERV);

    printf("FIM MANAGER...\n");

    return 0;
}
