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

typedef struct infoFils
{
    pid_t pidFil;
    int idFil;
}infoFils;

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

int localizarSignal(pid_t pid, int numFil)
{
    infoFils *memoria = (infoFils *)shmat(shm_inicio, NULL, 0);
    if (memoria == (void *)-1)
    {
        perror("Error en shmat (hijo)");
        exit(1);
    }

    for(int i=0;i<numFil;i++)
    {
        if(memoria[i].pidFil==pid)
        {
            printf("¡Me encontre!, mi idFil es: %d\n", memoria[i].idFil);
            fflush(stdout);
            return memoria[i].idFil;
        }
    }
    return -1;
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

    semid=semget(IPC_PRIVATE, numFil-1, IPC_CREAT|0600);
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
    shm_inicio=shmget(IPC_PRIVATE, tamMemComp+numFil*sizeof(infoFils), IPC_CREAT | 0600);
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


    infoFils *memoria = (infoFils *)shmat(shm_inicio, NULL, 0);
    if (memoria == (void *)-1)
    {
        perror("Error en shmat (hijo)");
        exit(1);
    }

    
    printf("\n");
    fflush(stdout);
    for(int i=0;i<numFil;i++){
        pid_t pid=fork();
        
        if(pid==0)
        {
            pid_hijo[i]=getpid();
            infoFils infoFil;
            infoFil.pidFil=getpid();
            infoFil.idFil=i;
            printf("|%d|%d|\n",getpid(),i);
            fflush(stdout);
            memoria[i]=infoFil;
            err=FI_inicioFilOsofo(i);
            if(err ==-1)
            {
                return -1;
            }

            //En memoria compartida se esta guardando correctamente:
            //1.- El pid del filosofo
            //2.- El identificador del filosofo
            //Basta que un filosofo recorra la memoria 
            //buscando su PID para saber que identificador tiene
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

        int *memoriaH = (int *)shmat(shm_inicio, NULL, 0);
        if (memoriaH == (void *)-1)
        {
            perror("Error en shmat (hijo)");
            exit(1);
        }
        
        shmdt(memoriaH);
        
        int nVueltas=0;
        while(nVueltas<numVuel)
        {
            printf("\n");
            fflush(stdout);
            errFI_puedo=FI_puedoAndar();
            if(errFI_puedo>=0&&errFI_puedo<100)
            {
                printf("Soy %d y no puedo andar. Me buscare.\n",getpid());
                fflush(stdout);
                int idFil=localizarSignal(getpid(),numFil);

                //El primero nunca la va a ser imposible moverse, ni le compruebo
                if(idFil>0&&idFil<numFil-1)
                {
                    //EL DE DELANTE
                    wait_semaforo(semid,errFI_puedo);
                    //Despues de esperar, SE MUEVE Y AVISA AL DE DETRAS
                    signal_semaforo(semid,idFil);
                }else if(idFil==numFil-1)
                {
                    wait_semaforo(semid, errFI_puedo);
                }
            }
            else
            {
                printf("Soy %d,puedo andar y me tengo que buscar\n",getpid());
                fflush(stdout);
                int idFil=localizarSignal(getpid(),numFil);
                
                if(idFil==-1)
                {
                    printf("Filosofo no encontrado\n");
                    fflush(stdout);
                    return 1;
                }
                else if(idFil==0) 
                {   //PRIMER FILOSOFO, solo signal
                    printf("Soy %d, primero y le hago un signal al semaforo %d\n",getpid(),idFil);
                    fflush(stdout);
                    signal_semaforo(semid,idFil);
                }
                else if(idFil>0&&idFil<numFil-1)
                {   //FILOSOFOS INTERMEDIOS
                    printf("Soy %d, intermedios y le hago un wait al semaforo %d signal al semaforo %d\n",getpid(),idFil,idFil-1);
                    fflush(stdout);
                    wait_semaforo(semid,idFil);
                    signal_semaforo(semid,idFil-1);
                }
                else if(idFil>0&&idFil==numFil-1)
                {   //FILOSOFO FINAL
                    printf("Soy %d, final y le hago un signal al semaforo %d\n",getpid(),idFil);
                    fflush(stdout);
                    wait_semaforo(semid,idFil);
                }
                
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
        for(int i=0;i<numFil;i++)
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
    1.- Crea el numero de filosofos que le pasas por parametro (ayer se nos olvido comprobarlo y no lo hacia)
    2.- Guardar pid de filosofos + id de filosofos en un struct para añadirlo en memoria compartida
    Lo he hecho para que se pueda localizar a si mismo el filosofo. Basta con que entre en la memoria
    recorra el bucle hasta que encuente su pid dentro de el mismo, cuando eso suceda, entonces podra
    mirar el otro campo del struct y conocer quien es (es la idea, no se 100% asegurado si funciona)
    */
