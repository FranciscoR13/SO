#include "includes.h"

bool verifica_login(LOGIN *l, int nUsers){
    //verificar se o username ja existe
    return true;
}

int main(int argc, char* argv[]) {
    if (argc != 1) {
        return 1;
    }

    // VARIAVEIS
    char fifo_feed[TAM];
    int man_pipe, feed_pipe, tam, nUsers = 0;;
    RESPOSTA r;
    MENSAGEM m;
    LOGIN l;

    // EXISTE SERVIDOR ON?(SO PODE EXISTIR UM)
    if(access(FIFO_SERV,F_OK)==0) {
        printf("[ERRO]-SERVER ON\n");
        exit(3);
    }

    // MENSAGENS INICIAIS
    printf("INICIO MANAGER...\n");
    printf("ESPERANDO USERS...\n");

    // CRIA/ABRE FIFO_SERV
    mkfifo(FIFO_SERV, 0600);
    man_pipe = open(FIFO_SERV, O_RDWR);

    // LOGIN
    read(man_pipe, &l, sizeof(LOGIN));
    sprintf(fifo_feed, FIFO_CLI, l.pid);
    feed_pipe = open(fifo_feed, O_WRONLY);

    if (verifica_login(&l, nUsers)) {
        strcpy(r.str,"OK");
        tam = write(feed_pipe, r.str, sizeof(RESPOSTA));  // Envia confirmação de login bem-suced
        if (tam == sizeof(RESPOSTA)) {
            printf("USERNAME: %s | PID: %d\n", l.username, l.pid);

            //GUARDAR O NOVO USER...

            nUsers++;
        }
    } else {
        write(feed_pipe, 0, 0);  // Envia falha de login
    }
    //FIM LOGIN

    // CICLO QUE RECEBE MSG
    do {
        tam = read(man_pipe, &m, sizeof(MENSAGEM));
        if (tam == sizeof(MENSAGEM)) {
            printf("\nMENSAGEM DE [%d] | TOPICO[%s] | MSG[%s]\n", m.pid, m.topic, m.str);

            // CONSTROI NOME/ABRE/RESPONDE/FECHA FIFO_CLI
            sprintf(fifo_feed, FIFO_CLI, m.pid);
            feed_pipe = open(fifo_feed, O_WRONLY);

            strcpy(r.str, "OK");
            tam = write(feed_pipe, &r, sizeof(RESPOSTA));
            if(tam>0) {
                printf("ENVIEI... '%s'\n", r.str);
                close(feed_pipe);
            }
        }
        else{
            break;
        }

    } while (strcmp(m.topic, "quit") != 0); // TEMPORARIO


    // FECHA PIPES & APAGA FIFO
    close(man_pipe);
    unlink(FIFO_SERV);

    printf("\nFIM MANAGER...\n");

    return 0;
}
