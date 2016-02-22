/**
 * server.c - The server program used in Operating Systems final project for OS class
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
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>

#include "include/CMTP.h"	//Protocollo di gestione dei messaggi
#include "include/gestoreUtenti.h" //Gestisce il file degli utenti (registrati-connessi)
#include "include/gestoreLogFile.h"	//Gestisce il log file
#include "include/circBufferUtil.h"	//Gestore buffer circolare

#define PORTNO 5000 //Impostiamo il numero di porta
#define K 100

void signal_handler(int);	//Gestione del signal
void mainServer();	//Funzione richiamata alla creazione del processo deamon
void *workerFunc(void *);	//Funzione associata ai thread worker
void *dispatcherFunc(void *);	//Funzione eseguita dal thread dispatcher

int go;	//Variabile che controlla i cicli dei thread
char * fileLog;	//Buffer associato al file di log
char *fileUtenti;	//Buffer associato al file contenente gli utenti registrati

int numThreadAttivi;	//Indica il numero di utenti connessi
pthread_mutex_t muxConnessi;	//Mutex per l'accesso in mutua esclusione alla variabile di utenti connessi

mesBufStr msgBuff;  // Variabile usata per gestire il buffer circolare

int main(int argc,char * argv[]){
	int pid;	//Valore di ritorno della fork

	//Copio il valore dei parametri passati in input
    fileUtenti = argv[1];
    fileLog = argv[2];

	//Preparo il processo ad intercettare i segnali
	if(signal(SIGINT,   signal_handler)== SIG_ERR)
        perror("can't catch SIGINT\n");
	if(signal(SIGTERM,   signal_handler)== SIG_ERR)
 		perror("can't catch SIGERR\n");

	//Rendo il processo demone
	pid = fork();
    if (pid == 0)
        mainServer(argc, argv);
    else if (pid < 0) {
        	fprintf(stderr, "Fork failed: chat server process not launched\n");
        	exit(-1);
    }

	return 0;
}

void signal_handler(int sig) {
	//Termino tutti i thread aperti
    go=0;

	logClose();

	updateUserFile(fileUtenti);	//Aggiorno userfile con i nuovi utenti registrati

	//Sblocco il dispatcher che attende messaggi
	insert(&msgBuff,messageCreate(2,NULL,NULL,NULL));

	//Creao una falsa connessione per uscire
	int sock_id;
	struct sockaddr_in sock_uscita;

	sock_id = socket(AF_INET,SOCK_STREAM,0);//Dominio internet, connection oriented, protocollo di default

	bzero(&sock_uscita,sizeof(struct sockaddr_in));//Pulisco la struttura
	sock_uscita.sin_family = AF_INET; //Associato al dominio
	sock_uscita.sin_port = PORTNO; //Associato alla porta
	sock_uscita.sin_addr.s_addr = INADDR_ANY; //Indirizzo associato al server

	//Chiudo tutti i worker
	char * nextUser = strtok(listaUtentiConnessi(), ":");
	int dest_socket;
	msg_t sentMessage;
	for (int i = numThreadAttivi; i > 0; i--) {
		dest_socket = getSock(nextUser);
		sentMessage = messageCreate(3,NULL,NULL,"Terminazione Server");
		write(dest_socket, marshalling(sentMessage),strlen(marshalling(sentMessage)));//invio messaggio al destinatario
		nextUser = strtok(NULL, ":");
	}

    if(connect(sock_id,(struct sockaddr *)&sock_uscita,sizeof(sock_uscita))<0) fprintf(stderr,"errore nella connessione..\n");
    	close(sock_id);
}

void mainServer(){
	int newSock; //rappresenta il socket che verrà generato
	int masterSocket;//socket del server
	struct sockaddr_in serverAddr;
	pthread_attr_t detached; //definizione attributi del worker
    	pthread_t dispatcherThread; //rappresenta il thread dispatcher
    	pthread_t workerThread; //rappresenta il thread worker

	//carico lo userfile
	readUserFile(fileUtenti);

	//inizializzo il logfile
	inizializzazioneLogFile(fileLog);

	//Creazione socket per l'ascolto
    	if ((masterSocket = socket(AF_INET, SOCK_STREAM, 0))<0)	//Creo il socket master
		exit(-2);
	bzero(&serverAddr, sizeof(struct sockaddr_in));	//Inizializzo la struttura del Server Internet address
	serverAddr.sin_family=AF_INET;	//Definisco i campi della struttura
	serverAddr.sin_port=PORTNO;
	serverAddr.sin_addr.s_addr=INADDR_ANY;

    	// Binding
    	if (bind(masterSocket, (struct sockaddr *) &serverAddr, sizeof(serverAddr))<0)//Binding del socket sull'indirizzo della macchina server
		exit(-2);

    	listen(masterSocket,5);	//Creazione della coda di richieste: ascolto di connesioni in entrata

	readUserFile(fileUtenti);//carico gli utenti registrati nella hash table

	initStruct(&msgBuff, K);	/* Inizializzo la struttura per gestire il buffer circolare */

	go = 1;//faccio partire i cicli

	//Definizione attributi dei thread worker
    	pthread_attr_init(&detached);
    	pthread_attr_setdetachstate(&detached, PTHREAD_CREATE_DETACHED);//modalità deteached

	// Creo il thread dispatcher
	if (pthread_create(&dispatcherThread, NULL, dispatcherFunc, NULL)<0) {
		exit(-3);
	}

	while(go && (newSock=accept(masterSocket, NULL, 0))>0){
		if (pthread_create(&workerThread, &detached, workerFunc, (void*)&newSock))
           		fputs("Errore nella generazione del worker\n",stderr);
	}


	//esco dal thread
	pthread_exit(0);
}

void *dispatcherFunc(void *arg) {	/* Funzione eseguita dal thread dispatcher */
    	msg_t message;  //messaggio prelevato dal buffer
    	msg_t sentMessage; //messaggio da inviare al client
	int dest_socket, send_socket;   //socket del destinatario/mittente
	char *error_message;	//Eventuale messaggio di errore
	char *connectedUSers;	//Tutti gli utenti connessi
	char *nextUser; //Prossimo utente connesso cui inviare il emssaggio in broadcast

	//message = messageCreate(2,NULL,NULL,NULL);

	while(go){
		bzero(&message, sizeof(msg_t));
		bzero(&sentMessage, sizeof(msg_t));
		message=extract(&msgBuff);    //estraggo messaggio dal buffer circolare

		//Una volta estratto il messaggio, devo verificare se si tratta di un messaggio single user o broadcast
		//Se e' un messaggio single user, verifico se l'utente esiste, e' connesso e se sono verificate le condizioni invio il messaggio
		if(message.type=='S') {
			dest_socket = getSock(message.receiver);
			send_socket = getSock(message.sender);


			//fprintf(stderr, "sockId receiver: %d, receiver name: %s\n", dest_socket, message.receiver);
			//fprintf(stderr, "sockId sender: %d\n", send_socket);

			if(dest_socket == -1 ) {
                		//Invio messaggio di errore al client
                		error_message = "L'utente a cui stai inviando il messaggio non esiste o non e' connesso..";
				sentMessage = messageCreate(3,NULL,NULL,error_message);//creo il messaggio di errore che invierò all'utente
				write(send_socket, marshalling(sentMessage),strlen(marshalling(sentMessage)));//invio messaggio al destinatario
			} else {
				//Il receiver e' connesso e gli invio il messaggio
				sentMessage = messageCreate(9, message.sender, NULL, message.msg);
				write(dest_socket, marshalling(sentMessage),strlen(marshalling(sentMessage)));//invio messaggio al destinatario
				//fprintf(stderr, "dest:%d:%d:%s\n",sizeof(message.msg),strlen(message.msg) ,message.msg);
				singleLog(fileLog,strdup(sentMessage.sender),strdup(message.receiver),strdup(sentMessage.msg));//aggiorno logfile
			}
		}
		else if (message.type=='B') {
			connectedUSers = listaUtentiConnessi();
			nextUser = strtok(connectedUSers, ":");
			for (int i = numThreadAttivi; i > 0; i--) {
				dest_socket = getSock(nextUser);
				sentMessage = messageCreate(10, message.sender, NULL, message.msg);
				//fprintf(stderr,"m1: %s\n\n\n",message.msg);
				//fprintf(stderr,"m2: %s\n\n\n",sentMessage.msg);
				write(dest_socket, marshalling(sentMessage),strlen(marshalling(sentMessage)));//invio messaggio al destinatario
				nextUser = strtok(NULL, ":");

				singleLog(fileLog,strdup(sentMessage.sender),strdup(message.sender),strdup(sentMessage.msg));//aggiorno logfile

			}

		}
	}

	pthread_exit(0);
}


void *workerFunc(void *arg) {
    	msg_t receive, send;//messaggio ricevuto e inviato al mittente
    	char * bufferIn, * bufferOut;//stringa ricevuta e inviata da/a socket
    	char * username;//username dell'utente in possesso di questo worker
    	int sockId;//indica il valore del socket

	//aggiorno il numero di utenti connessi
	pthread_mutex_lock(&muxConnessi);
        numThreadAttivi++;//incremento il numero di utenti connessi
        pthread_mutex_unlock(&muxConnessi);

	bufferIn = (char *)malloc(sizeof(char)* 8192);

    	sockId = *(int *)arg ;//casting dell'socket

	//ascolto le richieste dell'utente
	while(go && read(sockId,bufferIn,sizeof(char)*8192)>0){
		//scomposizione della stringa in una struttura messaggio
		receive = unmarshalling(bufferIn);
		//fprintf(stderr,"Messaggio %s\n",receive.msg);
		//fprintf(stderr,"Messaggio ricevuto");
		switch(receive.type){
			case 'L': //richiesta di login
            			username = strdup(receive.msg);//prelevo lo username

				//effettuo un controllo dell'utente sulla tabella degli utenti registrati
            			if(userLogin(username,sockId)){//login eseguito con successo
                			loginLog(fileLog,username);//stampo nel file log il login dell'utente
                			send = messageCreate(2,NULL,NULL,NULL);//msg ok
                			if(write(sockId, marshalling(send),strlen(marshalling(send)))<0){//invio messaggio al destinatario
                    				//impossibile contattare client - client disconnesso in manira anomala
                    				pthread_mutex_lock(&muxConnessi);
                    				numThreadAttivi--;
                    				pthread_mutex_unlock(&muxConnessi);

						userLogout(username);//elimino l'utente dalla lista di quelli connessi

                    				logoutLog(fileLog,username);//aggiorno il logfile con il logout dell'utente

						pthread_exit(NULL);
                			}
            			}
            			else{//login errato
					bufferOut = "Errore nel login";
                			send = messageCreate(3,NULL,NULL,bufferOut);//creo il messaggio di errore che invierò all'utente
					write(sockId, marshalling(send),strlen(marshalling(send)));//invio messaggio al destinatario
                			pthread_mutex_lock(&muxConnessi);
                			numThreadAttivi--;
                			pthread_mutex_unlock(&muxConnessi);
                			pthread_exit(NULL);//uccido il thread in quanto il login è errato
            			}

			break;
			case 'R'://richiesta di registrazione
				if(userRegistrazione(receive,sockId,&username)){//registrazione eseguita con successo
                			loginLog(fileLog,username);//stampo nel file log il login dell'utente

					send = messageCreate(2,NULL,NULL,NULL);//msg ok

					if(write(sockId, marshalling(send),strlen(marshalling(send)))<0){//invio messaggio al destinatario
                    				//impossibile contattare client - client disconnesso in manira anomala
                    				pthread_mutex_lock(&muxConnessi);
                    				numThreadAttivi--;
                    				pthread_mutex_unlock(&muxConnessi);

						userLogout(username);//elimino l'utente dalla lista di quelli connessi

                    				logoutLog(fileLog,username);//aggiorno il logfile con il logout dell'utente

						pthread_exit(NULL);
                			}
            			}
            			else{
                			send = messageCreate(3,NULL,NULL,"Errore nella registrazione");//messaggio di errore
                			write(sockId, marshalling(send),strlen(marshalling(send)));//invio messaggio al destinatario
                			pthread_mutex_lock(&muxConnessi);
                			numThreadAttivi--;
                			pthread_mutex_unlock(&muxConnessi);
                			pthread_exit(NULL);//uccido il thread in quanto il login è errato
            			}

        		break;
			case 'X'://richiesta di logout
				send = messageCreate(2,NULL,NULL,NULL);//msg ok
                		if(write(sockId, marshalling(send),strlen(marshalling(send)))<0){//invio messaggio al destinatario
                    				//impossibile contattare client - client disconnesso in manira anomala
                    				pthread_mutex_lock(&muxConnessi);
                    				numThreadAttivi--;
                    				pthread_mutex_unlock(&muxConnessi);

						userLogout(username);//elimino l'utente dalla lista di quelli connessi

                    				logoutLog(fileLog,username);//aggiorno il logfile con il logout dell'utente

						pthread_exit(NULL);
                		}
				pthread_mutex_lock(&muxConnessi);
            			numThreadAttivi--;
            			pthread_mutex_unlock(&muxConnessi);

				userLogout(username);//elimino l'utente dalla lista di quelli connessi

            			logoutLog(fileLog,username);//aggiorno il logfile con il logout dell'utente

            			pthread_exit(NULL);
			break;
			case 'I'://genera l'elenco degli utenti connessi
            			send=messageCreate(8,NULL,NULL,listaUtentiConnessi());

				if(write(sockId,marshalling(send),strlen(marshalling(send)))<0){
              				//aggiorno il numero di utenti connessi
              				pthread_mutex_lock(&muxConnessi);
              				numThreadAttivi--;
              				pthread_mutex_unlock(&muxConnessi);

              				logoutLog(fileLog,username);//logout su file log//logout su file log

              				userLogout(username);//elimino l'utente dalla lista di quelli connessi

              				pthread_exit(NULL);
				}
			break;
			case 'B'://broadcast
            			receive.sender = username;//aggiorno lo username
            			insert(&msgBuff, receive);//inserisco il messaggio nel buffer
          		break;
        		case 'S'://single mode
            			receive.sender = username;//aggiorno lo username
            			insert(&msgBuff, receive);//inserisco il messaggio nel buffer
          		break;

		}
	}

	pthread_exit(0);
}
