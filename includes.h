#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pthread.h>

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
#define FIFO_SERV "M_PIPE"  //MANAGER-PIPE
#define FIFO_CLI "F_%d"     //FEED-PIPE(PID)

// A PARTIDA APENAS USADA NO MANAGER
typedef struct {
    int man_pipe;
    int nUsers;
    int users_pids[MAX_USERS];
    char users_names[MAX_USERS][TAM_NOME];
}THREAD_LOGIN;

typedef struct {
    int *man_pipe;
    THREAD_LOGIN *thread_login;
    bool ligado;
    //...
}THREAD_MSG;

typedef struct {
    int pid;
    char username[TAM_NOME];
}LOGIN;

typedef struct {
    char str[TAM_MSG];
    //...
}RESPOSTA;

typedef struct {
    char topico[TAM_TOPICO];
    char corpo_msg[TAM_MSG];
    int duracao;      //duracao da msg
    int pid;
}MENSAGEM;

typedef struct
{
    char nome_topico[TAM];
    int bloqueado; // 0- desbloqueado !0-bloqueado

    char msgs_persistentes[MAX_MSG_PER][TAM_MSG];// 5 mensagens com 300 de comprimento
    MENSAGEM mensagens[MAX_MSG_PER];
}TOPICO;
