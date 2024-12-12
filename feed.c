#include "includes.h"

// FUNCAO DE LOGIN QUE ENVIA O USERNAME E O PID
// FUNCAO DE LOGIN QUE ENVIA O USERNAME E O PID
bool login(int man_pipe, int feed_pipe, char *username, int pid) {
    // VARIAVEIS
    int tam;
    PEDIDO p;
    LOGIN l;
    RESPOSTA r;

    // ESTRUTURA LOGIN
    strncpy(l.username, username, sizeof(l.username) - 1);
    l.username[sizeof(l.username) - 1] = '\0'; // Garantir que a string seja terminada
    l.pid = pid;

    // ESTRUTURA PEDIDO LOGIN
    p.tipo = 1;
    p.l = l;

    // TENTA ENVIAR O LOGIN
    tam = write(man_pipe, &p, sizeof(PEDIDO));
    if (tam < 0) {
        perror("[ERRO] Falha ao escrever no pipe do manager");
        return false;
    } else if (tam != sizeof(PEDIDO)) {
        fprintf(stderr, "[ERRO] Escrita incompleta no pipe do manager\n");
        return false;
    }

    // TENTA RECEBER A RESPOSTA DO MANAGER
    tam = read(feed_pipe, &r, sizeof(RESPOSTA));
    if (tam < 0) {
        perror("[ERRO] Falha ao ler do pipe do feed");
        return false;
    } else if (tam != sizeof(RESPOSTA)) {
        fprintf(stderr, "[ERRO] Leitura incompleta do pipe do feed\n");
        return false;
    }

    // VERIFICA A RESPOSTA
    if (strcmp(r.str, "FAIL") == 0) {
        fprintf(stderr, "[LOGIN FALHOU] Username ou PID inválidos\n");
        return false;
    }

    return true;
}


// FUNCAO QUE ENVIA MENSAGEM
bool envia_msg(int man_pipe, int feed_pipe) {
    // VARIAVEIS
    PEDIDO p;
    MENSAGEM m;
    //RESPOSTA r;
    char msg[TAM_MSG], topic[TAM_TOPICO];
    int tam;
    // FIM VARIAVEIS

    // Limpa buffers de entrada para evitar resíduos
    int c;
    while ((c = getchar()) != '\n' && c != EOF);

    // Solicita o tópico ao usuário
    printf("\nTOPICO: ");
    fflush(stdout);
    if (fgets(topic, TAM_TOPICO, stdin) == NULL) {
        perror("[ERRO] Falha ao ler o tópico");
        return false;
    }
    topic[strcspn(topic, "\n")] = '\0'; // Remove o '\n'

    if (strlen(topic) == 0) {
        printf("[AVISO] Tópico não pode estar vazio!\n");
        return false;
    }

    // Solicita a mensagem ao usuário
    printf("MENSAGEM: ");
    fflush(stdout);
    if (fgets(msg, TAM_MSG, stdin) == NULL) {
        perror("[ERRO] Falha ao ler a mensagem");
        return false;
    }
    msg[strcspn(msg, "\n")] = '\0'; // Remove o '\n'

    if (strlen(msg) == 0) {
        printf("[AVISO] Mensagem não pode estar vazia!\n");
        return false;
    }

    // Preenche a estrutura de mensagem
    m.pid = getpid();
    strncpy(m.topico, topic, sizeof(m.topico) - 1);
    m.topico[sizeof(m.topico) - 1] = '\0';
    strncpy(m.corpo_msg, msg, sizeof(m.corpo_msg) - 1);
    m.corpo_msg[sizeof(m.corpo_msg) - 1] = '\0';

    //>>>>>>>>>ALTERAR<<<<<<<<<<
    m.duracao = 0;
    //>>>>>>>>>ALTERAR<<<<<<<<<<


    //ESTRUTURA PEDIDO
    p.tipo = 2;
    p.m = m;

    // Envia a mensagem ao servidor
    tam = write(man_pipe, &p, sizeof(PEDIDO));
    if (tam != sizeof(PEDIDO)) {
        perror("[ERRO] Falha ao enviar a mensagem ao servidor");
        return false;
    }

    /*
    // Aguarda a resposta do servidor
    tam = read(feed_pipe, &r, sizeof(RESPOSTA));
    if (tam < 0) {
        perror("[ERRO] Falha ao ler a resposta do servidor");
        return false;
    } else if (tam != sizeof(RESPOSTA)) {
        fprintf(stderr, "[ERRO] Resposta incompleta do servidor\n");
        return false;
    }

    printf("[RESPOSTA DO SERVIDOR]: %s\n", r.str);
    */

    return true;
}


// MAIN
int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("\nNúmero de argumentos inválido!\n");
        return 1;
    }
    // TERMINAR SE O Nª ESTIVER INCORRETO

    // VERIFICA SE O SERVER ESTÁ ON
    if (access(FIFO_SERV, F_OK) != 0) {
        printf("[ERRO] Servidor está offline.\n");
        return 1;
    }

    // VARIAVEIS
    char cmd[TAM], fifo_feed[20];
    int man_pipe, feed_pipe, s;
    pid_t pid = getpid();
    fd_set fds;
    //...

    // CONFIGURA OS FIFOS
    snprintf(fifo_feed, sizeof(fifo_feed), FIFO_CLI, pid);
    mkfifo(fifo_feed, 0600);

    feed_pipe = open(fifo_feed, O_RDWR);
    if (feed_pipe < 0) {
        perror("[ERRO] Falha ao abrir FIFO_FEED");
        unlink(fifo_feed);
        return 1;
    }

    man_pipe = open(FIFO_SERV, O_WRONLY);
    if (man_pipe < 0) {
        perror("[ERRO] Falha ao abrir FIFO_SERV");
        close(feed_pipe);
        unlink(fifo_feed);
        return 1;
    }
    //...


    // MENSAGEM ABERTURA
    printf("INICIO FEED...\n");
    //...

    // FAZ O LOGIN
    if (!login(man_pipe, feed_pipe, argv[1], pid)) {
        printf("\n[ERRO] Falha no login.\n");
        close(feed_pipe);
        close(man_pipe);
        unlink(fifo_feed);
        return 2;
    }
    printf("\n[LOGIN CONCLUIDO]\nBem-vindo %s! (PID: %d)\n", argv[1], pid);
    // CONTINUA SE O LOGIN FOI CONCLUIDO


    // LOOP PRINCIPAL
    bool termina = false;
    bool linha_cmd = true;

    while (!termina) {
        FD_ZERO(&fds);
        FD_SET(0, &fds);
        FD_SET(feed_pipe, &fds);

        if(linha_cmd) {
            printf("\n%s CMD> ", argv[1]);
            fflush(stdout);
        }

        s = select(feed_pipe + 1, &fds, NULL, NULL, NULL);
        if (s < 0) {
            perror("\n[ERRO] Select falhou");
            continue;
        }

        if (s > 0) {
            if (FD_ISSET(0, &fds)) {
                if (scanf("%s", cmd) == EOF) break;

                if (strcmp(cmd, "msg") == 0) {
                    if (envia_msg(man_pipe, feed_pipe)) {
                        printf("[INFO] Mensagem enviada com sucesso.\n");
                    } else {
                        printf("[ERRO] Falha ao enviar a mensagem. Tente novamente.\n");
                    }
                    continue;
                }

                if (strcmp(cmd, "exit") == 0) {
                    RESPOSTA r;
                    r.pid = getpid();
                    strcpy(r.str, "EXIT");

                    PEDIDO p;
                    p.tipo = 3;
                    p.r = r;

                    int tam = write(man_pipe, &p, sizeof(PEDIDO));
                    if (tam < 0) {
                        printf("\n[ERRO] Falha ao sair. Tente novamente.\n");
                    } else {
                        printf("[INFO] Saindo do programa...\n");
                        linha_cmd = false;
                    }
                    continue;
                }

                printf("[AVISO] Comando não reconhecido. Comandos disponíveis:\n");
                printf("  - msg: Envia uma mensagem\n");
                printf("  - exit: Encerra o cliente\n");
            }

            if (FD_ISSET(feed_pipe, &fds)) {
                PEDIDO p;
                int n = read(feed_pipe, &p, sizeof(PEDIDO));
                if (n != sizeof(PEDIDO)) {
                    printf("\n[FEED] Erro ao ler o PEDIDO do servidor.\n");
                    continue;
                }

                switch (p.tipo) {
                    case 3:
                        if (strcmp(p.r.str, "CLOSE") == 0) {
                            printf("\n[FEED] O MANAGER encerrou. Fechando o cliente...\n");
                            termina = true;
                            break;
                        }
                        if (strcmp(p.r.str, "REMOVE") == 0) {
                            printf("\n[FEED] Você foi expulso. Encerrando o cliente...\n");
                            termina = true;
                            break;
                        }
                        if (strcmp(p.r.str, "EXIT") == 0) {
                            printf("\n[FEED] Encerrando o cliente...\n");
                            termina = true;
                            break;
                        }
                        break;

                    default:
                        printf("\n[FEED] Tipo de mensagem não reconhecido (%d). Ignorando...\n", p.tipo);
                        break;
                }
            }
        }
    }


    // FIM LOOP


    // FECHA OS PIPES E APAGA OS FIFOS
    close(feed_pipe);
    close(man_pipe);
    unlink(fifo_feed);

    printf("\nFIM FEED...\n");
    return 0;
}
