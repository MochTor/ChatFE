#ifndef gestoreMessaggi
#define gestoreMessaggi

//-----------struttura definita nel messaggio-------
typedef struct messaggio{
    char type;             // message type
    char *sender;          // message sender
    char *receiver;        // message receiver
    unsigned int msglen;   // message length
    char *msg;             // message buffer
} msg_t;
//--------------------------------------------------
msg_t messageCreate(int, char*, char*, char*);
char* marshalling(msg_t);//data la struct msg restituisco una stringa frutto della concatenazione dei campi
msg_t unmarshalling(char[]);
void itoc4(int, char[]);
int ctoi4(char[], int);
void reverse(char[]);

#endif
