/*****************************************************************************
                      Chat-fe Message Transfet Protocol
  protocollo di trasmissione utilizzato per la gestione e la preparazione dei
  messaggi che verranno scambiati tra applicativi di tipo ChatFe
******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "include/CMTP.h"

//---------definisco i tipi di messaggio che posso inviare-------------------
#define MSG_LOGIN 'L'
#define MSG_REGLOG 'R'
#define MSG_OK 'O'
#define MSG_ERROR 'E'
#define MSG_SINGLE 'S'
#define MSG_BRDCAST 'B'
#define MSG_LIST 'I'
#define MSG_LOGOUT 'X'
//----------------------------------------------------------------------------
msg_t messageCreate(int type,char * sender,char * receiver, char * msg){
  /*Passo in input i campi di msg_t e restituisco una struttura dati di tipo
    msg_t contenente tali campi passati;
     		 		type	      input
    				L       	0
				R 	    	1
				O 		2
				E 		3
	 			S  		4 //user side
				B 		5 //user side
				I 		6 //user side
				X        	7
				I	        8 //server side
	      			S 		9 //server side
				B 		10 //server side
   **************************************************************************/
   msg_t risultato;//struttura che ospiterà il risultato

   if(type == 0){//login
		risultato.type = MSG_LOGIN;
		risultato.sender = NULL;
		risultato.receiver = NULL;
		risultato.msglen = strlen(msg) + 1;// + '\0'
		risultato.msg = strdup(msg);
	}
	else if(type == 1){//register & login
		risultato.type = MSG_REGLOG;
		risultato.sender = NULL;
		risultato.receiver = NULL;
		risultato.msglen = strlen(msg) + 1;// + '\0'
		risultato.msg = strdup(msg);
	}
	else if(type == 2){//OK
		risultato.type = MSG_OK;
		risultato.sender = NULL;
		risultato.receiver = NULL;
		risultato.msglen = 0;
		risultato.msg = NULL;
	}
	else if(type == 3){//error
		risultato.type = MSG_ERROR;
		risultato.sender = NULL;
		risultato.receiver = NULL;
		risultato.msglen = strlen(msg)+ 1;// + '\0'
		risultato.msg = strdup(msg);
	}
	else if(type == 4){//single -user side
		risultato.type = MSG_SINGLE;
		risultato.sender = NULL;
		risultato.receiver = strdup(receiver);
		risultato.msglen = strlen(msg) + 1;// + '\0'
		risultato.msg = strdup(msg);
	}
	else if(type == 5){//broadcast -user side
		risultato.type = MSG_BRDCAST;
		risultato.sender = NULL;
		risultato.receiver = NULL;
		risultato.msglen = strlen(msg) + 1;// + '\0'
		risultato.msg = strdup(msg);
	}
	else if(type == 6){//list - user side
		risultato.type = MSG_LIST;
		risultato.sender = NULL;
		risultato.receiver = NULL;
		risultato.msglen = 0;
		risultato.msg = NULL;
	}
	else if(type == 7){//logout
		risultato.type = MSG_LOGOUT;
		risultato.sender = NULL;
		risultato.receiver = NULL;
		risultato.msglen = 0;
		risultato.msg = NULL;
	}
	else if(type == 8){//list - server side
		risultato.type = MSG_LIST;
		risultato.sender = NULL;
		risultato.receiver = NULL;
		risultato.msglen = strlen(msg) + 1;// + '\0'
		risultato.msg = strdup(msg);
	}
	else if(type == 9){//single - server side
		risultato.type = MSG_SINGLE;
		risultato.sender = strdup(sender);
		risultato.receiver = NULL;
		risultato.msglen = strlen(msg) + 1;// + '\0'
		risultato.msg = strdup(msg);
		}
	else if(type == 10){//broadcast - server side
		risultato.type = MSG_BRDCAST;
		risultato.sender = strdup(sender);
		risultato.receiver = NULL;
		risultato.msglen = strlen(msg) + 1;// + '\0'
		risultato.msg = strdup(msg);
	}
    return risultato;
}

char* marshalling(msg_t message) {
	int lenght = 4; //sono i #
	char * stringa;
	
	//calcolo la dimensione della stringa
	lenght += 1 + sizeof(message.sender) + sizeof(message.receiver) + sizeof(message.msg);

	stringa = (char *)malloc(sizeof(char) * lenght);//allocazione stringa

	sprintf(stringa,"%c#%s#%s#%s#",message.type,message.sender,message.receiver,message.msg); 

	return stringa;
}

msg_t unmarshalling(char * msgString) {
	//fprintf(stderr,"Unmarsh: %s\n",msgString);
	msg_t risultato;
	char * token;

	risultato.type = msgString[0];
	token =strtok(msgString,"#");

	token =strtok(NULL,"#");
	risultato.sender = strdup(token);

	token =strtok(NULL,"#");
	risultato.receiver = strdup(token);

	token =strtok(NULL,"#");
	risultato.msg = strdup(token);

	risultato.msglen = strlen(risultato.msg);

	return risultato;
}
/*
 int main(int argc, char const *argv[]) {

 	msg_t testMessage;
 	bzero(&testMessage, sizeof(msg_t));
 	testMessage.type='S';
 	testMessage.sender="Marco";
 	testMessage.receiver="Luca";
 	testMessage.msg="Ciao, Mondo!";
 	testMessage.msglen=strlen(testMessage.msg);

	//fprintf(stderr,"type: %c\nsender: %s\nreceiver: %s\nlenght: %d\nmsg: %s\n",testMessage.type,testMessage.sender,testMessage.receiver,testMessage.msglen,testMessage.msg);

 	char* msgMarsh=marshalling(testMessage);
 	fprintf(stderr, "%s\n", msgMarsh);

 	msg_t uMsg = unmarshalling(msgMarsh);
	fprintf(stderr,"type: %c\nsender: %s\nreceiver: %s\nlenght: %d\nmsg: %s\n",uMsg.type,uMsg.sender,uMsg.receiver,uMsg.msglen,uMsg.msg);
 	free(msgMarsh);

 	return 0;
}*/
