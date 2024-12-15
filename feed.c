#include "includes.h"


void timeout(int sig, siginfo_t *si, void *u) {
    printf("\n[ERRO] Timeout ao aguardar resposta do manager\n");
}

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
    if (tam != sizeof(PEDIDO)) {
        perror("[ERRO] Falha ao escrever no pipe do manager");
        return false;
    }

    // TRATAR SINAL TIMEOUT
    struct sigaction sa;
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = timeout;
    sigaction(SIGALRM, &sa, NULL);

    alarm(30);

    tam = read(feed_pipe, &r, sizeof(RESPOSTA));
    if (tam != sizeof(RESPOSTA) && tam != -1) {
        printf("[ERRO] %d Falha ao ler do pipe do feed", tam);
        return false;
    }

    alarm(0);

    if(tam == -1) {
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
bool envia_pedido(int man_pipe, int feed_pipe, int tipo) {
    // VARIAVEIS
    PEDIDO p;
    char msg[TAM_MSG], topic[TAM_TOPICO];
    int tam;
    // FIM VARIAVEIS

    // Limpa buffers de entrada para evitar resíduos
    int c;
    while ((c = getchar()) != '\n' && c != EOF);

    // MENSAGEM
    if(tipo == 2) {

        MENSAGEM m;

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



        return true;
    }

    if(tipo == 3) {

        RESPOSTA r;
        r.pid = getpid();
        strcpy(r.str, "TOPICS");

        p.tipo = 3;
        p.r = r;

        tam = write(man_pipe, &p, sizeof(PEDIDO));
        if (tam != sizeof(PEDIDO)) {
            perror("[ERRO] Falha ao enviar a mensagem ao servidor");
            return false;
        }

        tam = write(feed_pipe, &p, sizeof(PEDIDO));
        if (tam != sizeof(PEDIDO)) {
            perror("[ERRO] Falha ao enviar a mensagem ao servidor");
            return false;
        }

        return true;
    }

    if (tipo == 4)
    {
        TOPICO t;
        RESPOSTA r;
        strcpy(r.str, "subscribe");

        

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

        strcpy(t.nome_topico, topic);
        t.upid = getpid();

        p.tipo = 4;

        p.t = t;

        tam =write(man_pipe, &p, sizeof(PEDIDO));

        return true;

    }

    if (tipo == 5)
    {
        TOPICO t;
        RESPOSTA r;
        strcpy(r.str, "unsubscribe");

        

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

        strcpy(t.nome_topico, topic);
        t.upid = getpid();

        p.tipo = 5;

        p.t = t;

        tam =write(man_pipe, &p, sizeof(PEDIDO));

        return true;

    }
    


    return false;
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
                    if (envia_pedido(man_pipe, feed_pipe, 2)) {
                        printf("[INFO] Mensagem enviada com sucesso.\n");
                    } else {
                        printf("[ERRO] Falha ao enviar a mensagem. Tente novamente.\n");
                    }
                    continue;
                }

                if (strcmp(cmd, "topics") == 0) {
                    if (envia_pedido(man_pipe, feed_pipe, 3)) {
                        printf("[INFO] Topicos recebidos com sucesso.\n");
                    } else {
                        printf("[ERRO] Falha ao enviar a mensagem. Tente novamente.\n");
                    }
                    continue;
                }

                if (strcmp(cmd, "subscribe") == 0)
                {
                    if (envia_pedido(man_pipe, feed_pipe, 4))
                    {
                        printf("[INFO] Topico subscrito com sucesso.\n");
                    } else
                    {
                        printf("[ERRO] Falha ao subscrever no topico. Tente novamente.\n");
                    }
                    continue;
                    
                }
                
                if (strcmp(cmd, "unsubscribe") == 0)
                {
                    if (envia_pedido(man_pipe, feed_pipe, 5))
                    {
                        printf("[INFO] Deixaste de seguir o topico com sucesso.\n");
                    } else
                    {
                        printf("[ERRO] Falha ao deixar de seguir o topico. Tente novamente.\n");
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
                printf("  - subscribe: Subscreve em um topico.\n");
                printf("  - unsubscribe: Cancela a subscricão em um topico.\n");
                printf("  - topics: Mostra todos os topicos disponiveis.\n");
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
                        if (strcmp(p.r.str, "TOPICS") == 0) {

                            alarm(30);

                            int tam = read(feed_pipe, &p, sizeof(PEDIDO));

                            alarm (0);

                            if(tam != sizeof(PEDIDO)) {
                                break;
                            }

                            printf("==Tópicos==\n");

                            char topicos[TAM * MAX_TOPICS];
                            strncpy(topicos, p.r.str, sizeof(topicos) - 1);
                            // Divide a string em tokens usando " - " como delimitador
                            char *token = strtok(topicos, " - ");
                            int i = 0;
                            while (token != NULL) {
                                i++;
                                printf(" %d. %s\n",i, token); // Mostra cada tópico em uma nova linha
                                token = strtok(NULL, " - ");
                            }

                            break;
                        }
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
