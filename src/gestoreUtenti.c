/**
 * gestoreUtenti.c - The library used to manage users connected to server
 * used in Operating Systems final project for OS class
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
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#include "include/gestoreUtenti.h"
#include "include/common.h"
#include "include/hash.h"	//Gestisce completamente l'hash table
#include "include/CMTP.h"

static pthread_mutex_t mux;//mutua esclusione su lettura hash

static hash_t table;//rappresenta l'hash table -> rappresenta lo user file
static lista utentiConnessi;//lista di tutti gli utenti connessi
static lista utentiRegistrati;//lista degli utenti registrati in questa sessione
static int num_of_utentiConnessi;//indica il numero di utenti connessi
static int num_of_utentiRegistrati;//numero di utenti di registrati durante questa sessione

void readUserFile(char * filename){
	FILE * fp;//puntera al file user-name
	//posizione ultimoRegistrato;//posizione dell'ultimo utente inserito nella lista
    	hdata_t *utente;//contiene i dati corrispondenti a una riga scomposta letti dallo stream del file
    	char stringa[771]; //rappresenta una linea letta dal file 256

    	pthread_mutex_init(&mux, NULL);

	//apertura user-file / se non esiste viene creato
    	fp = fopen(filename, "r");
    	if (fp == NULL)	fp = fopen(filename, "w");

    	utentiConnessi = CREALISTA();
    	utentiRegistrati = CREALISTA();
    	table = CREAHASH();
    	num_of_utentiConnessi = 0;
    	num_of_utentiRegistrati = 0;

    	//lettura file riga per riga
    	while (fgets (stringa, sizeof(stringa), fp)){
        	utente = (hdata_t *)malloc(sizeof(hdata_t));

        	//----inizializzazione utente------------
        	utente->uname = strdup(strtok(stringa, ":"));
        	utente->fullname = strdup(strtok(NULL, ":"));
        	utente->email = strdup(strtok(NULL, ":\n"));//evito che vi sia \n nella mail
        	utente->sockid = -1;//al momento del caricamento tutti gli utenti sono offline
        	//-----------------------------------------

        	INSERISCIHASH(utente->uname, (void*) utente, table);//inserisco l'utente nella hashtable
    	}

    	fclose(fp);//chiusura puntatore a file
}


void updateUserFile(char *filename){
    //aggiorno il file userfile
    FILE * fp;
    posizione elemento;//rappresenta l'elemento della lista degli utenti registrati
    hdata_t * risultato;//contiene le informazioni della hashtable
    int i;//contatore generico
    fp = fopen(filename,"a");

    elemento = PRIMOLISTA(utentiRegistrati);//prelevo il primo elemento

    risultato = (hdata_t *)malloc(sizeof(hdata_t));
    for(i=0;i < num_of_utentiRegistrati;i++){
        risultato = CERCAHASH(elemento->elemento,table);
        fprintf(fp,"%s:%s:%s\n", risultato->uname,risultato->fullname,risultato->email);
        elemento = SUCCLISTA(elemento);//sfoglio l'elemento
    }

    fclose(fp);
}

int userLogin(char * username,int sock){
    hdata_t *risultato;//risultato della ricerca sulla hastable
    posizione elemento;//conterrà la posizione dell'ultimo utente connesso
    int valido;//variabile booleana che rappresenta il risultato dell'operazione

    valido = 0;

    pthread_mutex_lock(&mux);//evito race condition

    risultato = CERCAHASH(username, table);//cerco all'interno della tabella se esiste l'utente
    elemento = ULTIMOLISTA(utentiConnessi);//prelevo l'ultimo elemento della lista degli utenti connessi

    //ora controllo che l'utente sia nella lista e che non sia connesso già attualmente
    if(risultato != NULL && risultato->sockid == -1) {

        risultato->sockid = sock;//login eseguito correttamente quindi aggiorno il suo sock id

        INSLISTA(username, &elemento);//aggiorno la lista degli utenti connessi
        num_of_utentiConnessi++;//incremento il numero di utenti connessi

        valido = 1;
    }

    pthread_mutex_unlock(&mux);//sblocco il mux

    return valido;
}

int userRegistrazione(msg_t msg, int sock,char ** username){
    	int valido;//booleano indica se l'operazione è andata a buon fine
    	hdata_t * risultato;
    	posizione elReg;//elemento della lista utenti registrati
    	posizione elConn;//elemento lista utenti connessi
    	char * token;
    	valido = 0;

    	risultato = (hdata_t *)malloc(sizeof(hdata_t));

	//prelevo lo username
	token = strtok(msg.msg, ":");
	*username = strdup(token);


    	pthread_mutex_lock(&mux);
    	risultato = CERCAHASH(*username, table);
    	pthread_mutex_unlock(&mux);

    	if(!risultato){
        	valido = 1;

        	//riempio la struttura che verrà inserita nella hash table
        	risultato = (hdata_t *)malloc(sizeof(hdata_t));
        	risultato->uname = *username;//inserisco lo username
		//printf("uname %s\n",risultato->uname);
        	token = strtok(NULL,":");
		risultato->fullname= strdup(token);//printf("uname %s\n",risultato->fullname);

		token = strtok(NULL,"\n\0");
		risultato->email= strdup(token);

		risultato->sockid   = sock;


        	pthread_mutex_lock(&mux);
        	num_of_utentiRegistrati++;
        	num_of_utentiConnessi++;
        	elReg = ULTIMOLISTA(utentiRegistrati);//prendo l'ultimo elemento
        	elConn = ULTIMOLISTA(utentiConnessi);//prendo l'ultimo elemento
        	INSLISTA(risultato->uname, &elReg);//inserisco nella lista utenti registrati
        	INSLISTA(risultato->uname, &elConn);//inserisco nella lista utenti connessi
        	INSERISCIHASH(risultato->uname, (void*) risultato, table); //inserisco struttura nella hash table
        	pthread_mutex_unlock(&mux);
    	}

    	return valido;
}

char * listaUtentiConnessi(){
    	char * stringa = NULL;//contiene il risultato della concatenazione del nome di tutti gli utenti connessi
	char * tmp;
	posizione utente;//indica la posizione del primo elemento della lista utenti connessi

    	int i;//contatore generico

    	pthread_mutex_lock(&mux);//evito race condition su utentiConnessi e listaUtentiCOnnessi

    	utente = PRIMOLISTA(utentiConnessi);//prelevo il primo elemento della lista

    	//analizzo ogni singolo utente connessi
	for(i = 0;i <num_of_utentiConnessi;i++){
		if(i != 0){
			utente = SUCCLISTA(utente);//sfoglio la lista di un elemento
			tmp = (char *)malloc(strlen(stringa) + strlen(utente->elemento) + 1);
			strcpy(tmp,stringa);
			strcat(tmp,":");
			strcat(tmp,utente->elemento);
			free(stringa);
			stringa = strdup(tmp);
			free(tmp);
		}
		else
			stringa = strdup(utente->elemento);
	}

    	pthread_mutex_unlock(&mux);

    	return stringa;
}

void userLogout(char * username){
    hdata_t * risultato;//rappresenta il risultato della ricerca dello username sulla hashtable
    posizione elemento;//rappresenta l'elemento della lista utentiConnessi che devo eliminare
    int trovato;//booleano che indica se abbiamo trovato l'elemento all'interno della lista
    int i;//contatore generico

    pthread_mutex_lock(&mux);//evito race condition
    elemento = PRIMOLISTA(utentiConnessi);//prendo il primo elemento della lista

    //cerco l'utente all'interno della tabella utenti connessi
    risultato = CERCAHASH(username, table);
    if(risultato != NULL){
        //ci siamo accertati che l'utente esiste->aggiorno sock
        risultato->sockid = -1;

        //cerco l'utente all'interno della lista e lo elimino
        trovato = 0;//FALSE
        i =0;
        while(i < num_of_utentiConnessi && !trovato){
            if(strcmp(elemento->elemento, username) == 0) {//elemento trovato
                CANCLISTA(&elemento);//elimino l'elemento dalla lista
                trovato = 1;//TRUE
            }
            else{
                elemento = SUCCLISTA(elemento);//sfoglio gli elementi
                i++;
            }
        }
        if(trovato) num_of_utentiConnessi--;//aggiorno il numero di utenti connessi
    }
    pthread_mutex_unlock(&mux);
}

int getSock(char * username){
    //restituisce l'id del socket
    int sockid;
    hdata_t *utente;

    sockid = -1;

    utente = CERCAHASH(username,table);
    if(utente!= NULL){
        sockid = utente->sockid;
    }

    return sockid;
}
