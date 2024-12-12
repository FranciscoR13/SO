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
