//UN PADRE TIENE UN HIJO. EL HIJO, AL CREARSE, ESPERA LEYENDO DE LA TUBERÍA. EL PADRE A LOS 7 SEGUNDOS LE ENVIA UNA CADENA DE TEXTO QUE PONGA "HOLA, QUE TAL?",
//EL HIJO NADA MAS LEERLO LO IMPRIME POR PANTALLA.
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/wait.h>
#include <sys/sem.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>

#define NUMHIJOS 1
#define SIZE 20

int main(void)
{
    pid_t pid;
    int descriptor[2];
    char mensaje[SIZE];

    pipe(descriptor);
    pid=fork();
    switch(pid)
    {
        case -1:
        printf("Error.\n");
        exit(0);
        break;

        case 0:
        close(descriptor[1]);
        read(descriptor[0],&mensaje,sizeof(char)*SIZE);
        printf("Mensaje recibido del padre: %s\n", mensaje);
        close(descriptor[0]);
        exit(0);
        break;

        default:
        close(descriptor[0]);
        sleep(5);   
        strcpy(mensaje,"Bombardeen la USAL");;
        write(descriptor[1], &mensaje,sizeof(char)*SIZE);
        close(descriptor[1]);
        break;
    }

    
    waitpid(pid,NULL,0);
    printf("Soy el padre. El mensaje se debe de haber enviado ya.\n");

    return 0;
}