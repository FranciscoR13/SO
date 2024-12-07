#include "includes.h"

// Comando 'topics': mostra todos os tópicos, se não existirem topicos da erro
void listar_topicos(int num_topicos) {
    if (num_topicos == 0) {
        printf("Não existem tópicos cadastrados.\n");
        return;
    }

    //resto do codigo para fazer o comando funcionar
}

// ESTRUTURA PARA PARTILHAR OS DADOS COM THREAD
typedef struct {
    pthread_t main_thread; // ID da thread principal
    int feed_pipe;         // Pipe para comunicação
    int thread_finished;   // Flag para saber se a thread acabou
} ThreadData;

// SINAL PARA AVISAR A THREAD PRINCIPAL
void para_recebe_inf(int sig) {
    if (sig == SIGUSR1) {
        printf("\n[NOTIFICAÇÃO] Foste expulso do server.\n");
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
void* recebe_info(void* arg) {

    // ESTRUTURA COM OS DADOS
    ThreadData* data = (ThreadData*)arg;

    while (1) {
        RESPOSTA r;
        
        int nb = read(data->feed_pipe, &r, sizeof(RESPOSTA));
        if (nb != sizeof(RESPOSTA)) {
            perror("\nErro ao ler dados do feed");
            continue;
        }

        if (strcmp(r.str, "QUIT") == 0) {
            data->thread_finished = 1;                   // Marca como finalizada
            pthread_kill(data->main_thread, SIGUSR1);    // Notifica a thread principal
            return NULL;                                 // Sai imediatamente da thread
        }

        // AINDA POR IMPLEMENTAR...
        printf("[Mensagem do Server]: %s\n", r.str);
    }

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
    ThreadData thread_data;
    //...

    // CONFIGURAÇÃO DO SINAL
    struct sigaction sa;
    sa.sa_handler = para_recebe_inf;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
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
    printf("\n[LOGIN CONCLUIDO]\n Bem-vindo %s! (PID: %d)\n", argv[1], pid);
    // CONTINUA SE O LOGIN FOI CONCLUIDO

    // CONFIGURACAO DA ESTRUTURA DE DADOS
    thread_data.main_thread = pthread_self();
    thread_data.feed_pipe = feed_pipe;
    thread_data.thread_finished = 0;

    // CRIA A THREAD QUE RECEBE INFO DO SERVER
    pthread_t thread_server;
    pthread_create(&thread_server, NULL, recebe_info, &thread_data);

    // LOOP DOS COMANDOS
    while (!thread_data.thread_finished) {
        printf("%s CMD> ", argv[1]);
        fflush(stdout);
        if (scanf("%s", cmd) == EOF) break;

        if (strcmp(cmd, "msg") == 0) {
            if (envia_msg(man_pipe_log, feed_pipe)) {

            }
            continue;
        }

        if (strcmp(cmd, "quit") == 0) {
            RESPOSTA r;
            strcpy(r.str, "QUIT");
            write(feed_pipe, &r, sizeof(RESPOSTA));
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

    printf("\nFim Feed...\n");
    return 0;
}
