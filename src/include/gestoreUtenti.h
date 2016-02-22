#ifndef gestoreUtenti
#define gestoreUtenti

#include "CMTP.h"

void readUserFile(char *);//carica dati da user file e li inserisce nella hash table
void updateUserFile(char *);//aggiorna lo userfile 
int userLogin(char *,int);//controllo di login dell'utente con annessi controlli
int userRegistrazione(msg_t ,int,char **);//registrazione dell'utente con annessi controlli
char * listaUtentiConnessi();//restituisce una stringa frutto della concatenazione degli username degli utenti connessi
void userLogout(char * );//effettua il logout
int getSock(char *);//restituisce il valore del sock associato ad uno username


#endif
