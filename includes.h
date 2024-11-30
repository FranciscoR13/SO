#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

//TAM MAX DE...
#define NOME 20
#define TOPIC 20
#define MSG 300
#define TAM 20

// MAX Nº DE...
#define MAX_USERS 10
#define MAX_TOPICS 20
#define MAX_MSG_PER 5

// FIFOS
#define FIFO_SERV "M_PIPE"  //MANAGER-PIPE
#define FIFO_CLI "F_%d"     //FEED-PIPE(PID)

// Á PARTIDA APENAS USADA NO MANAGER
typedef struct {
    int man_pipe;                          // PIPE QUE RECEBE OS LOGINS
    int nUsers;                            // Nº USERS
    int users_pids[MAX_USERS];             // TABELA COM OS PIDS
    char users_names[MAX_USERS][NOME];     // TABELA COM OS USERNAMES
}THREAD_LOGIN;

typedef struct {
    int *man_pipe;                 // PIPE QUE RECEBE AS MSG (ESTE É * PQ ESTAVA A TESTAR E NAO MUDEI O OUTRO)
    THREAD_LOGIN *thread_login;    // SERVE PARA TER ACESSO A TABELA DOS USERS (QUERO MUDAR PARA UM PONTEIRO PARA A TABELA))
    bool ligado;                   //AINDA EM TESTE MAS APENAS PARA FECHAR A THREAD
}THREAD_MSG;

typedef struct {
    int pid;
    char username[NOME];
}LOGIN;

typedef struct {
    char str[MSG];
    //...
}RESPOSTA;

typedef struct 
{
    char topicO[TOPICO];//para sabermos a que topico pertence
    char corpo_msg[MSG];
    int duracao; //a msg tem uma duração por isso pus isto aqui pode ser sujeito a alterações

}Msg; // NO MEU CODIGO TEM O NOME DE MENSAGEM


typedef struct 
{
    char nomeTopico[20];
    char msg_persistentes[5][300];// 5 mensagens com 300 de comprimento
    int bloqueado; // 0- desbloqueado !0-bloqueado 
    Msg msgs[5];
}Topico;
