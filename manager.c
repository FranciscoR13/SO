#include "includes.h"

// ENVIA UMA "RESPOSTA" A UM USER
bool envia_info(const int pid, PEDIDO *p) {
    // Variáveis auxiliares
    char fifo_feed[TAM];
    int feed_pipe, tam;

    // Constrói o nome do FIFO
    sprintf(fifo_feed, FIFO_CLI, pid);

    // Tenta abrir o FIFO do cliente
    feed_pipe = open(fifo_feed, O_WRONLY);
    if (feed_pipe < 0) {
        perror("[ERRO] Não foi possível abrir o FIFO do cliente");
        return false;
    }

    // Envia a mensagem (conteúdo do PEDIDO)
    tam = write(feed_pipe, p, sizeof(PEDIDO));
    if (tam != sizeof(PEDIDO)) {
        perror("[ERRO] Falha ao enviar o pedido ao cliente");
        close(feed_pipe);
        return false;
    }

    // Fecha o FIFO e retorna sucesso
    close(feed_pipe);
    return true;
}

//...


void *recebe_pedidos(void *data) {
    // VARIAVEIS
    DATA *d = (DATA *) data; // CONVERTE * PARA ACEDER AOS DADOS
    PEDIDO p;
    int feed_pipe, man_pipe;
    char feed_fifo[TAM];

    // CONSTROI O FIFO
    if (mkfifo(FIFO_SERV, 0600) < 0) {
        perror("Erro ao criar FIFO_SERV");
        return NULL;
    }

    // ABRE O FIFO
    man_pipe = open(FIFO_SERV, O_RDWR);
    if (man_pipe < 0) {
        perror("Erro ao abrir FIFO_SERV_MSG");
        return NULL;
    }

    while (d->pedidos_on) {
        // LER PEDIDO DO FIFO
        int tam = read(man_pipe, &p, sizeof(PEDIDO));
        if (tam <= 0) {
            perror("Erro ao ler do FIFO_SERV_MSG");
            break;
        }

        // TRATA OS TIPOS DE PEDIDO
        switch (p.tipo) {
            case 1: {
                // LOGIN
                // CONSTRÓI O NOME DO FIFO_CLI
                snprintf(feed_fifo, TAM, FIFO_CLI, p.l.pid);
                feed_pipe = open(feed_fifo, O_WRONLY);
                if (feed_pipe < 0) {
                    perror("\nErro ao abrir FIFO do cliente\n");
                    continue;
                }

                // VERIFICA SE O USERNAME JÁ EXISTE
                bool valido = true;

                pthread_mutex_lock(d->ptrinco);
                for (int i = 0; i < d->nUsers; i++) {
                    if (strcmp(d->users_names[i], p.l.username) == 0 || d->users_pids[i] == p.l.pid) {
                        valido = false;
                        break;
                    }
                }
                pthread_mutex_unlock(d->ptrinco);

                // ENVIA RESPOSTA AO CLIENTE
                RESPOSTA res;
                if (valido) {
                    strcpy(res.str, "SUCCESS");
                    write(feed_pipe, &res, sizeof(RESPOSTA));

                    // ADICIONA O USERNAME E O PID AOS DADOS
                    pthread_mutex_lock(d->ptrinco);
                    snprintf(d->users_names[d->nUsers], TAM_NOME, "%s", p.l.username);
                    d->users_pids[d->nUsers++] = p.l.pid;
                    pthread_mutex_unlock(d->ptrinco);
                } else {
                    strcpy(res.str, "FAIL");
                    write(feed_pipe, &res, sizeof(RESPOSTA));
                }

                // FECHA O PIPE INDEPENDENTEMENTE
                close(feed_pipe);
                break;
            }

            case 2: {
                // MENSAGEM
                bool topico_encontrado = false;
                bool bloqueado = true;

                pthread_mutex_lock(d->ptrinco);

                for (int i = 0; i < d->nTopicos; i++) {
                    if (strcmp(d->topicos[i].nome_topico, p.m.topico) == 0) {
                        // Tópico encontrado
                        topico_encontrado = true;
                        if (!d->topicos[i].bloqueado) {
                            bloqueado = false;

                            // Enviar mensagem para todos os subscritores
                            for (int j = 0; j < d->topicos[i].nSubs; j++) {
                                if (d->topicos[i].subscritos_pid[j] != p.m.pid) {
                                    PEDIDO pp;
                                    pp.tipo = 2;
                                    pp.m = p.m;
                                    envia_info(d->topicos[i].subscritos_pid[j],&pp);
                                }
                            }

                            // Adicionar mensagem ao tópico (se não exceder o limite)
                            if (d->topicos[i].nMsgs < MAX_MSG_PER) {
                                strncpy(d->topicos[i].mensagens[d->topicos[i].nMsgs].corpo_msg, p.m.corpo_msg,
                                        TAM_MSG - 1);
                                d->topicos[i].mensagens[d->topicos[i].nMsgs].corpo_msg[TAM_MSG - 1] = '\0';
                                // Garantir null-termination
                                d->topicos[i].mensagens[d->topicos[i].nMsgs].duracao = p.m.duracao;
                                d->topicos[i].mensagens->pid = p.m.pid;
                                d->topicos[i].nMsgs++;
                            } else {
                                printf("[AVISO] Limite de mensagens por tópico atingido. Mensagem descartada.\n");
                            }
                        }

                        if(bloqueado) {
                            RESPOSTA r;
                            strcpy(r.str,"BLOQUEADO");
                            r.pid = getpid();

                            PEDIDO pr;
                            pr.tipo = 3;
                            pr.r = r;
                            envia_info(p.m.pid, &pr);
                        }


                        // Notificar cliente sobre o desbloqueio
                        if (!bloqueado) {
                            RESPOSTA r;
                            strcpy(r.str, "DESBLOQUEADO");
                            r.pid = getpid();

                            PEDIDO pr;
                            pr.tipo = 3;
                            pr.r = r;

                            envia_info(p.m.pid, &pr);
                        }

                        break; // Sai do loop ao encontrar o tópico
                    }
                }


                // SE O TÓPICO NÃO EXISTIR, CRIA UM NOVO
                if (!topico_encontrado && d->nTopicos < MAX_TOPICS) {
                    strncpy(d->topicos[d->nTopicos].nome_topico, p.m.topico, TAM_TOPICO);
                    strncpy(d->topicos[d->nTopicos].mensagens[0].corpo_msg, p.m.corpo_msg, TAM_MSG);
                    d->topicos[d->nTopicos].mensagens[0].duracao = p.m.duracao;
                    d->topicos[d->nTopicos].nMsgs = 1;
                    d->topicos[d->nTopicos].mensagens[0].pid = p.m.pid;
                    d->nTopicos++;
                } else if (!topico_encontrado) {
                    printf("[AVISO] Número máximo de tópicos atingido. Mensagem descartada.\n");
                }

                pthread_mutex_unlock(d->ptrinco);

                RESPOSTA r;
                strcpy(r.str, "DESBLOQUEADO");
                r.pid = getpid();

                PEDIDO pr;
                pr.tipo = 3;
                pr.r = r;

                envia_info(p.m.pid, &pr);

            }

            case 3: {
                // RESPOSTA
                if (strcmp(p.r.str, "TOPICS") == 0) {
                    pthread_mutex_lock(d->ptrinco);
                    int size = MAX_TOPICS * TAM_TOPICO;
                    char topicos[size];
                    bzero(topicos, size);
                    for (int i = 0; i < d->nTopicos; i++) {
                        strcat(topicos, d->topicos->nome_topico);
                        strcat(topicos, " - ");
                    }
                    pthread_mutex_unlock(d->ptrinco);

                    topicos[sizeof(topicos) - 1] = '\0'; // Garante a terminação nula

                    RESPOSTA r;
                    r.pid = getpid();
                    strcpy(r.str, topicos);

                    PEDIDO pt;
                    pt.tipo = 3;
                    pt.r = r;

                    envia_info(p.r.pid, &pt);

                    break;
                }

                if (strcmp(p.r.str, "CLOSE") == 0) {
                    pthread_mutex_lock(d->ptrinco);
                    d->pedidos_on = false;
                    pthread_mutex_unlock(d->ptrinco);
                    break;
                }

                if (strcmp(p.r.str, "EXIT") == 0) {
                    PEDIDO pf = {.tipo = 3};
                    strcpy(pf.r.str, "EXIT");

                    pthread_mutex_lock(d->ptrinco);
                    for (int i = d->nUsers - 1; i >= 0; i--) {
                        if (d->users_pids[i] == p.r.pid) {
                            if (envia_info(d->users_pids[i], &pf)) {
                                for (int j = i; j < d->nTopicos - 1; j++) {
                                    for (int k = 0; k < d->topicos[j].nSubs; k++) {
                                        if (d->topicos[j].subscritos_pid[k] == pf.r.pid) {
                                            d->topicos[j].subscritos_pid[k] = d->topicos[j].subscritos_pid[k + 1];
                                        }
                                    }
                                }

                                for (int j = i; j < d->nUsers - 1; j++) {
                                    d->users_pids[j] = d->users_pids[j + 1];
                                    strncpy(d->users_names[j], d->users_names[j + 1], TAM_NOME);
                                }
                                d->nUsers--;
                            } else {
                                printf("[ERRO] Não foi possível enviar a mensagem 'EXIT' para o usuário %d\n",
                                       d->users_pids[i]);
                            }
                        }
                    }
                    pthread_mutex_unlock(d->ptrinco);
                }
                break;
            }
            case 4: {
                pthread_mutex_lock(d->ptrinco);

                // Verifica se o tópico existe
                for (int i = 0; i < d->nTopicos; i++) {
                    if (strcmp(p.t.nome_topico, d->topicos[i].nome_topico) == 0) {
                        // Verifica se o usuário já está inscrito
                        bool ja_inscrito = false;
                        for (int j = 0; j < d->topicos[i].nSubs; j++) {
                            if (d->topicos[i].subscritos_pid[j] == p.t.upid) {
                                ja_inscrito = true;
                                break;
                            }
                        }

                        // Se não estiver inscrito, adiciona à lista de inscritos
                        if (!ja_inscrito && d->topicos[i].nSubs < MAX_USERS) {
                            d->topicos[i].subscritos_pid[d->topicos[i].nSubs++] = p.t.upid;
                        }

                        break;
                    }
                }


                pthread_mutex_unlock(d->ptrinco);
                break;
            }
            case 5: {
                pthread_mutex_lock(d->ptrinco);

                for (int i = 0; i < d->nTopicos; i++) {
                    if (strcmp(p.t.nome_topico, d->topicos[i].nome_topico) == 0) {
                        bool usuario_encontrado = false;

                        // Procura o usuário na lista de inscritos e remove
                        for (int j = 0; j < d->topicos[i].nSubs; j++) {
                            if (d->topicos[i].subscritos_pid[j] == p.t.upid) {
                                usuario_encontrado = true;

                                // Remove o usuário da lista de inscritos
                                for (int k = j; k < d->topicos[i].nSubs - 1; k++) {
                                    d->topicos[i].subscritos_pid[k] = d->topicos[i].subscritos_pid[k + 1];
                                }

                                d->topicos[i].nSubs--; // Decrementa o número de inscritos
                                break;
                            }
                        }

                        if (!usuario_encontrado) {
                            printf("[AVISO] Usuário não encontrado para cancelar a inscrição no tópico %s\n",
                                   p.t.nome_topico);
                        }

                        break;
                    }
                }

                pthread_mutex_unlock(d->ptrinco);
                break;
            }

            default:
                printf("[AVISO] Tipo de pedido desconhecido: %d\n", p.tipo);
                break;
        }
    }

    // FECHAR E APAGAR
    close(man_pipe);
    unlink(FIFO_SERV);

    printf("[MANAGER] Thread Pedidos encerrada.\n");

    return NULL;
}


//==============RECEBE PEDIDOS=================//


// Função da thread para reduzir o tempo de duração das mensagens
void *reduz_duracao(void *data) {
    DATA *d = (DATA *) data;

    while (d->pedidos_on) {
        // Espera 1 segundo
        sleep(1);

        // Bloqueia o acesso aos dados compartilhados
        pthread_mutex_lock(d->ptrinco);

        // Itera pelos tópicos e mensagens
        for (int i = 0; i < d->nTopicos; i++) {
            for (int j = 0; j < d->topicos[i].nMsgs; j++) {
                if (d->topicos[i].mensagens[j].duracao > 0) {
                    // Reduz a duração
                    d->topicos[i].mensagens[j].duracao--;

                    // Remove a mensagem se a duração chegar a zero
                    if (d->topicos[i].mensagens[j].duracao == 0) {
                        printf("\n[INFO] Mensagem '%s' no tópico '%s' expirou.\n\nADMIN CMD>",
                               d->topicos[i].mensagens[j].corpo_msg,
                               d->topicos[i].nome_topico);
                        fflush(stdout);
                        // Remove a mensagem e ajusta o array
                        for (int k = j; k < d->topicos[i].nMsgs - 1; k++) {
                            d->topicos[i].mensagens[k] = d->topicos[i].mensagens[k + 1];
                        }

                        // Reduz o número de mensagens no tópico
                        d->topicos[i].nMsgs--;
                        j--; // Ajusta o índice para verificar a nova mensagem na posição
                    }
                }
            }
        }

        // Desbloqueia o mutex
        pthread_mutex_unlock(d->ptrinco);
    }


    //GUARDAR MENSAGENS
    //E DEPOIS ZERAR O TEMPO


    printf("[THREAD] Thread de redução de duração de mensagens encerrada.\n");
    return NULL;
}


//BLOQUEIA UM TOPICO
void bloqueia_topico(DATA *d, char topico[TAM]) {
    pthread_mutex_lock(d->ptrinco);

    for (int i = 0; i < d->nTopicos; i++) {
        if (strcmp(d->topicos[i].nome_topico, topico) == 0) {
            d->topicos[i].bloqueado = true;
            printf("[AVISO] O topico %s foi bloqueado. \n", topico);
        } else {
            printf("[AVISO] Nao foi possivel encontrar o topico. \n");
        }
    }
    pthread_mutex_unlock(d->ptrinco);
}

void desbloqueia_topico(DATA *d, char topico[TAM]) {
    pthread_mutex_lock(d->ptrinco);

    bool topico_encontrado = false;

    for (int i = 0; i < d->nTopicos; i++) {
        if (strcmp(d->topicos[i].nome_topico, topico) == 0) {
            d->topicos[i].bloqueado = false;
            printf("[AVISO] O tópico '%s' foi desbloqueado.\n", topico);
            topico_encontrado = true;
            break; // Sai do loop ao encontrar o tópico
        }
    }

    if (!topico_encontrado) {
        printf("[AVISO] Não foi possível encontrar o tópico '%s'.\n", topico);
    }

    pthread_mutex_unlock(d->ptrinco);
}


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
}


// MOSTRA OS USERNAMES E OS RESPETIVOS PIDS DE TODOS
void mostra_subs(DATA *d, char nomet[]) {
    pthread_mutex_lock(d->ptrinco);
    printf("\nTOPICO: %s\n", d->topicos->nome_topico);


    for (int i = 0; i < d->nTopicos; i++) {
        if (strcmp(d->topicos[i].nome_topico, nomet) == 0) {
            for (int j = 0; j < d->topicos[i].nSubs; j++) {
                char nome_sub[TAM_NOME];


                for (int k = 0; k < d->nUsers; k++) {
                    if (d->topicos[i].subscritos_pid[j] == d->users_pids[k]) {
                        strcpy(nome_sub, d->users_names[k]);

                        printf(" %d: %s \n", j + 1, nome_sub);
                    }
                }
            }
            break;
        }
    }
    pthread_mutex_unlock(d->ptrinco);
}

// MOSTRA OS USERNAMES E OS RESPETIVOS PIDS DE TODOS
void mostra_msgs(DATA *d, char nomet[]) {
    bool topico_exite = false;

    pthread_mutex_lock(d->ptrinco);
    for (int i = 0; i < d->nTopicos; i++) {
        if (strcmp(d->topicos[i].nome_topico, nomet) == 0) {
            printf("\nTOPICO: %s\n", nomet);
            topico_exite = true;
            break;
        }
    }

    if (!topico_exite) {
        printf("\n[ERRO]Topico nao encontrado.\n");
        return;
    }

    for (int i = 0; i < d->nTopicos; i++) {
        if (strcmp(d->topicos[i].nome_topico, nomet) == 0) {
            for (int j = 0; j < d->topicos[i].nMsgs; j++) {
                for (int k = 0; k < d->nUsers; k++) {
                    if (d->topicos[i].subscritos_pid[j] == d->users_pids[k]) {
                        printf(" %d. %s[%d] (%d segundos restantes): %s\n",
                               j + 1, d->users_names[k],
                               d->topicos[i].mensagens->pid,
                               d->topicos[i].mensagens[j].duracao,
                               d->topicos[i].mensagens[j].corpo_msg);
                        break;
                    }
                }
            }
            break;
        }
    }
    pthread_mutex_unlock(d->ptrinco);
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
}

void guardarMensagens(DATA *d, const char *nomeFicheiro) {
    FILE *ficheiro = fopen(nomeFicheiro, "w");
    if (ficheiro == NULL) {
        perror("Erro ao abrir o ficheiro para escrita");
        return;
    }

    for (int i = 0; i < d->nTopicos; i++) {
        for (int j = 0; j < d->topicos[i].nMsgs; j++) {
            fprintf(ficheiro, "%s %d %d %s\n", d->topicos[i].nome_topico,d->topicos[i].mensagens[j].pid,d->topicos[i].mensagens[j].duracao,d->topicos[i].mensagens[j].corpo_msg);
        }
    }

    fclose(ficheiro);
    printf("Mensagens guardadas com sucesso em %s.\n", nomeFicheiro);
}

// Função para recuperar mensagens persistentes de um ficheiro
//ESTA É TUDO CHATGPT MAS A LOGICA DEVE TAR BOA, SO DEVE SER NECESSARIO ALTERAR PARA AS NOSSAS STRUCTS
void recuperarMensagens(DATA *data, const char *nomeFicheiro) {
    FILE *ficheiro = fopen(nomeFicheiro, "r");
    if (ficheiro == NULL) {
        perror("Erro ao abrir o ficheiro para leitura");
        return;
    }

    char linha[512];
    while (fgets(linha, sizeof(linha), ficheiro) != NULL) {
        char nome_topico[TAM];
        MENSAGEM msg;
        if (sscanf(linha, "%s %d %d %s", nome_topico, &msg.pid, &msg.duracao, msg.corpo_msg) == 4) {
            // Encontrar ou criar o tópico correspondente
            TOPICO *topico = NULL;
            for (int i = 0; i < data->nTopicos; i++) {
                if (strcmp(data->topicos[i].nome_topico, nome_topico) == 0) {
                    topico = &data->topicos[i];
                    break;
                }
            }

            if (!topico && data->nTopicos < MAX_TOPICS) {
                topico = &data->topicos[data->nTopicos++];
                strcpy(topico->nome_topico, nome_topico);
                topico->nMsgs = 0;
            }

            if (topico && topico->nMsgs < MAX_MSG_PER) {
                topico->mensagens[topico->nMsgs++] = msg;
            }
        }
    }

    fclose(ficheiro);
    printf("Mensagens recuperadas de %s.\n", nomeFicheiro);
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
    data.nTopicos = 0;
    data.pedidos_on = true;
    pthread_mutex_t trinco;
    data.ptrinco = &trinco;
    for (int i = 0; i < MAX_TOPICS; i++) {
        data.topicos[i].nMsgs = 0;
        data.topicos[i].nSubs = 0;
        data.topicos[i].bloqueado = 0;
        for (int j = 0; j < MAX_USERS; j++) {
            data.topicos[i].subscritos_pid[j] = 0;
        }
    }
    // FIM DATA


    // MENSAGENS INICIAIS
    printf("INICIO MANAGER...\n");
    printf("ESPERANDO USERS...\n");

    // THREAD PEDIDOS
    pthread_t thread_pedidos;
    pthread_create(&thread_pedidos, NULL, recebe_pedidos, &data);

    // THREAD REDUSZ DURACAO
    pthread_t thread_reduz_duracao;
    pthread_create(&thread_reduz_duracao, NULL, reduz_duracao, &data);
    //...

    recuperarMensagens(&data,FICHEIRO);

    //...

    // CICLO PRINCIPAL
    char cmd[TAM];
    do {
        printf("\nADMIN CMD> ");
        fflush(stdout);
        fgets(cmd, TAM, stdin);
        cmd[strcspn(cmd, "\n")] = '\0';
        //printf("A executar '%s' ... \n", cmd);

        if (strcmp(cmd, "users") == 0) {
            mostra_users(&data);
            continue;
        }

        if (strcmp(cmd, "block") == 0) {
            printf("Nome Topico: ");
            fflush(stdout);
            char topico[TAM];
            scanf("%s", topico);
            bloqueia_topico(&data, topico);

            // Limpa buffers de entrada para evitar resíduos
            int c;
            while ((c = getchar()) != '\n' && c != EOF);

            continue;
        }

        if (strcmp(cmd, "unlock") == 0) {
            printf("Nome Topico: ");
            fflush(stdout);
            char topico[TAM];
            scanf("%s", topico);
            desbloqueia_topico(&data, topico);

            // Limpa buffers de entrada para evitar resíduos
            int c;
            while ((c = getchar()) != '\n' && c != EOF);

            continue;
        }

        if (strcmp(cmd, "topics") == 0) {
            mostra_topicos(&data);
            continue;
        }

        if (strcmp(cmd, "subs") == 0) {
            printf("Nome Topico: ");
            fflush(stdout);
            char topico[TAM];
            scanf("%s", topico);
            mostra_subs(&data, topico);

            // Limpa buffers de entrada para evitar resíduos
            int c;
            while ((c = getchar()) != '\n' && c != EOF);

            continue;
        }

        if (strcmp(cmd, "show") == 0) {
            printf("Nome Topico: ");
            fflush(stdout);
            char topico[TAM];

            scanf("%s", topico);
            mostra_msgs(&data, topico);

            // Limpa buffers de entrada para evitar resíduos
            int c;
            while ((c = getchar()) != '\n' && c != EOF);

            continue;
        }

        //RETIRA TODOS OS USER LIGADOS AO SERVER
        if (strcmp(cmd, "remove all") == 0) {
            pthread_mutex_lock(data.ptrinco);
            int n = data.nUsers;
            pthread_mutex_unlock(data.ptrinco);

            // Criação do pedido para remover usuário
            PEDIDO pf;
            pf.tipo = 3; // Tipo para notificar expulsão ou remoção
            strcpy(pf.r.str, "REMOVE"); // Define a mensagem de remoção

            // Laço de expulsão
            for (int i = n - 1; i >= 0; i--) {
                if (envia_info(data.users_pids[i], &pf)) {
                    // Remoção do usuário em caso de sucesso
                    for (int j = i; j < data.nUsers - 1; j++) {
                        data.users_pids[j] = data.users_pids[j + 1];
                        strncpy(data.users_names[j], data.users_names[j + 1], TAM_NOME);
                    }
                    data.nUsers--;
                } else {
                    printf("[ERRO] Não foi possível enviar a mensagem 'REMOVE' para o usuário %d\n",
                           data.users_pids[i]);
                }
            }
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

            // Criação do pedido para remover usuário
            PEDIDO pf;
            pf.tipo = 3; // Tipo para notificar expulsão ou remoção
            strcpy(pf.r.str, "REMOVE"); // Define a mensagem de remoção

            // Laço de expulsão
            pthread_mutex_lock(data.ptrinco);
            for (int i = data.nUsers - 1; i >= 0; i--) {
                if (strcmp(data.users_names[i], user_name) == 0) {
                    if (envia_info(data.users_pids[i], &pf)) {
                        // Remoção do usuário em caso de sucesso
                        for (int j = i; j < data.nUsers - 1; j++) {
                            data.users_pids[j] = data.users_pids[j + 1];
                            strncpy(data.users_names[j], data.users_names[j + 1], TAM_NOME);
                        }
                        data.nUsers--;
                    } else {
                        printf("[ERRO] Não foi possível enviar a mensagem 'REMOVE' para o usuário %d\n",
                               data.users_pids[i]);
                    }
                } else {
                    printf("[ERRO] Usuário %s inexistente.\n",
                           data.users_names[i]);
                }
            }
            pthread_mutex_unlock(data.ptrinco);

            continue;
        }

        if (strcmp(cmd, "close") == 0) {
            PEDIDO pf;
            pf.tipo = 3;
            strcpy(pf.r.str, "CLOSE");

            // AVISA OS USERS QUE VAI FECHAR
            pthread_mutex_lock(data.ptrinco);
            for (int i = data.nUsers - 1; i >= 0; i--) {
                if (envia_info(data.users_pids[i], &pf)) {
                    data.nUsers--;
                } else {
                    printf("[ERRO]!\n");
                }
            }
            pthread_mutex_unlock(data.ptrinco);

            // TERMINA THREAD
            data.pedidos_on = false;

            // ENVIA QUALQUER COISA PARA SAIR DO READ DA THREAD
            RESPOSTA res;
            strcpy(res.str, "CLOSE"); //->apenas fecha nao retira os users do data

            PEDIDO pm;
            pm.tipo = 3;
            pm.r = res;

            int man_pipe = open(FIFO_SERV, O_WRONLY);
            int tam = write(man_pipe, &pm, sizeof(PEDIDO));

            if (tam < 0) {
                printf("[MANAGER] Pedido de encerramento mal executado.\n");
            }

            guardarMensagens(&data,FICHEIRO);

            pthread_join(thread_pedidos, NULL);
            pthread_join(thread_reduz_duracao, NULL);

            close(man_pipe);
        }
    } while (strcmp(cmd, "close") != 0);


    // APAGA FIFO

    unlink(FIFO_SERV);

    printf("\nFIM MANAGER...\n");

    return 0;
}
