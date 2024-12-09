#include "includes.h"

// Comando 'topics': mostra todos os tópicos, se não existirem topicos da erro
void listar_topicos(int num_topicos) {
    if (num_topicos == 0) {
        printf("Não existem tópicos cadastrados.\n");
        return;
    }

    //resto do codigo para fazer o comando funcionar
}

// ENVIA UM SINAL PARA TERMINAR A THREAD
void sinal_termina(int sig, siginfo_t *si, void *u) {
    if (sig == SIGUSR1) {
        printf("\n[SINAL] Encerrando thread de receber respostas...");
        fflush(stdout);
    }
}

// FUNCAO DE LOGIN QUE ENVIA O USERNAME E O PID
bool login(int man_pipe, int feed_pipe, char* username, int pid) {
    // VARIAVEIS
    int tam;
    LOGIN l;
    RESPOSTA r;

    // ESTRUTURA LOGIN
    strncpy(l.username, username, sizeof(l.username) - 1);
    l.username[sizeof(l.username) - 1] = '\0'; // Garantir que a string seja terminada
    l.pid = pid;

    // TENTA ENVIAR O LOGIN
    tam = write(man_pipe, &l, sizeof(LOGIN));
    if (tam < sizeof(LOGIN)) {
        return false; // Se não escreveu todos os bytes
    }

    // TENTA RECEBER O LOGIN
    tam = read(feed_pipe, &r, sizeof(RESPOSTA));
    if (tam != sizeof(RESPOSTA) || strcmp(r.str, "FAIL") == 0) {
        return false; // Se não leu todos os bytes esperados
    }

    return true;
}

// FUNCAO QUE ENVIA MENSAGEM
bool envia_msg(int man_pipe, int feed_pipe) {
    // VARIAVEIS
    MENSAGEM m;
    char msg[TAM_MSG], topic[TAM_TOPICO];

    // LIMPA BUFFER
    int c;
    while ((c = getchar()) != '\n' && c != EOF);

    // TOPICO?
    printf("\nTOPICO: ");
    fflush(stdout);
    fgets(topic, TAM_TOPICO, stdin);
    topic[strcspn(topic, "\n")] = '\0';

    // MENSAGEM?
    printf("MENSAGEM: ");
    fflush(stdout);
    fgets(msg, TAM_MSG, stdin);
    msg[strcspn(msg, "\n")] = '\0';

    // ESTRUTURA MENSAGEM
    m.pid = getpid();
    strcpy(m.topico, topic);
    strcpy(m.corpo_msg, msg);

    /*
    // ENVIA A MENSAGEM PARA O MANAGER
    tam = write(man_pipe, &m, sizeof(MENSAGEM));
    if (tam == sizeof(MENSAGEM)) {
        printf("\n[ENVIO] Topico: '%s' | Mensagem: '%s'\n", m.topico, m.corpo_msg);

        // RECEBE RESPOSTA DO SERVIDOR
        tam = read(feed_pipe, &r, sizeof(RESPOSTA));
        if (tam == sizeof(RESPOSTA)) {
            if (strcmp(r.str, "FECHOU") == 0) {
                printf("\n[AVISO] O servidor foi encerrado.\n");
                return false;
            }
            printf("\n[RESPOSTA DO SERVER]: '%s'\n", r.str);
        }
    }
    */
    return true;
}

// FUNCAO DA THREAD QUE RECEBE "RESPOSTAS" DO SERVER
void* recebe_info(void* rb) {

     RB* x = (RB*)rb;

    while (1) {
        RESPOSTA r;

        int nb = read(x->man_pipe, &r, sizeof(RESPOSTA));
        if (nb != sizeof(RESPOSTA)) {
            perror("\nErro ao ler dados do feed");
            continue;
        }

        if (strcmp(r.str, "QUIT") == 0) {
            x->ligado = false;
            pthread_kill(x->main_thread, SIGUSR1);
            break;

        }

        // AINDA POR IMPLEMENTAR...
        printf("[Mensagem do Server]: %s\n", r.str);
    }

    printf("[THREAD RI] Fechou! \n");
    return NULL;
}

// MAIN
int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("\nNúmero de argumentos inválido!\n");
        return 1;
    }
    // TERMINAR SE O Nª ESTIVER INCORRETO

    // VERIFICA SE O SERVER ESTÁ ON
    if (access(FIFO_SERV_LOG, F_OK) != 0) {
        printf("[ERRO] Servidor está offline.\n");
        return 1;
    }

    // VARIAVEIS
    char cmd[TAM], fifo_feed[20];
    int man_pipe_log, feed_pipe;
    pid_t pid = getpid();
    //...

    // CONFIGURAÇÃO DO SINAL
    struct sigaction sa;
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = sinal_termina;
    sigaction(SIGUSR1, &sa, NULL);
    //...

    // CONFIGURA OS FIFOS
    snprintf(fifo_feed, sizeof(fifo_feed), FIFO_CLI, pid);
    mkfifo(fifo_feed, 0600);
    feed_pipe = open(fifo_feed, O_RDWR);
    man_pipe_log = open(FIFO_SERV_LOG, O_WRONLY);
    //...

    // MENSAGEM ABERTURA
    printf("INICIO FEED...\n");
    //...

    // FAZ O LOGIN
    if (!login(man_pipe_log, feed_pipe, argv[1], pid)) {
        printf("\n[ERRO] Falha no login.\n");
        close(feed_pipe);
        close(man_pipe_log);
        unlink(fifo_feed);
        return 2;
    }
    printf("\n[LOGIN CONCLUIDO]\nBem-vindo %s! (PID: %d)\n", argv[1], pid);
    // CONTINUA SE O LOGIN FOI CONCLUIDO

    RB rb;
    rb.man_pipe = feed_pipe;
    rb.main_thread = pthread_self();
    rb.ligado = true;

    // CRIA A THREAD QUE RECEBE INFO DO SERVER
    pthread_t thread_server;
    pthread_create(&thread_server, NULL, recebe_info, &rb);

    // LOOP DOS COMANDOS
    while (rb.ligado) {
        printf("%s CMD> ", argv[1]);
        fflush(stdout);
        if (scanf("%s", cmd) == EOF) break;

        if (strcmp(cmd, "msg") == 0) {
            if (envia_msg(man_pipe_log, feed_pipe)) {

            }
            continue;
        }

        if (strcmp(cmd, "quit") == 0) {
            // MUDAR O DESTINATARIO
            RESPOSTA r;
            strcpy(r.str, "QUIT");
            write(feed_pipe, &r, sizeof(RESPOSTA));
            pthread_join(thread_server, NULL);
            break;
        }
    }
    // FIM LOOP

    // AGUARDA A THREAD_SERVER TERMINAR
    pthread_join(thread_server, NULL);

    // FECHA OS PIPES E APAGA OS FIFOS
    close(feed_pipe);
    close(man_pipe_log);
    unlink(fifo_feed);

    printf("\nFIM FEED...\n");
    return 0;
}
