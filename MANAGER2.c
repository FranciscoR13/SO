#include "includes.h"

// ENVIA UMA "RESPOSTA" A UM USER
bool envia_info(const int pid, PEDIDO* p) {
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


// FUNCAO DA THREAD_MSG QUE FICA A RECEBER AS MENSAGENS
void *recebe_pedidos(void *data) {
    // VARIAVEIS
    DATA *d = (DATA *) data; // CONVERTE * PARA ACEDER AOS DADOS
    PEDIDO p;
    int feed_pipe, man_pipe;
    char feed_fifo[TAM];
    // FIM VARIAVEIS

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
        // VERIFICA O TIPO DE PEDIDO E PROCESSA DE ACORDO
        // ( 1->LOGIN | 2->MENSAGEM | 3->STRING)
        int tam = read(man_pipe, &p, sizeof(PEDIDO));
        if (tam <= 0) {
            perror("Erro ao ler do FIFO_SERV_MSG");
            break;
        }

        // LOGIN
        if (p.tipo == 1) {
            // CONSTRÓI O NOME DO FIFO_CLI
            snprintf(feed_fifo, TAM, FIFO_CLI, p.l.pid);
            feed_pipe = open(feed_fifo, O_WRONLY);
            if (feed_pipe < 0) {
                perror("\nErro ao abrir FIFO do cliente\n");
                continue;
            }

            // CONTINUA SE CORREU BEM

            // VERIFICA SE O USERNAME JÁ EXISTE
            bool valido = true;

            pthread_mutex_lock(d->ptrinco);
            int n = d->nUsers;

            for (int i = 0; i < n; i++) {
                if (strcmp(d->users_names[i], p.l.username) == 0 || d->users_pids[i] == p.l.pid) {
                    valido = false;
                    break;
                }
            }
            pthread_mutex_unlock(d->ptrinco);

            // ENVIA A RESPOSTA AO USER
            if (valido) {
                //========> ENVIA RESPOSTA DE SUCESSO <========//
                RESPOSTA res;
                strcpy(res.str, "SUCCESS");
                write(feed_pipe, &res, sizeof(RESPOSTA));
                //============================================//

                // ADICIONA O USERNAME E O PID AOS DADOS
                pthread_mutex_lock(d->ptrinco);
                snprintf(d->users_names[d->nUsers], TAM_NOME, "%s", p.l.username);
                d->users_pids[d->nUsers++] = p.l.pid;
                pthread_mutex_unlock(d->ptrinco);
            } else {
                //=========> ENVIA RESPOSTA DE FALHA <=========//
                RESPOSTA r;
                strcpy(r.str, "FAIL");
                write(feed_pipe, &r, sizeof(RESPOSTA));
                //============================================//
            }

            // INDEPENDENTE DA RESPOSTA, O PIPE É FECHADO
            close(feed_pipe);
        }
        // FIM LOGIN

        // MENSAGEM
        else if (p.tipo == 2) {
            // MENSAGEM RECEBIDA
            /*
            printf("\nTópico recebido: %s\nMensagem recebida: %s\nDuração: %d\n",
                   p.m.topico, p.m.corpo_msg, p.m.duracao);
            */

            // Verifica se a mensagem deve ser persistente (ainda não implementado)
            // >>>>> Implementar lógica de persistência conforme necessário <<<<<

            // Verifica se o tópico já existe
            int topico_encontrado = 0;

            pthread_mutex_lock(d->ptrinco);

            for (int i = 0; i < d->nTopicos; i++) {
                if (strcmp(d->topicos[i].nome_topico, p.m.topico) == 0) {
                    topico_encontrado = 1;

                    // Armazena a mensagem no tópico encontrado
                    if (d->topicos[i].nMsgs < MAX_MSG_PER) {
                        strncpy(d->topicos[i].mensagens[d->topicos[i].nMsgs].corpo_msg,
                                p.m.corpo_msg, TAM_MSG);
                        d->topicos[i].mensagens[d->topicos[i].nMsgs].corpo_msg[TAM_MSG - 1] = '\0';
                        d->topicos[i].mensagens[d->topicos[i].nMsgs].duracao = p.m.duracao;
                        d->topicos[i].nMsgs++;
                    } else {
                        printf("[AVISO] Limite de mensagens por tópico atingido. Mensagem descartada.\n");
                    }
                    break;
                }
            }

            // Se o tópico não foi encontrado, cria um novo
            if (!topico_encontrado && d->nTopicos < MAX_TOPICS) {
                strncpy(d->topicos[d->nTopicos].nome_topico, p.m.topico, TAM_TOPICO);
                d->topicos[d->nTopicos].nome_topico[TAM_TOPICO - 1] = '\0';
                strncpy(d->topicos[d->nTopicos].mensagens[0].corpo_msg, p.m.corpo_msg, TAM_MSG);
                d->topicos[d->nTopicos].mensagens[0].corpo_msg[TAM_MSG - 1] = '\0';
                d->topicos[d->nTopicos].mensagens[0].duracao = p.m.duracao;
                d->topicos[d->nTopicos].nMsgs = 1;
                d->nTopicos++;
            } else {
                printf("[AVISO] Número máximo de tópicos atingido. Mensagem descartada.\n");
            }


            pthread_mutex_unlock(d->ptrinco);

            // Exibe os tópicos e mensagens armazenados (temporário para debug)
            printf("\n[Estado Atual]\n");
            pthread_mutex_lock(d->ptrinco);
            for (int i = 0; i < d->nTopicos; i++) {
                printf("Tópico: %s\n", d->topicos[i].nome_topico);
                for (int j = 0; j < d->topicos[i].nMsgs; j++) {
                    printf("  Mensagem: %s (Duração: %d)\n",
                           d->topicos[i].mensagens[j].corpo_msg,
                           d->topicos[i].mensagens[j].duracao);
                }
            }
            pthread_mutex_unlock(d->ptrinco);
        }


        // RESPOSTA
        else if (p.tipo == 3) {

            // MANAGER
            if (strcmp(p.r.str, "CLOSE") == 0) {
                pthread_mutex_lock(d->ptrinco);
                d->pedidos_on = false;
                pthread_mutex_unlock(d->ptrinco);
                break;
            }

            // FEED
            if (strcmp(p.r.str, "EXIT") == 0) {

                PEDIDO pf;
                pf.tipo = 3;
                strcpy(pf.r.str, "EXIT");

                pthread_mutex_lock(d->ptrinco);
                for (int i = d->nUsers - 1; i >= 0; i--) {
                    if (envia_info(d->users_pids[i], &pf)) {
                        // Remoção do usuário em caso de sucesso
                        for (int j = i; j < d->nUsers - 1; j++) {
                            d->users_pids[j] = d->users_pids[j + 1];
                            strncpy(d->users_names[j], d->users_names[j + 1], TAM_NOME);
                        }
                        d->nUsers--;
                    } else {
                        printf("[ERRO] Não foi possível enviar a mensagem 'REMOVE' para o usuário %d\n", d->users_pids[i]);
                    }
                    pthread_mutex_unlock(d->ptrinco);
                    break;
                }
            }
        }
    }

    // FECHAR E APAGAR
    close(man_pipe);
    unlink(FIFO_SERV);

    printf("[MANAGER] Thread Pedidos encerrada.\n");

    return NULL;
}

//==============RECEBE PEDIDOS=================//


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
    }
    // FIM DATA

    // MENSAGENS INICIAIS
    printf("INICIO MANAGER...\n");
    printf("ESPERANDO USERS...\n");

    // THREAD PEDIDOS
    pthread_t thread_pedidos;
    pthread_create(&thread_pedidos, NULL, recebe_pedidos, &data);
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

        if (strcmp(cmd, "topics") == 0) {
            mostra_topicos(&data);
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
                    printf("[ERRO] Não foi possível enviar a mensagem 'REMOVE' para o usuário %d\n", data.users_pids[i]);
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

            pthread_mutex_lock(data.ptrinco);
            int n = data.nUsers;
            //copiar tabela user_names???
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
                    printf("[ERRO] Não foi possível enviar a mensagem 'REMOVE' para o usuário %d\n", data.users_pids[i]);
                }
            }

            continue;
        }

        if (strcmp(cmd, "desliga") == 0) {
            // Sinaliza que o login deve parar
            data.pedidos_on = false;

            RESPOSTA res;
            strcpy(res.str, "CLOSE");

            PEDIDO p;
            p.tipo = 3;
            p.r = res;

            int man_pipe = open(FIFO_SERV, O_WRONLY);
            int tam = write(man_pipe, &p, sizeof(PEDIDO));

            if (tam < 0) {
                printf("[MANAGER] Pedido de encerramento mal executado.\n");
                continue;
            }

            continue;
        }

        if (strcmp(cmd, "liga") == 0) {
            if (data.pedidos_on == false) {
                pthread_create(&thread_pedidos, NULL, recebe_pedidos, &data);
                data.pedidos_on = true;
            } else {
                printf("Os Pedidos já estão a funcionar\n");
            }
            continue;
        }

        if (strcmp(cmd, "close") == 0) {

            // EXPULSA O USER
            pthread_mutex_lock(data.ptrinco);
            int n = data.nUsers;
            pthread_mutex_unlock(data.ptrinco);

            PEDIDO pf;
            pf.tipo = 3;
            strcpy(pf.r.str, "CLOSE");

            for (int i = n - 1; i >= 0; i--) {
                pthread_mutex_lock(data.ptrinco);
                // >>>>>>>>>>>>>ALTERAR "RESPOSTA"<<<<<<<<<<<<<
                if (envia_info(data.users_pids[i], &pf)) {
                    data.nUsers--;
                } else {
                    printf(" NAO DEU!\n");
                }
                pthread_mutex_unlock(data.ptrinco);
            }
            // EXPULSA O USER

            // TERMINA A THREAD

            // Sinaliza que o login deve parar
            data.pedidos_on = false;

            RESPOSTA res;
            strcpy(res.str, "CLOSE");

            PEDIDO pm;
            pm.tipo = 3;
            pm.r = res;

            int man_pipe = open(FIFO_SERV, O_WRONLY);
            int tam = write(man_pipe, &pm, sizeof(PEDIDO));

            if (tam < 0) {
                printf("[MANAGER] Pedido de encerramento mal executado.\n");
            }

            pthread_join(thread_pedidos, NULL);
        }
    } while (strcmp(cmd, "close") != 0);


    // APAGA FIFO

    unlink(FIFO_SERV);

    printf("\nFIM MANAGER...\n");

    return 0;
}
