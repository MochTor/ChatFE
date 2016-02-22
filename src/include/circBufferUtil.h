#ifndef CIRCBUFFERUTIL_H
#define CIRCBUFFERUTIL_H

#include "CMTP.h"

// La struttura del buffer messaggi (MesBufStr)che contiene le variabili per poter lavorare con il buffer circolare, il buffer stesso, le variabili di condizione (se e' pieno o vuoto) e il mutex per l'accesso ad esso
typedef struct mesBufStr {
	msg_t * messageBuffer;//array che contiene i messaggi pendenti
	int readpos, writepos;//indice di lettura e scrittura
	int count;//numero messaggi pendenti
	int dimMessageBuffer;//dimensione dell'array 'messageBuffer'
	pthread_mutex_t buffMutex;
	pthread_cond_t FULL;
	pthread_cond_t EMPTY;
} mesBufStr;

void initStruct(mesBufStr *,int);//inizializzazione della struttura del buffer
void insert(mesBufStr *, msg_t);//inserisce un nuovo elemento (di tipo messaggio) nel buffer
msg_t extract(mesBufStr *);//estrae un elemento (di tipo messaggio) dal buffer

#endif
