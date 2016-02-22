/**
 * client.c - The client program used in Operating Systems final project for OS class
 *
 * 2016 Marco Tieghi & Luca Pajola
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <pthread.h>
#include <time.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/uio.h>
#include <string.h>

#include "include/CMTP.h"	//Protocollo di gestione dei messaggi

void *Writer();
void *Listener();
void guidaChatFe(); //Stampa la guida di ChatFe
char* colonTokenizer(char *, char*); //Concatena username, name e email nel formato "user:name:email"
int tipoMessaggioUtente(char *);

int go;	//Variabile globale che regola il funzionamento dei cicli di ascolto
int clientSocket;
char *userName;

int main(int argc,char * argv[]){
	int r = 0; // Variabile per indicare se si e' eseguita un'operazione di register and login o no
    	char *userRegData;  //memorizzo i parametri passati dall'opzione -r
    	struct sockaddr_in serverAddr;	// Struttura per memorizzare i valori di connessione al server
    	int cntConn; //Contatore connessioni
	char* regInfo, *msgServer; //Risultato della concatenazione della chiamata a colonTokenizer()
    	msg_t messaggio; //Messaggio scambiato con il server
	pthread_t tListener;
	pthread_t tWriter;

	//------------- Parsing parametri d'ingresso ---------------
	if (argc < 2) {
		fprintf(stderr,"Numero di parametri errato!\n");
		exit(-1);
	}
	if ((strcmp(argv[1], "-r") == 0) && argc == 4) {
		userRegData=argv[2];  //memorizzo i parametri passati dall'opzione -r
		userName = argv[3];
		r=1;
	} else if (argc == 2) {
		userName=argv[1];
	} else if ((strcmp(argv[1], "-h") == 0) && (strcmp(argv[2], "-r") == 0) && argc == 5) {
		guidaChatFe();
		userRegData=argv[3];  //memorizzo i parametri passati dall'opzione -r
		userName = argv[4];
		r=1;
	} else if (strcmp(argv[1], "-h") == 0 && argc == 2) {
		guidaChatFe();
		exit(0);
	} else if ((strcmp(argv[1], "-h") == 0) && argc == 3) {
		guidaChatFe();
		userName = argv[2]; //memorizzo i parametri di login
	}
    	//----------------------------------------------------------

	//--------------connessione al server-----------------------
    	//Stabilisco la connessione al server
    	if ((clientSocket = socket(AF_INET, SOCK_STREAM, 0))<0) {	//dominio internet, connection oriented, protocollo di default
		fprintf(stderr, "Error opening the socket.\n");
		exit(-2);
	}

	// Inizializzazione dell'indirizzo del server
	bzero((char *) &serverAddr, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr=INADDR_ANY;
	serverAddr.sin_port = 5000;

    	cntConn = 10;
	while(cntConn > 0){
		if(connect(clientSocket,(struct sockaddr *)&serverAddr,sizeof(serverAddr))<0){
            		cntConn--;
            		sleep(1);	//Sospendo il processo per un secondo
		} else
            		cntConn = -1;	//Connessione avenuta con successo
	}

	if(cntConn ==0) {
	        fprintf(stderr, "Ops... Impossibile connettersi al server... riprovare più tardi!\n");
	        exit(0);
    	} else
        	fprintf(stderr,"Connessione al server riuscita!\n");
    	//----------------------------------------------------------

	//-------------------Autenticazione-------------------------
    	if (r) {    //Registrazione
        	fprintf(stderr, "Tentativo di registrazione e login\n");
        	regInfo=colonTokenizer(userRegData, userName);
        	messaggio=messageCreate(1,NULL,NULL,regInfo); //creo messaggio di registrazione
    	} else {    //Login
        	fprintf(stderr, "Tentativo di login\n");
        	messaggio=messageCreate(0,NULL,NULL,userName); //creo messaggio di login
    	}

	msgServer=marshalling(messaggio);

	if(write(clientSocket,msgServer,strlen(msgServer))<0) {	//Invio del messaggio di login al server
        	fprintf(stderr, "Impossibile contattare il server. Disconnessione in corso\n");
        	exit(-3);
    	}

	//Lettura della risposta del server: connessione accettata o no?
	if(read(clientSocket,msgServer, sizeof(char)*8192)<0) {	//Invio del messaggio di login al server
        	fprintf(stderr, "Impossibile contattare il server. Disconnessione in corso\n");
        	exit(-3);
    }

	bzero(&messaggio,sizeof(msg_t));
	messaggio=unmarshalling(msgServer);
	switch (messaggio.type) {
		case 'O':
			fprintf(stderr, "Accesso effettuato %s!\n",userName);
		break;
		case 'E':
			fprintf(stderr, "Errore: %s\n Disconessione in corso. Ritenta piu' tardi\n", messaggio.msg);
			exit(0);
		break;
	}
	//----------------------------------------------------------
	if (pthread_create(&tListener,NULL,&Listener,NULL) != 0) {
        	fprintf(stderr, "Impossible creare il thread listener\n");
        	exit(0);
    }
    if (pthread_create(&tWriter,NULL,&Writer,NULL) != 0) {
        	fprintf(stderr, "Impossible creare il thread writer\n");
        	exit(0);
    }

    	go = 1;

    	//Aspetto che muoiano i thread
    	pthread_join(tListener, NULL);
    	pthread_join(tWriter, NULL);

    	close(clientSocket);	//Chiusura sock

	return 0;
}


void * Writer(){
	int type;	//Indica il tipo di messaggio scritto dall'utente #ls #logout ..
	msg_t messaggio;
	char *linea;	//Linea di input letta
	char * token;
	char *username;
	size_t lineaLenght;	//Lunghezza della linea

	linea = NULL;
    	lineaLenght = 0;

	//Inizio a leggere le linee
	while (getline(&linea, &lineaLenght, stdin) != -1 && go){
		type = tipoMessaggioUtente(linea);

		if(type >= 0){	//Formato corretto
            		if(type == 1){//#ls
    				messaggio = messageCreate(6,NULL,NULL,NULL);
    			}
    			else if(type == 2){//#logout
                		messaggio = messageCreate(7,NULL,NULL,NULL);
				go = 0;
    			}
    			else if(type == 3){//#broadcast
    				token = strdup(strtok(linea,":"));
    				token = strdup(strtok(NULL,"\n"));
    				messaggio = messageCreate(5,NULL,NULL,token);
    			}
    			else{//#single
    				token = strdup(strtok(linea," "));
    				token = strdup(strtok(NULL,":"));
    				username = strdup(token);
    				token = strdup(strtok(NULL,"\n"));
    				messaggio = messageCreate(4,NULL,username,token);
    			}
    			//il messaggio è stato creato ->lo invio
    			if (write(clientSocket, marshalling(messaggio), strlen(marshalling(messaggio))) < 0) {//errore nella comunicazione del server
            			fprintf(stderr,"ATTENZIONE:Errore nell'invio del messaggio - Impossibile contattare il server\n");
            			go = 0;	//Chiudo tutto
				pthread_exit(NULL);	//Chiusura thread
            		}

            		if(messaggio.type == 'X') {
				go = 0;	//logout -> chiudiamo tutto
				pthread_exit(NULL);	//Chiusura thread
			}
    		}
    		else //Formato errato
    			fprintf(stderr,"ATTENZIONE:Formato del messaggio errato (code %d)\n",type);
    	}

	pthread_exit(NULL);
}

void * Listener(){
	char *stringa;	//Messaggio ricevuto dal server
	msg_t messaggio,err;	//Messaggio ricevuto dal server scomposto

	stringa = (char *)malloc(sizeof(char)*8192);	//Preparo la stringa ad essere ricevuta

	while (read(clientSocket, stringa, sizeof(char)*8192) > 0 && go){
		messaggio=unmarshalling(stringa);	//Eseguo l'unmarshalling del messaggio inserendo nei campi della struttura i token dalla stringa ricevuta
		 switch (messaggio.type) {
		 	case 'I':
		 	fprintf(stderr, "Attualmente sono collegati i seguenti utenti: %s\n", messaggio.msg);
		 	break;
		 	case 'S':
		 	fprintf(stderr, "%s:%s:%s\n", messaggio.sender ,userName, messaggio.msg);
		 	break;
		 	case 'B':
		 	fprintf(stderr, "%s:*:%s\n", messaggio.sender, messaggio.msg);
		 	break;
		 	case 'E':{
		 		fprintf(stderr, "Errore: %s\n", messaggio.msg);
				err = messageCreate(2,NULL,NULL,NULL);	//msg ok
				if (write(clientSocket, marshalling(err), strlen(marshalling(err))) < 0) {	//server
            				fprintf(stderr,"ATTENZIONE:Errore nell'invio del messaggio - Impossibile contattare il server\n");
            				go = 0;	//Chiudo tutto
					pthread_exit(NULL);	//Chiusura thread
            			}
			}
		 }

        	// Resettiamo il buffer per il prossimo giro
        	stringa = realloc(stringa, sizeof(char)*8192);
			//bzero(&messaggio, sizeof(msg_t));
	}
	go =0;
	pthread_exit(NULL);
}

void guidaChatFe(){
    	printf("*********************************************************************\n");
    	printf("*                            ChatFe                                 *\n");
    	printf("*             versione 1.2 ultimo aggiornamento 28/12/2015          *\n");
    	printf("*********************************************************************\n");
    	printf("*                            GUIDA                                  *\n");
    	printf("*********************************************************************\n");
    	printf("*   '#' per eseguire uno dei seguenti comandi (esso non può esser   *\n");
    	printf("*                contenuto all'interno dei messaggi):               *\n");
    	printf("*    #logout : per sconnettersi dal server e terminare il client    *\n");
    	printf("*    #ls : serve per conoscere la lista di utenti connessi          *\n");
    	printf("*    #dest : per specificare un destinatario. Se il destinatario    *\n");
    	printf("*            è vuoto il messaggio è inviato in broadcast            *\n");
    	printf("*    #dest destinatario:testo : invio messaggio a destinatario      *\n");
    	printf("*********************************************************************\n");
}

//Riceve in ingresso due stringhe: la prima e' la stringa passata con l'opzione -r "Nome Cognome email",
//la seconda l'username di registrazione. Resituisce il puntatore alla stringa ottenuta
char* colonTokenizer(char *userInfo, char* username) {
	int stringSize=(int) (strlen(userInfo)+strlen(username)) +2;
	char *colonString=(char *) malloc(stringSize*sizeof(char));
	strcat(colonString, username);
	strcat(colonString, ":");
	strcat(colonString, strtok(userInfo, " "));
	strcat(colonString, " ");
	strcat(colonString, strtok(NULL, " "));
	strcat(colonString, ":");
	strcat(colonString, strtok(NULL, " "));
	strcat(colonString, "\0");
	return colonString;
}

int tipoMessaggioUtente(char * msg){
    	//-------------------------LEGENDA------------------------------
    	//              Valore          Significato
    	//                1                #ls:
    	//                2                #logout:
    	//                3                #dest:       (broadcast)
    	//                4                #dest .. :    (utente)
    	//--------------------------------------------------------------
    int length = strlen(msg) -1; //lunghezza di msg
    //---controllo numero '#' ->deve essercene uno ed uno solo e in posizione 0
    int pos=-1,i=0;
    while(i<length){
        if(msg[i] == '#') pos=i;//così facendo ottengo la posizione dell'ultimo '#' che scorro
        i++;
    }
    if(pos!=0)return -1;
    //---------------------------------------------------------------

    //-----controllo comando '#ls:'----------------------------------
    if(length == 4){//deve essere #ls: altrimenti è un comando errato
        //non controllo la prima posizione della stringa perchè so che è #
        if(msg[1]=='l' && msg[2] == 's' && msg[3] == ':') {
        //fprintf(stderr, "Are you fucking kidding me?\n");
        return 1;}//comando ls corretto
        else    return -2;//comando errato
    }
    //---------------------------------------------------------------

    //-----controllo comando '#logout:'----------------------------------
    if(length == 8){//deve essere #logout: altrimenti è un comando errato
        //non controllo la prima posizione della stringa perchè so che è #
        if(msg[1]=='l' && msg[2] == 'o' && msg[3] == 'g' && msg[4] == 'o' && msg[5] == 'u' && msg[6] == 't' && msg[7] == ':') return 2;//comando ls corretto
        else    return -3;//comando errato
    }
    //---------------------------------------------------------------

    //-------controllo comando '#dest:'-------------------------------
    if(msg[1]=='d' && msg[2] == 'e' && msg[3] == 's' && msg[4] == 't')//inizio di sequenza corretta
        //cerco la posizione di ':'
        if(msg[5] == ':' || (msg[5]==' ' && msg[6] == ':')) return 3; //messaggio di tipo broadcast #dest: o #dest :
        pos =0;i=0;
        if(msg[5] != ' ') return-4;//se il messaggio non è broadcast il comando #dest deve essere seguito da uno spazio
        while(i<length && !pos){
            if(msg[i]==':') pos =1;
            i++;
        }
        if(pos) return 4; //messaggio a specifici utenti
    //----------------------------------------------------------------

    return -4;//se arrivo a questo punto significa che non ho riconosciuto alcun comando
}
