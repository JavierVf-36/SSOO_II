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

#define NUMHIJOS 2
int semid; //id del semaforo

int eliminar_semaforos(int semid, int num_sem, int op){
    int sem = semctl(semid, num_sem, op);
    return sem;
}

void wait_semaforo(int semid, int num_sem){
    struct sembuf operacion;
    operacion.sem_num = num_sem;
    operacion.sem_op = -1;
    operacion.sem_flg = 0;

    if (semop(semid, &operacion, 1) == -1) {
        perror("Error en wait_semaforo");
        exit(EXIT_FAILURE);
    }
}

void signal_semaforo(int semid, int num_sem){
    struct sembuf operacion;
    operacion.sem_num = num_sem;
    operacion.sem_op = 1;
    operacion.sem_flg = 0;

    if (semop(semid, &operacion, 1) == -1) {
        perror("Error en signal_semaforo");
        exit(EXIT_FAILURE);
    }
}

void manejadora_salida(int sig)
{
    printf("\nHas pulsado CTRL+C. Eliminando semaforo y saliendo.\n");
    for (int i = 0; i < NUMHIJOS; i++) {
        wait(NULL);
    }
    eliminar_semaforos(semid, 0, IPC_RMID);
    exit(0);
}


int main(void)
{
    pid_t pids[NUMHIJOS];

    /*
    ***************************************
    * CAPTURADORA DE SEÑAL SIGINT (CTRL C)*
    ***************************************
    */

    struct sigaction sa;
    sa.sa_handler = manejadora_salida;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);

    semid = semget(IPC_PRIVATE, 2, IPC_CREAT | 0600); //CREAMOS SEMAFOROS, EN ESTE CASO 2

    if (semid < 0) {
        perror("Se ha producido un error.\n");
        return 1;
    }
    
    int ctl, ctl2;
    ctl = semctl(semid, 0, SETVAL, 1);  // Para permitir que el hijo 0 empiece
    ctl2 = semctl(semid, 1, SETVAL, 0); // Para que el hijo 1 espera

    for (int i = 0; i < NUMHIJOS; i++) {
        pids[i] = fork();
        int j = 0;
        switch (pids[i]) {
            case -1:
                printf("Error al crear un hijo.\n");
                eliminar_semaforos(semid, 0, IPC_RMID);
                return 1;
                break;

            case 0:
                switch (i) {
                    case 0:
                        while (j < 50) {
                            wait_semaforo(semid, 1);
                            printf("0");
                            fflush(stdout);
                            j++;
                            signal_semaforo(semid, 0);  
                        }
                        break;

                    case 1:
                        while (j < 50) {
                            wait_semaforo(semid, 0);
                            printf("1");
                            fflush(stdout);
                            j++;
                            signal_semaforo(semid, 1);
                        }
                        break;
                }
                exit(0);
                break;
        }
    }

    for (int i = 0; i < NUMHIJOS; i++) {
        wait(NULL);  // Espera a que los hijos terminen
    }
    printf("\n");
    eliminar_semaforos(semid, 0, IPC_RMID);  // Elimina los semáforos
    return 0;
}
