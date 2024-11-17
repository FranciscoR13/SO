#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#define TOPICO 50
#define MSG 300
#define FIFO_CS "M-PIPE"


#define MAX_TOPICOS 20 //definir a quantidade de topicos que podem existir


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
