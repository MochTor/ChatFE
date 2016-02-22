/**
 * gestoreLogFile.c - The library to print server's actions onto log file
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
#include <sys/time.h>
#include <time.h>
#include <string.h>
#include <pthread.h>

#include "include/gestoreLogFile.h"

static pthread_mutex_t muxLog;
FILE * fp;	//Il file associato al file di log

void inizializzazioneLogFile(const char * filename){
	//stampa nel log file l'apertura del server

	time_t t;//contiene la data e ora in cui il server viene avviato
	char ts[64];//conterrà la data in formato stringa

	pthread_mutex_init(&muxLog, NULL);//inizializzazione mutex

	fp = fopen(filename,"w+");

	//prelevo la data e ora
	t = time(NULL);
	ctime_r(&t,ts);
	ts[strlen(ts)-1] = '\0';//remove new line

	fprintf(fp, "****************************************************\n");
	fprintf(fp, "**Chat Server started @%s **\n",ts);
	fprintf(fp, "****************************************************\n");

	//fclose(fp);
}

void loginLog(const char * filename, char *username){
	//aggiorno il log file con l'utente che ha appena fatto il login
	time_t t;//contiene la data e ora in cui il server viene avviato
	char ts[64];//conterrà la data in formato stringa

	pthread_mutex_lock(&muxLog);

	//prelevo la data e ora
	t = time(NULL);
	ctime_r(&t,ts);
	ts[strlen(ts)-1] = '\0';//remove new line

	fprintf(fp, "%s:login:%s\n", ts, username);

  	pthread_mutex_unlock(&muxLog);
}

void logoutLog(const char * filename,char *username)
{
	//stampa nel log file l'apertura del server
	time_t t;//contiene la data e ora in cui il server viene avviato
	char ts[64];//conterrà la data in formato stringa

	pthread_mutex_lock(&muxLog);

	//prelevo la data e ora
	t = time(NULL);
	ctime_r(&t,ts);
	ts[strlen(ts)-1] = '\0';//remove new line

  	fprintf(fp, "%s:logout:%s\n", ts, username);

  	pthread_mutex_unlock(&muxLog);
}

void singleLog(const char * filename,char *mittente, char *destinatario, char *messaggio)
{
	time_t t;//contiene la data e ora in cui il server viene avviato
	char ts[64];//conterrà la data in formato stringa

	pthread_mutex_lock(&muxLog);

	//prelevo la data e ora
	t = time(NULL);
	ctime_r(&t,ts);
	ts[strlen(ts)-1] = '\0';//remove new line

    fprintf(fp, "%s:%s:%s:%s\n", ts, mittente, destinatario, messaggio);
    pthread_mutex_unlock(&muxLog);
}

void logClose(){
	fclose(fp);
}
