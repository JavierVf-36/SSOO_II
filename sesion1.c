#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/wait.h>
#include <sys/sem.h>
#include <signal.h>
#define NUMHIJOS 5

int semid; //guardar el ID del semaforo
pid_t pids [NUMHIJOS];

int eliminar_semaforos(int semid, int num_sem, int op){
    int sem=semctl(semid,num_sem,op);
    return sem;
}

void wait_semaforo(int semid, int num_sem){
    struct sembuf operacion;
    operacion.sem_num=num_sem;
    operacion.sem_op=-1;
    operacion.sem_flg=0;

    if(semop(semid, &operacion, 1)==-1){
        perror("Error en wait_semaforo");
        exit(EXIT_FAILURE);
    }
}

void signal_semaforo(int semid, int num_sem){
    struct sembuf operacion;
    operacion.sem_num=num_sem;
    operacion.sem_op=1;
    operacion.sem_flg=0;

    if(semop(semid, &operacion,1)==-1){
        perror("Error en signal_semaforo");
        exit(EXIT_FAILURE);
    }
}

void handler_sigint(int sig){

    /*
    * FUNCIONAMIENTO EXPLICADO: Como los hijos ignoran esta señal,
    * nos aseguramos que no entran aqui a dar por saco. El padre sigue 
    * recibiendo SIGINT, entra y mata a los hijos de manera controlada
    * con kill(pids[i], SIGKILL). Luego libera los semaforos y listo
    */

    for(int i=0;i<NUMHIJOS;i++){
        kill(pids[i], SIGKILL);
    }

    for (int i = 0; i < NUMHIJOS; i++) {
        wait(NULL);
    }

    printf("\nQue impaciente, ¿no quieres ver a los hijos?, pues nada, libero los semaforos.\n");
    if (eliminar_semaforos(semid,0,IPC_RMID) == -1) {
        perror("Error eliminando el semáforo");
        exit(EXIT_FAILURE);
    }
    exit(EXIT_SUCCESS);
}

int main(void){
    int ctl;
    semid=semget(IPC_PRIVATE, 1, IPC_CREAT|0600);

    if(semid<0){
        perror("Se ha producido un error.\n");
        return 1;
    }

    if (signal(SIGINT, handler_sigint) == SIG_ERR) {
        perror("No se pudo configurar el manejador de SIGINT");
        return 1;
    }

    ctl=semctl(semid,0,SETVAL,0);


    for(int i=0;i<NUMHIJOS;i++){
        pids[i]=fork();

        switch(pids[i]){
            case -1:
            printf("Error en la creacion del hijo.\n");
            return 1;
            break;

            case 0:
            signal(SIGINT, SIG_IGN); //Para que los hijos no terminen abruptamente
            wait_semaforo(semid,0);
            printf("Soy el hijo numero %d, mi PID es %d.\n",i+1, getpid());
            exit(0);
            break;

            default:
            break;
        }
    }

    sleep(7);
    for(int i=0;i<NUMHIJOS;i++){
        signal_semaforo(semid,0);
    }

    for (int i = 0; i < NUMHIJOS; i++) {
        wait(NULL);
    }

    printf("Me acabo de despertar, soy el padre. Mis hijos deben haber terminado ya.\n");

    eliminar_semaforos(semid,0,IPC_RMID);

    return 0;
}