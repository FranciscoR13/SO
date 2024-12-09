#include "includes.h"

// ENVIA UMA "RESPOSTA" A UM USER
bool envia_info(const int pid) {
    // VARIAVEIS AUXILIARES
    RESPOSTA r;
    char fifo_feed[TAM];
    int feed_pipe;
    int tam;

    // CRIA/ABRE FIFO
    sprintf(fifo_feed, FIFO_CLI, pid);
    feed_pipe = open(fifo_feed, O_WRONLY);

    //ENVIA MENSAGEM DE TERMINO(TEMPORARIO)
    strcpy(r.str, "QUIT");
    tam = write(feed_pipe, &r, sizeof(RESPOSTA));

    // FECHA E RETORNA
    if (tam == sizeof(RESPOSTA)) {
        close(feed_pipe);
        return true;
    }
    close(feed_pipe);
    return false;
}
//...

// FUNCAO DA THREAD_LOG QUE FICA A RECEBER OS USERS
void *executa_login(void *data) {

    // VARIAVEIS
    DATA *d = (DATA *)data;
    LOGIN l;
    RESPOSTA r;
    char fifo_feed[TAM];
    int feed_pipe, man_pipe_log, nb;
    // FIM VARIAVEIS

    // CRIA O FIFO_SERV_LOG E ABRE
    mkfifo(FIFO_SERV_LOG, 0600);
    man_pipe_log = open(FIFO_SERV_LOG, O_RDWR);
    if (man_pipe_log < 0) {
        perror("Erro ao abrir FIFO_SERV_LOG");
        return NULL;
    }
    // ABRE PARA LEITURA E ESCRITA PARA FUNCIONAR SEM USERS

    // CICLO QUE FICA A RECEBER OS LOGINS
    // >>>>>>ALTERAR<<<<<<
    while (1) {

        // LÊ O FIFO_SERV_LOG
        nb = read(man_pipe_log, &l, sizeof(LOGIN));
        if (nb != sizeof(LOGIN)) {
            perror("\nErro ao ler dados de login\n");
            continue;
        }
        // CONTINUA SE RECEBER O LOGIN CORRETAMENTE

        // CONSTROI O NOMEDO FIFO_CLI
        snprintf(fifo_feed, TAM, FIFO_CLI, l.pid);
        feed_pipe = open(fifo_feed, O_WRONLY);
        if (feed_pipe < 0) {
            perror("\nErro ao abrir FIFO do cliente\n");
            continue;
        }
        // CONTINUA SE CORREU BEM

        // CONSTROI O NOME DO FIFO_CLI
        snprintf(fifo_feed, TAM, FIFO_CLI, l.pid);
        feed_pipe = open(fifo_feed, O_WRONLY);
        if (feed_pipe < 0) {
            perror("\nErro ao abrir FIFO do cliente\n");
            continue;
        }
        // VAI SER USADO PARA RETORNAR A RESPOSTA

        // VERIFICA SE O USERNAME JA EXISTE
        bool valido = true;
        //pthread_mutex_lock(d->ptrinco); ????????????????
        for (int i = 0; i < d->nUsers; i++) {

            // MUTEX QUE TRANCA O ACESSO
            pthread_mutex_lock(d->ptrinco);
            if (strcmp(d->users_names[i], l.username) == 0 || d->users_pids[i] == l.pid) {
                valido = false;
                break;
            }
            pthread_mutex_unlock(d->ptrinco);
            // DESTRANCA O ACEESO AOS DADOS

        }
        // SE FOR VALIDO CONTINUA A TRUE

        // ENVIA A RESPOSTA AO USER
        if (valido) {
            strcpy(r.str, "SUCCESS");
            write(feed_pipe, &r, sizeof(RESPOSTA));

            // ADICIONA O USERNAME E O PID AOS DADOS
            pthread_mutex_lock(d->ptrinco);
            snprintf(d->users_names[d->nUsers], TAM_NOME, "%s", l.username);
            d->users_pids[d->nUsers++] = l.pid;
            pthread_mutex_unlock(d->ptrinco);

        } else {
            strcpy(r.str, "FAIL");
            write(feed_pipe, &r, sizeof(RESPOSTA));
        }
        close(feed_pipe);
        // INDEPENDENTE DA RESPOSTA O PIPE É FECHADO
    }

    //>>>>>>>AINDA FALTA FECHAR ESTA THREAD CORRETAMENTE<<<<<<<<
    close(man_pipe_log);
    unlink(FIFO_SERV_LOG);

    return NULL;
}
//...

// FUNCAO DA THREAD_MSG QUE FICA A RECEBER AS MENSAGENS
void* revecebe_msg(void* data) {

    //VARIAVEIS

    //FIM VARIAVEIS

    return NULL;
}
//...

// MOSTRA OS USERNAMES E OS RESPETIVOS PIDS DE TODOS
void mostra_users(DATA *d) {

    pthread_mutex_lock(d->ptrinco);
    int n = d->nUsers;
    pthread_mutex_unlock(d->ptrinco);

    printf("\n\nUSERS(%d):\n", n);
    for (int i = 0; i < n; i++) {
        pthread_mutex_lock(d->ptrinco);
        printf(" %d: %s | %d\n", i+1, d->users_names[i], d->users_pids[i]);
        pthread_mutex_unlock(d->ptrinco);
    }

    printf("\n\n");
}

int main(int argc, char *argv[]) {
    if (argc != 1) {
        return 1;
    }

    // EXISTE SERVIDOR ON?(SO PODE EXISTIR UM)
    if (access(FIFO_SERV,F_OK) == 0) {
        printf("[ERRO]-SERVER ON\n");
        exit(3);
    }

    // DATA
    DATA data;
    data.nUsers = 0;
    pthread_mutex_t trinco;
    data.ptrinco = &trinco;
    // FIM DATA

    // MENSAGENS INICIAIS
    printf("INICIO MANAGER...\n");
    printf("ESPERANDO USERS...\n");

    // THREAD LOGIN
    pthread_t thread_login;
    pthread_create(&thread_login, NULL, executa_login, &data);
    //...

    // MSG

    //...

    // CICLO PRINCIPAL
    char cmd[TAM];
    do {

        printf("ADMIN CMD> ");
        fflush(stdout);
        scanf("%s", cmd);
        printf("A executar '%s' ... \n", cmd);

        if (strcmp(cmd, "users") == 0) {
            mostra_users(&data);
            continue;
        }

        //RETIRA TODOS OS USER LIGADOS AO SERVER
        if (strcmp(cmd, "para") == 0 || strcmp(cmd, "quit") == 0) {
            for(int i=data.nUsers-1; i>=0; i--) {
                if(envia_info(data.users_pids[i])) {
                    data.nUsers--;
                }else {
                    printf(" NAO DEU!\n");
                }
            }
            continue;
        }

    } while (strcmp(cmd, "quit") != 0);


    // FECHA PIPES & APAGA FIFO
    //close(man_pipe);
    unlink(FIFO_SERV);
    unlink(FIFO_SERV_LOG);

    printf("\nFIM MANAGER...\n");

    return 0;
}
