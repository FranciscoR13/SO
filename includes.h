#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <pthread.h>
#include <errno.h>

//TAM MAX DE...
#define TAM_NOME 20
#define TAM_TOPICO 20
#define TAM_MSG 300
#define TAM 20

// MAX Nº DE...
#define MAX_USERS 10
#define MAX_TOPICS 20
#define MAX_MSG_PER 5

// FIFOS
#define FIFO_SERV "M_PIPE"      //MANAGER-PIPE
#define FIFO_CLI "F_%d"             //FEED-PIPE(PID)

typedef struct {
    int pid;
    char username[TAM_NOME];
}LOGIN;

typedef struct {
    int pid;
    char str[TAM_MSG];
}RESPOSTA;

typedef struct {
    char topico[TAM_TOPICO];
    char corpo_msg[TAM_MSG];
    int duracao;
    int pid;
}MENSAGEM;

typedef struct
{
    char nome_topico[TAM];
    int bloqueado; // 0- desbloqueado !0-bloqueado

    //char msgs_persistentes[MAX_MSG_PER][TAM_MSG];// 5 mensagens com 300 de comprimento

    MENSAGEM mensagens[MAX_MSG_PER];
    int nMsgs;

    int subescritos_pid[MAX_USERS];
    int nSubs;

}TOPICO;

typedef struct {
    int tipo;   // TIPO DE ESTRUTURA
    union {
        LOGIN l;
        MENSAGEM m;
        RESPOSTA r;
    };
}PEDIDO;

// >>>>>>>>>>>>>>TENTAR DIMINUIR O TAMANHO DAS ESTRUTURAS <<<<<<<<<<<<<<<<<
typedef struct {

    // LOGIN
    int nUsers;
    int users_pids[MAX_USERS];
    char users_names[MAX_USERS][TAM_NOME];

    // TOPICOS
    TOPICO topicos[MAX_TOPICS];
    int nTopicos;

    // INTERRUPTOR
    bool pedidos_on;

    // TRINCO
    pthread_mutex_t *ptrinco;

}DATA;
