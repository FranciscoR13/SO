#include "includes.h"

bool fecha_login(const int pid) {
    // VARIAVEIS AUXILIARES
    RESPOSTA r;
    char fifo_feed[TAM];
    int feed_pipe;
    int tam;

    // CRIA/ABRE FIFO
    sprintf(fifo_feed, FIFO_CLI, pid);
    feed_pipe = open(fifo_feed, O_WRONLY);

    //ENVIA MENSAGEM DE TERMINO
    strcpy(r.str, "FECHOU");
    tam = write(feed_pipe, &r, sizeof(RESPOSTA));

    // FECHA E RETORNA
    if (tam == sizeof(RESPOSTA)) {
        close(feed_pipe);
        return true;
    }
    close(feed_pipe);
    return false;
}

bool verifica_login(const LOGIN *l, const THREAD_LOGIN *tl) {
    //verificar se o username ja existe
    for (int i = 0; i < tl->nUsers; i++) {
        if (strcmp(tl->users_names[i], l->username) == 0 || tl->users_pids[i] == l->pid) {
            return false;
        }
    }
    return true;
}

void *executa_login(void *thread_login) {
    THREAD_LOGIN *tl = (THREAD_LOGIN *) thread_login;
    LOGIN l;
    RESPOSTA r;

    char fifo_feed[TAM];
    int feed_pipe;

    while (tl->nUsers < 20) {
        // Lê dados do pipe
        if (read(tl->man_pipe, &l, sizeof(LOGIN)) != sizeof(LOGIN)) {
            perror("Erro ao ler dados de login");
            continue;  // Continua a próxima iteração se ocorrer um erro
        }

        // Prepara o nome do FIFO
        snprintf(fifo_feed, TAM, FIFO_CLI, l.pid);

        // Abre o FIFO para enviar resposta
        feed_pipe = open(fifo_feed, O_WRONLY);
        if (feed_pipe < 0) {
            perror("Erro ao abrir FIFO");
            continue;  // Tenta novamente na próxima iteração
        }

        // Verifica se o login é válido
        if (verifica_login(&l, tl)) {
            // Login bem-sucedido
            strcpy(r.str, "OK");
            if (write(feed_pipe, r.str, sizeof(RESPOSTA)) == sizeof(RESPOSTA)) {
                // Adiciona o usuário às listas
                snprintf(tl->users_names[tl->nUsers], TAM_NOME, "%s", l.username);
                tl->users_pids[tl->nUsers] = l.pid;
                tl->nUsers++;
            }
        } else {
            // Login falhou
            strcpy(r.str, "FAIL");
            write(feed_pipe, r.str, sizeof(RESPOSTA));
        }

        // Fecha o FIFO
        close(feed_pipe);
    }

    return NULL;
}


void *recebe_msg(void *thread_msg) {
    THREAD_MSG *tm = (THREAD_MSG *) thread_msg;
    MENSAGEM m;
    RESPOSTA r;

    int tam;
    char fifo_feed[TAM];
    int feed_pipe;

    do {
        tam = read(*tm->man_pipe, &m, sizeof(MENSAGEM));
        if (tam == sizeof(MENSAGEM)) {
            printf("\n\nMENSAGEM DE [%d] | TOPICO[%s] | MSG[%s]\n", m.pid, m.topico, m.corpo_msg);

            // CONSTROI NOME/ABRE/RESPONDE/FECHA FIFO_CLI
            sprintf(fifo_feed, FIFO_CLI, m.pid);
            feed_pipe = open(fifo_feed, O_WRONLY);

            strcpy(r.str, "OK");
            tam = write(feed_pipe, &r, sizeof(RESPOSTA));
            if (tam > 0) {
                printf("ENVIEI... '%s'\n\n", r.str);
                close(feed_pipe);
            }
        }
    } while (tm->ligado);

    int n = tm->thread_login->nUsers;
    for (int i = 0; i < n; i++) {
        if (fecha_login(tm->thread_login->users_pids[i])) {
            tm->thread_login->users_pids[i] = tm->thread_login->users_pids[n - 1];
            tm->thread_login->nUsers--;
        }
    }

    return NULL;
}

// MOSTRA OS USERNAMES E OS RESPETIVOS PIDS DE TODOS
void mostra_users(THREAD_LOGIN *tl) {
    printf("\n\nUSERS(%d):\n", tl->nUsers);
    for (int i = 0; i < tl->nUsers; i++) {
        printf(" %d: %s | %d\n", i+1, tl->users_names[i], tl->users_pids[i]);
    }
    printf("\n\n");
}

int main(int argc, char *argv[]) {
    if (argc != 1) {
        return 1;
    }

    // VARIAVEIS
    char cmd[TAM];
    int man_pipe;

    // EXISTE SERVIDOR ON?(SO PODE EXISTIR UM)
    if (access(FIFO_SERV,F_OK) == 0) {
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
    pthread_t thread_login_id;
    THREAD_LOGIN thread_login;

    thread_login.nUsers = 0;
    thread_login.man_pipe = man_pipe;

    pthread_create(&thread_login_id, NULL, executa_login, &thread_login);
    // FIM LOGIN

    // MSG
    pthread_t thread_msg_id;
    THREAD_MSG thread_msg;

    thread_msg.man_pipe = &man_pipe;
    thread_msg.thread_login = &thread_login;
    pthread_create(&thread_msg_id, NULL, recebe_msg, &thread_msg);
    // FIM MSG

    // CICLO PRINCIPAL
    do {
        printf("ADMIN CMD> ");
        fflush(stdout);
        scanf("%s", cmd);
        printf("A executar '%s' ... \n", cmd);

        if (strcmp(cmd, "para") == 0) {
            thread_msg.ligado = false;
            int n = thread_login.nUsers;
            for (int i = 0; i < n; i++) {
                if (fecha_login(thread_login.users_pids[i])) {
                    thread_login.users_pids[i] = thread_login.users_pids[n - 1];
                    thread_login.nUsers--;
                }
            }
        }

        if (strcmp(cmd, "users") == 0) {
            mostra_users(&thread_login);
        }

    } while (strcmp(cmd, "quit") != 0); // TEMPORARIO


    // FECHA PIPES & APAGA FIFO
    close(man_pipe);
    unlink(FIFO_SERV);

    printf("\nFIM MANAGER...\n");

    return 0;
}
