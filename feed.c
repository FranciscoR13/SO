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
    if (argc != 2) {
        return 1;
    }

    //VARIAVEIS
    char msg[MSG], topic[TOPIC], fifo_feed[20];
    int man_pipe, feed_pipe, tam;
    pid_t pid = getpid();
    MENSAGEM m;
    RESPOSTA r;

    //se nao existir da erro
    if(access(FIFO_SERV,F_OK)!=0) {
        printf("[ERRO]-SERVER OFF\n");
        exit(3);
    }

    printf("INICIO FEED...\n");
    printf("PID: %d\n", pid);

    //FIFO QUE RECEBE MENSAGENS DO MANAGER
    sprintf(fifo_feed, FIFO_CLI, pid);
    mkfifo(fifo_feed, 0600);
    feed_pipe = open(fifo_feed, O_RDWR);
    man_pipe = open(FIFO_SERV, O_WRONLY);

    do {
        //TOPICO?
        printf("\nTOPICO: ");
        fflush(stdout);
        fgets(topic, TOPIC, stdin);
        topic[strcspn(topic, "\n")] = '\0';

        if(strcmp(topic,"quit")==0){break;}

        //MENSAGEM?
        printf("MENSAGEM: ");
        fflush(stdout);
        fgets(msg, MSG, stdin);
        msg[strcspn(msg, "\n")] = '\0';

        //ESTRUTURA MENSAGEM
        m.pid = pid;
        strcpy(m.topic,topic);
        strcpy(m.str,msg);

        tam = write(man_pipe, &m, sizeof(MENSAGEM));

        if (tam == sizeof(MENSAGEM)) {
            printf("\nTPICO[%s] | MSG[%s]\n", m.topic, m.str);

            tam = read(feed_pipe, &r, sizeof(RESPOSTA));

            if (tam == sizeof(RESPOSTA)) {
                printf("SERVER RESPONDE: '%s'\n", r.str);
            }
        }

    } while (strcmp(m.topic, "quit") != 0);

    close(feed_pipe);
    close(man_pipe);
    unlink(fifo_feed);


    printf("FIM FEED...\n");

    return 0;
}
