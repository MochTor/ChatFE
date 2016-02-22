/**
 * circBufferUtil.c - The library to handle the circular buffer
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
#include <pthread.h>

#include "include/circBufferUtil.h"
#include "include/CMTP.h"

//Funzione per poter inizializzare correttamente la struttura del buffer
void initStruct(mesBufStr *buffer,int dim) {
	pthread_mutex_init(&buffer->buffMutex, NULL);
	pthread_cond_init(&buffer->FULL, NULL);
	pthread_cond_init(&buffer->EMPTY, NULL);
	buffer->readpos = 0;
	buffer->writepos = 0;
	buffer->count = 0;
	buffer->dimMessageBuffer = dim;
	buffer->messageBuffer = (msg_t*)malloc(sizeof(msg_t) * buffer->dimMessageBuffer);
}

//Funzione per inserire un nuovo elemento (di tipo messaggio) nel buffer. Viene eseguito l'accesso in mutua esclusione
void insert(mesBufStr *buffer, msg_t message){
	int mywritepos;

	pthread_mutex_lock(&buffer->buffMutex);
	while(buffer->count == buffer->dimMessageBuffer) {	/* Il buffer e' pieno, e visualizzo un messaggio di errore */
		pthread_cond_wait(&buffer->FULL, &buffer->buffMutex);	//Attendo che il buffer si liberi
	}
	mywritepos = buffer->writepos;
	buffer->writepos++;
	pthread_mutex_unlock(&buffer->buffMutex);

	//Memorizzo il messaggio in una cella del buffer
	buffer->messageBuffer[mywritepos] = message;

	pthread_mutex_lock(&buffer->buffMutex);
	// Incremento il numero di elementi
	buffer->count++;
	// Gestione in modulo del numero di elementi del buffer
	if(buffer->writepos >= buffer->dimMessageBuffer)
		buffer->writepos = 0;
	//Risveglio un eventuale thread consumatore in sospeso sulla condizione di buffer vuoto (che ora non e' piu' vuoto)
	pthread_cond_signal(&buffer->EMPTY);
	pthread_mutex_unlock(&buffer->buffMutex);
}

//Funzione per estrarre un elemento (di tipo messaggio) dal buffer
msg_t extract(mesBufStr *buffer) {
	msg_t msg;
	int myreadpos;

	pthread_mutex_lock(&buffer->buffMutex);
	while (buffer->count == 0) {
		pthread_cond_wait(&buffer->EMPTY, &buffer->buffMutex);	//Attendo che il buffer si riempia
	}
	myreadpos = buffer->readpos;
	buffer->readpos++;
	pthread_mutex_unlock(&buffer->buffMutex);

	//Leggo il messaggio e lo memorizzo nella variabile locale
	msg = buffer->messageBuffer[myreadpos];
	pthread_mutex_lock(&buffer->buffMutex);
	buffer->count--;

	//Gestione in modulo del numero di elementi del buffer
	if(buffer->readpos >= buffer->dimMessageBuffer)
		buffer->readpos = 0;
	//Risveglio un eventuale thread produttore in sospeso sulla condizione di buffer pieno (che ora non e' piu' pieno)
	pthread_cond_signal(&buffer->FULL);
	pthread_mutex_unlock(&buffer->buffMutex);
	//Ritorno il messaggio di tipo msg_t estratto dal buffer
	return msg;
}
