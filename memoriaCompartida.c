#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/wait.h>
#include <sys/sem.h>
#include <signal.h>
#include <sys/shm.h>

int semid; // id del semáforo
int memid; // id de la memoria compartida

#define NUMHIJOS 1

int eliminar_semaforos(int semid, int num_sem, int op) {
    return semctl(semid, num_sem, op);
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


void manejadora_salida(int sig) {
    printf("\nHas pulsado CTRL+C. Eliminando semáforo y memoria compartida.\n");
    eliminar_semaforos(semid, 0, IPC_RMID);
    shmctl(memid, IPC_RMID, NULL);
    exit(0);
}

int obtenerValorRandom() {
    return rand() % 50;
}

int main(void) {
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


    /*
    ***************************************
    *              SEMAFOROS              *
    ***************************************
    */
    
    semid = semget(IPC_PRIVATE, 1, IPC_CREAT | 0600);
    if (semid < 0) {
        perror("Error al crear semáforo");
        return 1;
    }
    semctl(semid, 0, SETVAL, 0);  // Inicializar en 0 para que el padre espere

    
    /*
    ***************************************
    *         MEMORIA COMPARTIDA          *
    ***************************************
    */

    memid = shmget(IPC_PRIVATE, 2 * sizeof(int), IPC_CREAT | 0600);
    if (memid < 0) {
        perror("Error al crear memoria compartida");
        eliminar_semaforos(semid, 0, IPC_RMID);
        return 1;
    }

    //Para el padre
    int *shared_memory = (int *)shmat(memid, NULL, 0);
    if (shared_memory == (void *)-1) {
        perror("Error en shmat (padre)");
        eliminar_semaforos(semid, 0, IPC_RMID);
        shmctl(memid, IPC_RMID, NULL);
        return 1;
    }

    for (int i = 0; i < NUMHIJOS; i++) {
        pids[i] = fork();

        switch (pids[i]) {
            case -1:
                perror("Error al crear un hijo");
                eliminar_semaforos(semid, 0, IPC_RMID);
                shmctl(memid, IPC_RMID, NULL);
                return 1;
                break;

            case 0:
                srand(getpid());
                int num1 = obtenerValorRandom();
                int num2 = obtenerValorRandom();

                // Adjuntar la memoria en el hijo
                int *shared_memory_hijo = (int *)shmat(memid, NULL, 0);
                if (shared_memory_hijo == (void *)-1) {
                    perror("Error en shmat (hijo)");
                    exit(1);
                }

                printf("Soy el hijo %d, mi PID es %d.\n", i + 1, getpid());
                printf("Voy a enviar los números %d y %d.\n", num1, num2);

                shared_memory_hijo[0] = num1;
                shared_memory_hijo[1] = num2;

                signal_semaforo(semid, 0);

                shmdt(shared_memory_hijo);
                exit(0);
                break;
            
        }
    }

    wait_semaforo(semid, 0);

    for (int i = 0; i < NUMHIJOS; i++) {
        wait(NULL);
    }

    printf("Soy el padre. Leo los valores: %d y %d\n", shared_memory[0], shared_memory[1]);


    shmdt(shared_memory);
    shmctl(memid, IPC_RMID, NULL);
    eliminar_semaforos(semid, 0, IPC_RMID);

    return 0;
}
