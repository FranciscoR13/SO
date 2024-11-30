#include "includes.h"


// Comando 'topics': mostra todos os tópicos, se não existirem topicos da erro
void listar_topicos(int num_topicos) {
    if (num_topicos == 0) {
        printf("Não existem tópicos cadastrados.\n");
        return;
    }
    
   //resto do codigo para fazer o comando funcionar
}



// FUNCAO DE LOGIN QUE ENVIA O USERNAME E O PID
bool login(int man_pipe, int feed_pipe, char* username, int pid) {

    //VARIAVEIS
    int tam;
    LOGIN l;
    RESPOSTA r;

    // ESTRURA LOGIN
    strncpy(l.username, username, sizeof(l.username) - 1);
    l.username[sizeof(l.username) - 1] = '\0'; // Garantir que a string seja terminada
    l.pid = pid;

    //TENTA ENVIAR O LOGIN
    tam = write(man_pipe, &l, sizeof(LOGIN));
    if (tam < sizeof(LOGIN)) {
        return false;  // Se não escreveu todos os bytes
    }

    //TENTA RECEBER O LOGIN
    tam = read(feed_pipe, &r, sizeof(RESPOSTA));
    if (strcmp(r.str,"FAIL")==0) {
        return false;  // Se não leu todos os bytes esperados
    }

    return true;
}

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

    //EXISTE SERVER? SE NAO -> TERMINA
    if(access(FIFO_SERV,F_OK)!=0) {
        printf("[ERRO]-SERVER OFF\n");
        exit(1);
    }

    // MENSAGEM DE ABERTURA
    printf("INICIO FEED...\n");

    //FIFO QUE RECEBE MENSAGENS DO MANAGER + ABRE PIPES
    sprintf(fifo_feed, FIFO_CLI, pid);
    mkfifo(fifo_feed, 0600);
    feed_pipe = open(fifo_feed, O_RDWR);
    man_pipe = open(FIFO_SERV, O_WRONLY);

    //LOGIN
    if(!login(man_pipe,feed_pipe,argv[1],pid)) {
        printf("\nERRO NO LOGIN!\n");
        unlink(fifo_feed);
        close(feed_pipe);
        close(man_pipe);
        exit(2);
    }
    printf("\nLOGIN CONCLUIDO!\nPID: %d | USERNAME: %s\n", pid, argv[1]);

    // CICLO QUE ESCREVE MSG PARA O MANAGER (VAI PASSAR A SER UMA FUNCAO)
    do {
        //TOPICO?
        printf("\nTOPICO: ");
        fflush(stdout);
        fgets(topic, TOPIC, stdin);
        topic[strcspn(topic, "\n")] = '\0';

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

            if (tam == sizeof(RESPOSTA) && strcmp(r.str,"FECHOU")==0) {
                printf("SERVER FECHOU");
                break;
            }

            if (tam == sizeof(RESPOSTA)) {
                printf("SERVER RESPONDE: '%s'\n", r.str);
            }
        }

    } while (strcmp(m.topic, "quit") != 0);

    // FECHA PIPES & APAGA FIFO
    close(feed_pipe);
    close(man_pipe);
    unlink(fifo_feed);

    printf("\nFIM FEED...\n");

    return 0;
}

