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

}Msg;


typedef struct 
{
    char nomeTopico[20];
    char msg_persistentes[5][300];// 5 mensagens com 300 de comprimento
    int bloqueado; // 0- desbloqueado !0-bloqueado 
    Msg msgs[5];
}Topico;
