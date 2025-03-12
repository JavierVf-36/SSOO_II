/*
**********************************************
*   ULTIMOS CAMBIOS GUARDADOS DE FILOSOFAR   *
*   POR SI ACASO LA CAGAMOS NO PERDEMOS NADA *
**********************************************
*/

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
int semid;
int numFil;
pid_t pidPadre;

void wait_semaforo(int semid, int num_sem){
    struct sembuf operacion;
    operacion.sem_num=num_sem;
    operacion.sem_op=-1;
    operacion.sem_flg=0;

    if(semop(semid, &operacion, 1)==-1)
    {
        perror("Error en wait_semaforo");
        exit(EXIT_FAILURE);
    }
}

void signal_semaforo(int semid, int num_sem){
    struct sembuf operacion;
    operacion.sem_num=num_sem;
    operacion.sem_op=1;
    operacion.sem_flg=0;

    if(semop(semid, &operacion,1)==-1)
    {
        perror("Error en signal_semaforo");
        exit(EXIT_FAILURE);
    }
}


void eliminar_sem(){
    int err=semctl(sem_inicio, 0, IPC_RMID);    
    if(err==-1)
    {
        printf("Error al eliminar semáforo...\n");
        return;
    }
    err=semctl(semid, 0, IPC_RMID);
    if(err==-1)
    {
        printf("Error al eliminar semáforo...\n");
        return;
    }
}



void liberar_mem(){
    int err=shmctl(shm_inicio, IPC_RMID, NULL);
    if(err==-1)
    {
        printf("Error al liberar memoria compartida...\n");
        return;
    }
}



void manejadora_salida(int sig) {
    
    for(int i=0;i<numFil;i++)
    {   
        if(getpid()==pidPadre)
        wait(NULL);
        else
        exit(0);
    }

    printf("\nHas pulsado CTRL+C. Eliminando semáforo y memoria compartida.\n");
    fflush(stdout);
    

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
   
    pidPadre=getpid();

    
    //numero de filosofos, numero de vueltas por filosofo y lentitud de ejecucion>=0 (4)
    if(argc!=4)
    {
        printf("Numero de parámetros incorrecto...\n");
        return -1;
    }

    numFil=atoi(argv[1]);
    int numVuel=atoi(argv[2]);
    int lentitud=atoi(argv[3]);
    pid_t pid_hijo[numFil];
    
    if(numFil<0 || numFil>MAXFILOSOFOS || numVuel<=0 || lentitud<0)
    {
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
    if(sem_inicio<0)
    {
        printf("Error al crear semaforos...\n");
        return -1;
    }

    semid=semget(IPC_PRIVATE, 1, IPC_CREAT|0600);
    if(semid==-1)
    {
        printf("Error al crear semaforo...\n");
        return -1;
    }

    int ctl;
    ctl=semctl(semid, 0, SETVAL, 0);   
    if(ctl==-1)
    {
        printf("Error al inicializar semáforo...\n");
        return -1;
    }


    int tamMemComp=FI_getTamaNoMemoriaCompartida();
    shm_inicio=shmget(IPC_PRIVATE, tamMemComp+sizeof(int), IPC_CREAT | 0600);
    if(shm_inicio<0)
    {
        printf("Error al crear memoria compartida...\n");
        return -1;
    }
    //memoria compartida en encina, lo más ajustado a lo que usemos (multiplo de 4)


    //INICIO
    unsigned long long clave=41211294392005;
    err=FI_inicio(lentitud, clave, &ddssp, sem_inicio, shm_inicio, NULL);
    if(err ==-1)
    {
        return -1;
    }


    int *memoria = (int *)shmat(shm_inicio, NULL, 0);
    if (memoria == (void *)-1)
    {
        perror("Error en shmat (hijo)");
        exit(1);
    }
    memoria[0]=0;

    

    for(int i=0;i<numFIl;i++){
        pid_t pid=fork();
        
        if(pid==0)
        {
            pid_hijo[i]=getpid();
            break;
        }
        else if(pid<=-1)
        {
            printf("Error al hacer fork...\n");
            return -1;
        }
    }

    int errFI_puedo,errFI_pausa;
    if(getpid()!=pidPadre){


        err=FI_inicioFilOsofo(memoriaH[0]++);
        if(err ==-1)
        {
            return -1;
        }
        
        int nVueltas=0;
        while(nVueltas<numVuel)
        {
            errFI_puedo=FI_puedoAndar();
            if(errFI_puedo>=0&&errFI_puedo<100)
            {
                wait_semaforo(semid,errFI_puedo);
            }
            else
            {
                signal_semaforo(semid,0);
            }

            

            if(errFI_puedo ==-1)
            {
                return -1;
            }
            
            errFI_pausa=FI_pausaAndar();
            if(errFI_pausa ==-1)
            {
                return -1;
                
            }

            int zona=FI_andar();
            if(zona ==-1)
            {
                return -1;
            }
            else if(zona == ENTRADACOMEDOR)
            {
                FI_entrarAlComedor(0);
                while(1)
                {
                    errFI_puedo=FI_puedoAndar();
                    if(errFI_puedo ==-1)
                    {
                        return -1;
                    }
                    
                    errFI_pausa=FI_pausaAndar();
                    if(errFI_pausa ==-1)
                    {
                        return -1;
                    }

                    int zona2=FI_andar();
                    if(zona2==-1)
                    {
                        return -1;
                    }
                    else if(zona2 == SILLACOMEDOR)
                    {
                        FI_cogerTenedor(TENEDORDERECHO);
                        FI_cogerTenedor(TENEDORIZQUIERDO);
                        int comiendo;
                        do
                        {
                            comiendo=FI_comer();
                        } while (comiendo==SILLACOMEDOR);
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
                    errFI_puedo=FI_puedoAndar();
                    if(errFI_puedo ==-1)
                    {
                        return -1;
                    }
                    
                    errFI_pausa=FI_pausaAndar();
                    if(errFI_pausa ==-1)
                    {
                        return -1;
                    }

                    int zona2=FI_andar();
                    if(zona2==-1)
                    {
                        return -1;
                    }
                    else if(zona2 == SITIOTEMPLO){
                        int meditar;
                        do
                        {
                            meditar=FI_meditar();
                        } while (meditar==SITIOTEMPLO);
                        nVueltas+=1;
                        break;
                        
                    }
                }
            }

        }
        err= FI_finFilOsofo();
        if(err ==-1)
        {
            return -1;
        }

        return 0;
    }
    else
    {
        for(int i=0;i<NUMHIJOS;i++)
        {   
        wait(NULL);
        }
    }


    //ELIMINO IPCs
    eliminar_sem(sem_inicio);
    shmdt(memoria);
    liberar_mem(shm_inicio);



    //FIN
    err=FI_fin();
    if(err<0)
    {
        printf("Error al hacer FI_fin...\n");
        return -1;
    }



   return 0;
}


/*
* Hay 3 casos: 
    1.- Ser el primero. Solo hara signals al anterior
    2.- Ser un intermedio. Hara wait del de delante y signal del de atras
    3.- Ser el ultimo. Solo hara wait del de delante
    Averiguar quien es quien (el identificador de cada uno) o el orden en el que se posicionan al principio
*
*
*   COSAS QUE HE CONSEGUIDO POR MI CUENTA 
    1.- Crea el numero de filosofos que le pasas por parametro (ayer se nos olvido comprobarlo)
*/
