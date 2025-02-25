#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/wait.h>
#include <sys/sem.h>
#include <signal.h>
#include <unistd.h>
#include <sys/msg.h>
#include <time.h>

#define NUMHIJOS 10
int buzon;

typedef struct mensaje
{
    long tipo;
    int numRandom;
}mensaje;

int eliminar_buzon(int buzon)
{
    return msgctl(buzon,IPC_RMID,NULL);
}


int asignarNumeroRandom()
{
    return rand()%15+1;
}


void manejadora_salida(int sig)
{
    printf("\nHas pulsado CTRL+C. Eliminando buzon y saliendo.\n");
    for(int i=0;i<NUMHIJOS;i++)
    {
        wait(NULL);
    }
    eliminar_buzon(buzon);
    exit(0);
}

int main(void)
{
    /*
    ***************************************
    * CAPTURADORA DE SEÑAL SIGINT (CTRL C)*
    ***************************************
    */

    struct sigaction sa;
    sa.sa_handler=manejadora_salida;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags=0;
    sigaction(SIGINT,&sa,NULL);

    pid_t pid[NUMHIJOS];
    mensaje msg;
    int resultado=0;

    buzon=msgget(IPC_PRIVATE,IPC_CREAT | 0600);
    if(buzon==-1)
    {
        printf("Error al crear el buzon.\n");
        return 1;
    }


    for(int i=0;i<NUMHIJOS;i++)
    {
        pid[i]=fork();

        switch(pid[i])
        {
            case -1:
            printf("Error al crear proceso hijo.\n");
            return 1;
            break;

            case 0:
            srand(getpid()); //PARA LOGRAR QUE CADA PROCESO ENVIE UN NUMERO ALEATORIO DIFERENTE
            msg.numRandom=asignarNumeroRandom();
            msg.tipo=5;
            int enviado=msgsnd(buzon,&msg,sizeof(mensaje)-sizeof(long),0);
            if(enviado==0)
            {
                printf("Soy el hijo numero %d, con PID %d. Numero a enviar: %d\n",i+1, getpid(), msg.numRandom);
            }
            exit(0);
            break;
        }
    }

    sleep(4);

    for(int i=0;i<NUMHIJOS;i++)
    {
        msgrcv(buzon,&msg,sizeof(mensaje)-sizeof(long),5,0);
        resultado=resultado+msg.numRandom;
    }

    for(int i=0;i<NUMHIJOS;i++)
    {
        wait(NULL);
    }

    printf("He recibido las cartas de mis hijos. Resultado: %d\n", resultado);

    int eliminado=msgctl(buzon,IPC_RMID,NULL);
    return 0;
}