#include "includes.h"

// ENVIA UM SINAL PARA TERMINAR A THREAD
void sinal_termina(int sig, siginfo_t *si, void *u) {
    if (sig == SIGUSR2) {
        printf("\n[SINAL] Encerrando thread de login...");
        fflush(stdout);
    }
}


// ENVIA UMA "RESPOSTA" A UM USER
bool envia_info(const int pid, const char str[]) {
    // VARIAVEIS AUXILIARES
    RESPOSTA r;
    char fifo_feed[TAM];
    int feed_pipe;
    int tam;

    // CRIA/ABRE FIFO
    sprintf(fifo_feed, FIFO_CLI, pid);
    feed_pipe = open(fifo_feed, O_WRONLY);

    //ENVIA MENSAGEM DE TERMINO
    strcpy(r.str, str);
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
    DATA *d = (DATA *) data;
    LOGIN l;
    RESPOSTA r;
    char fifo_feed[TAM];
    int feed_pipe, man_pipe_log, nb;
    // FIM VARIAVEIS

    // CONFIGURAÇÃO DO SINAL
    struct sigaction sa;
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = sinal_termina;
    sigaction(SIGUSR2, &sa, NULL);
    //...

    // CRIA O FIFO_SERV_LOG E ABRE
    mkfifo(FIFO_SERV_LOG, 0600);
    man_pipe_log = open(FIFO_SERV_LOG, O_RDWR);
    if (man_pipe_log < 0) {
        perror("Erro ao abrir FIFO_SERV_LOG");
        return NULL;
    }
    // ABRE PARA LEITURA E ESCRITA PARA FUNCIONAR SEM USERS

    // CICLO QUE FICA A RECEBER OS LOGINS
    while (d->login_on) {
        // LÊ O FIFO_SERV_LOG

        nb = read(man_pipe_log, &l, sizeof(LOGIN));
        if (nb != sizeof(LOGIN)) {
            // Se o sinal for recebido e o FIFO for interrompido, saia do loop
            if (errno == EINTR) {
                printf("\n[THREAD LOGIN] Interrompido pelo sinal.\n");
                break;
            }
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


    close(man_pipe_log);
    unlink(FIFO_SERV_LOG);
    printf("\n[THREAD LOGIN] Encerrada com sucesso.\n");

    return NULL;
}

//...

// FUNCAO DA THREAD_MSG QUE FICA A RECEBER AS MENSAGENS
void *recebe_msg(void *data) {
    MENSAGEM m;
    DATA *d = (DATA *)data;

    // Inicializa as estruturas de mensagens e tópicos
    memset(m.topico, 0, TAM_TOPICO);
    memset(m.corpo_msg, 0, TAM_MSG);
    m.duracao = 0;

    // Criação e abertura do FIFO para mensagens
    if (mkfifo(FIFO_SERV_MSG, 0600) < 0 && errno != EEXIST) {
        perror("Erro ao criar FIFO_SERV_MSG");
        return NULL;
    }
    int man_pipe_msg = open(FIFO_SERV_MSG, O_RDWR);
    if (man_pipe_msg < 0) {
        perror("Erro ao abrir FIFO_SERV_MSG");
        return NULL;
    }

    while (d->msg_on) {
        int tam = read(man_pipe_msg, &m, sizeof(MENSAGEM));
        if (tam <= 0) {
            perror("Erro ao ler do FIFO_SERV_MSG");
            break;
        }

        // Checa se a mensagem é para encerrar a thread
        if (strcmp(m.topico, "desliga") == 0 && m.pid == getpid()) {
            printf("Mensagem de desligamento recebida. Encerrando thread.\n");
            break;
        }

        printf("Tópico recebido: %s\nMensagem recebida: %s\nDuração: %d\n",
               m.topico, m.corpo_msg, m.duracao);

        // Armazena a mensagem no tópico apropriado
        int topico_encontrado = 0;
        for (int i = 0; i < TAM_TOPICO; i++) {
            if (strcmp(d->topicos[i].nome_topico, m.topico) == 0) {
                topico_encontrado = 1;
                for (int j = 0; j < MAX_MSG_PER; j++) {
                    if (d->topicos[i].mensagens[j].corpo_msg[0] == '\0') {
                        // Armazena a mensagem no slot disponível
                        strcpy(d->topicos[i].mensagens[j].corpo_msg, m.corpo_msg);
                        d->topicos[i].mensagens[j].duracao = m.duracao;
                        break;
                    }
                }
                break;
            } else if (d->topicos[i].nome_topico[0] == '\0') {
                // Registra um novo tópico
                strcpy(d->topicos[i].nome_topico, m.topico);
                strcpy(d->topicos[i].mensagens[0].corpo_msg, m.corpo_msg);
                d->topicos[i].mensagens[0].duracao = m.duracao;
                topico_encontrado = 1;
                break;
            }
        }

        if (!topico_encontrado) {
            printf("[AVISO] Número máximo de tópicos atingido. Mensagem descartada.\n");
        }

        // Exibe os tópicos e mensagens armazenados
        printf("\n[Estado Atual]\n");
        for (int i = 0; i < TAM_TOPICO; i++) {
            if (d->topicos[i].nome_topico[0] != '\0') {
                printf("Tópico: %s\n", d->topicos[i].nome_topico);
                for (int j = 0; j < MAX_MSG_PER; j++) {
                    if (d->topicos[i].mensagens[j].corpo_msg[0] != '\0') {
                        printf("  Mensagem: %s (Duração: %d)\n",
                               d->topicos[i].mensagens[j].corpo_msg,
                               d->topicos[i].mensagens[j].duracao);
                    }
                }
            }
        }
    }

    // Fechamento e limpeza
    close(man_pipe_msg);
    unlink(FIFO_SERV_MSG);

    return NULL;
}

//...

// MOSTRA OS USERNAMES E OS RESPETIVOS PIDS DE TODOS
void mostra_users(DATA *d) {
    pthread_mutex_lock(d->ptrinco);
    int n = d->nUsers;
    pthread_mutex_unlock(d->ptrinco);

    printf("\nUSERS(%d):\n", n);
    for (int i = 0; i < n; i++) {
        pthread_mutex_lock(d->ptrinco);
        printf(" %d: %s | %d\n", i + 1, d->users_names[i], d->users_pids[i]);
        pthread_mutex_unlock(d->ptrinco);
    }

    //printf("\n\n");
}

// MOSTRA OS USERNAMES E OS RESPETIVOS PIDS DE TODOS
void mostra_topicos(DATA *d) {
    pthread_mutex_lock(d->ptrinco);
    int n = d->nTopicos;
    pthread_mutex_unlock(d->ptrinco);

    printf("\nTOPICOS(%d):\n", n);
    for (int i = 0; i < n; i++) {
        pthread_mutex_lock(d->ptrinco);
        printf(" %d: %s \n", i + 1, d->topicos[i].nome_topico);
        pthread_mutex_unlock(d->ptrinco);
    }

    //printf("\n\n");
}

int main(int argc, char *argv[]) {
    if (argc != 1) {
        return 1;
    }

    // EXISTE SERVIDOR ON?(SO PODE EXISTIR UM)
    if (access(FIFO_SERV_MSG,F_OK) == 0 || access(FIFO_SERV_MSG,F_OK) == 0) {
        printf("[ERRO]-SERVER ON\n");
        exit(3);
    }

    // DATA
    DATA data;
    data.nUsers = 0;
    data.nTopicos = 0;
    data.login_on = true;
    data.msg_on = true;
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

    pthread_t thread_msg;
    pthread_create(&thread_msg, NULL, recebe_msg, &data);
    //...

    // CICLO PRINCIPAL
    char cmd[TAM];
    do {
        printf("\nADMIN CMD> ");
        fflush(stdout);
        fgets(cmd, TAM, stdin);
        cmd[strcspn(cmd, "\n")] = '\0';
        printf("A executar '%s' ... \n", cmd);

        if (strcmp(cmd, "users") == 0) {
            mostra_users(&data);
            continue;
        }

        if (strcmp(cmd, "topics") == 0) {
            mostra_topicos(&data);
            continue;
        }

        //RETIRA TODOS OS USER LIGADOS AO SERVER
        if (strcmp(cmd, "remove all") == 0) {

            pthread_mutex_lock(data.ptrinco);
            int n = data.nUsers;
            pthread_mutex_unlock(data.ptrinco);

            for (int i = n - 1; i >= 0; i--) {

                pthread_mutex_lock(data.ptrinco);
                // >>>>>>>>>>>>>ALTERAR "RESPOSTA"<<<<<<<<<<<<<
                if (envia_info(data.users_pids[i], "QUIT")) {
                    data.nUsers--;
                } else {
                    printf(" NAO DEU!\n");
                }
                pthread_mutex_unlock(data.ptrinco);

            }
            continue;
        }

        //RETIRA UM USER LIGADOS AO SERVER
        if (strcmp(cmd, "remove") == 0) {

            // >>>>>>>>PROVISÓRIO<<<<<<<<
            printf("Name: ");
            fflush(stdout);
            char user_name[TAM_NOME];
            fgets(user_name, TAM, stdin);
            user_name[strcspn(user_name, "\n")] = '\0';
            // >>>>>>>>PROVISÓRIO<<<<<<<<

            pthread_mutex_lock(data.ptrinco);
            int n = data.nUsers;
            //copiar tabela user_names???
            pthread_mutex_unlock(data.ptrinco);

            for (int i = n - 1; i >= 0; i--) {

                pthread_mutex_lock(data.ptrinco);
                if(strcmp(data.users_names[i],user_name) == 0) {

                    // >>>>>>>>>>>>>ALTERAR "RESPOSTA"<<<<<<<<<<<<<
                    if (envia_info(data.users_pids[i], "QUIT")) {
                        data.nUsers--;
                    } else {
                        printf(" NAO DEU!\n");
                    }
                }
                pthread_mutex_unlock(data.ptrinco);

            }
            continue;
        }

        if (strcmp(cmd, "desligaL") == 0) {
            // Sinaliza que o login deve parar
            data.login_on = false;

            // Envia o sinal SIGUSR2 para a thread de login
            pthread_kill(thread_login, SIGUSR2);

            // Aguarda a thread terminar
            pthread_join(thread_login, NULL);

            printf("[MAIN] Thread de login encerrada.\n");
            continue;
        }

        if (strcmp(cmd, "ligaL") == 0) {

            if(data.login_on == false) {
                pthread_create(&thread_login, NULL, executa_login, &data);
                data.login_on = true;
            }
            else {
                printf("Os Logins já estão a funcionar\n");
            }
            continue;
        }

        if (strcmp(cmd, "desligaM") == 0) {
            // Sinaliza que o login deve parar
            data.msg_on = false;

            MENSAGEM m;
            strcpy(m.topico,"desliga");
            m.pid = getpid();

            int man_pipe_msg = open(FIFO_SERV_MSG, O_WRONLY);
            write(man_pipe_msg, &m, sizeof(MENSAGEM));

            // Aguarda a thread terminar
            pthread_join(thread_login, NULL);

            printf("[MAIN] Thread de login encerrada.\n");
            continue;
        }

        if (strcmp(cmd, "quit") == 0) {

            pthread_mutex_lock(data.ptrinco);
            int n = data.nUsers;
            pthread_mutex_unlock(data.ptrinco);

            for (int i = n - 1; i >= 0; i--) {

                pthread_mutex_lock(data.ptrinco);
                // >>>>>>>>>>>>>ALTERAR "RESPOSTA"<<<<<<<<<<<<<
                if (envia_info(data.users_pids[i], "QUIT")) {
                    data.nUsers--;
                } else {
                    printf(" NAO DEU!\n");
                }
                pthread_mutex_unlock(data.ptrinco);

            }

            // Sinaliza que o login deve parar
            data.login_on = false;

            // Envia o sinal SIGUSR2 para a thread de login
            pthread_kill(thread_login, SIGUSR2);

            // Aguarda a thread terminar
            pthread_join(thread_login, NULL);

            printf("[MAIN] Thread de login encerrada.\n");
            continue;
        }

    } while (strcmp(cmd, "quit") != 0);


    // FECHA PIPES & APAGA FIFO
    //close(man_pipe);
    unlink(FIFO_SERV_MSG);
    unlink(FIFO_SERV_LOG);

    printf("\nFIM MANAGER...\n");

    return 0;
}
