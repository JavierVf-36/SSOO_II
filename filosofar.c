#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/wait.h>
#include <sys/sem.h>
#include <unistd.h>
#include <signal.h>
#include <sys/shm.h>
#include "filosofar.h"

//./filosofar 4 4 4>salida (para guardar posibles errores en fichero de salida,creo)

int shm_inicio;
int sem_inicio;
pid_t pid_hijo;

void eliminar_sem(){
    int err=semctl(sem_inicio, 0, IPC_RMID);    
    if(err==-1){
        printf("Error al eliminar semáforo...\n");
        return;
    }

}



void liberar_mem(){
    int err=shmctl(shm_inicio, IPC_RMID, NULL);
    if(err==-1){
        printf("Error al liberar memoria compartida...\n");
        return;
    }
}



void manejadora_salida(int sig) {
    
    if(getpid()==pid_hijo){
        exit(0);
    }
    printf("\nHas pulsado CTRL+C. Eliminando semáforo y memoria compartida.\n");
    wait(NULL);
    eliminar_sem();
    liberar_mem();
    exit(0);
}



int main (int argc, char *argv[]){
    int err;
   //comprobar argumentos pasados al main
   //registrar señales a usar
   //obtener IPCs
   //FI_inicio con clave  41211294392005
   //crear procesos como se indique en el main
   //esperar muerte hijos sin cpu
   //eliminar IPC
   //llamar FI_fin
   


    
    //numero de filosofos, numero de vueltas por filosofo y lentitud de ejecucion>=0 (4)
    if(argc!=4){
        printf("Numero de parámetros incorrecto...\n");
        return -1;
    }

    int numFil=atoi(argv[1]);
    int numVuel=atoi(argv[2]);
    int lentitud=atoi(argv[3]);
    
    if(numFil<0 || numFil>MAXFILOSOFOS || numVuel<=0 || lentitud<0){
        printf("Parámetros incorrectos...\n");
        return -2;
    }

    //manejadora
    struct sigaction sa;
    sa.sa_handler = manejadora_salida;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);



    struct DatosSimulaciOn ddssp;
    ddssp.maxFilOsofosEnPuente=10;
    ddssp.maxUnaDirecciOnPuente=0;
    ddssp.sitiosTemplo=10;
    ddssp.nTenedores=5;


    int numSemaforos=FI_getNSemAforos();
    sem_inicio=semget(IPC_PRIVATE, numSemaforos, IPC_CREAT | 0600);
    if(sem_inicio<0){
        printf("Error al crear semaforos...\n");
        return -1;
    }


    int tamMemComp=FI_getTamaNoMemoriaCompartida();
    shm_inicio=shmget(IPC_PRIVATE, tamMemComp+3*sizeof(int), IPC_CREAT | 0600);
    if(shm_inicio<0){
        printf("Error al crear memoria compartida...\n");
        return -1;
    }
    //memoria compartida en encina, lo más ajustado a lo que usemos (multiplo de 4)


    //INICIO
    unsigned long long clave=41211294392005;
    err=FI_inicio(lentitud, clave, &ddssp, sem_inicio, shm_inicio, NULL);
    if(err ==-1){
        return -1;
    }



    pid_t pid=fork();
    if(pid==-1){
        return -1;
    }else if(pid==0){
        pid_hijo=getpid();
        err=FI_inicioFilOsofo(0);
        if(err ==-1){
            return -1;
        }
        
        
        int nVueltas=0;
        while(nVueltas<numVuel){
            err=FI_puedoAndar();
            if(err ==-1){
                return -1;
            }
            
            err=FI_pausaAndar();
            if(err ==-1){
                return -1;
                
            }

            int zona=FI_andar();
            if(zona ==-1){
                return -1;
            }else if(zona == ENTRADACOMEDOR){
                FI_entrarAlComedor(0);
                while(1){
                    err=FI_puedoAndar();
                    if(err ==-1){
                        return -1;
                    }
                    
                    err=FI_pausaAndar();
                    if(err ==-1){
                        return -1;
                    }

                    int zona2=FI_andar();
                    if(zona2==-1){
                        return -1;
                    }else if(zona2 == SILLACOMEDOR){
                        FI_cogerTenedor(TENEDORDERECHO);
                        FI_cogerTenedor(TENEDORIZQUIERDO);
                        int zona3;
                        do
                        {
                            zona3=FI_comer();
                        } while (zona3==SILLACOMEDOR);
                        //soltar tenedores
                        FI_dejarTenedor(TENEDORIZQUIERDO);
                        FI_dejarTenedor(TENEDORDERECHO);
                        break;
                    }
                    
                }
            }
            else if(zona==TEMPLO)
            {
                FI_entrarAlTemplo(0);
                while(1)
                {
                    err=FI_puedoAndar();
                    if(err ==-1){
                        return -1;
                    }
                    
                    err=FI_pausaAndar();
                    if(err ==-1){
                        return -1;
                    }

                    int zona2=FI_andar();
                    if(zona2==-1){
                        return -1;
                    }else if(zona2 == SITIOTEMPLO){
                        int zona3;
                        do
                        {
                            zona3=FI_meditar();
                        } while (zona3==SITIOTEMPLO);
                        nVueltas+=1;
                        break;
                        
                    }
                }
            }

        }
        err= FI_finFilOsofo();
        if(err ==-1){
            return -1;
        }

        return 0;
    }else{

        wait(NULL);
    }


    //ELIMINO IPCs
    eliminar_sem(sem_inicio);
    liberar_mem(shm_inicio);



    //FIN
    err=FI_fin();
    if(err<0){
        printf("Error al hacer FI_fin...\n");
        return -1;
    }



   return 0;
}
