#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/wait.h>
#include <sys/sem.h>
#include <sys/msg.h>
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

typedef struct memoria
{
    int tenedores[5];
    int platos_libres[5];
    infoFils infoFil[21];
}memoria;

typedef struct mensaje
{
    long tipo;
    int info;
}mensaje;


int shm_inicio;
int sem_inicio;
int semid;
int numFil;
pid_t pidPadre;
int buzon;

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

int eliminar_buzon(int buzon)
{
    return msgctl(buzon,IPC_RMID,NULL);
}

int localizarSignal(pid_t pid, int numFil)
{
    memoria *memm= (memoria *)shmat(shm_inicio, NULL, 0);
    if (memm == (void *)-1)
    {
        perror("Error en shmat (hijo)");
        exit(1);
    }

    for(int i=0;i<numFil;i++)
    {
        if(memm->infoFil[i].pidFil==pid)
        {
            //printf("¡Me encontre!, mi idFil es: %d\n", memoria[i].idFil);
            //fflush(stdout);
            return memm->infoFil[i].idFil;
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

    printf("\nHas pulsado CTRL+C. Eliminando semáforo,memoria compartida y buzones.\n");
    fflush(stdout);
    

    eliminar_sem();
    liberar_mem();
    eliminar_buzon(buzon);

    int err=FI_fin();
    if(err<0)
    {
        printf("Error al hacer FI_fin...\n");
        return -1;
    }
    exit(0);
}



int main (int argc, char *argv[]){
    int err;
    mensaje msg;
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
    

    semid=semget(IPC_PRIVATE, 4, IPC_CREAT|0600);
    if(semid==-1)
    {
        printf("Error al crear semaforo...\n");
        return -1;
    }
     
    int ctl;
    ctl=semctl(semid,0,SETVAL,1); //semaforo de andar

    if(ctl==-1)
    {
        perror("Error al asignar el contador del semaforo.\n");
        return 1;
    }

    ctl=semctl(semid,1,SETVAL,2);   //semaforo de puente

    if(ctl==-1)
    {
        perror("Error al asignar el contador del semaforo.\n");
        return 1;
    }

    ctl=semctl(semid,2,SETVAL,4);   //semaforo antesala

    if(ctl==-1)
    {
        perror("Error al asignar el contador del semaforo.\n");
        return 1;
    }

    ctl=semctl(semid,3,SETVAL,1);   //semaforo eleccion plato
    if(ctl==-1)
    {
        perror("Error al asignar el contador del semaforo.\n");
        return 1;
    }



    memoria memoriam;
    
    int tamMemComp=FI_getTamaNoMemoriaCompartida();
    
    for(int i=0; i<5; i++){
        memoriam.platos_libres[i]=0;
    }

    

    for(int i=0; i<numFil; i++){    //filosofos que no existeb a -1
        memoriam.infoFil[i].pidFil=-1;
        memoriam.infoFil[i].idFil=-1;
    }

    shm_inicio=shmget(IPC_PRIVATE, tamMemComp+sizeof(memoriam), IPC_CREAT | 0600);
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


    buzon=msgget(IPC_PRIVATE,IPC_CREAT | 0600);
    if(buzon==-1)
    {
        printf("Error al crear el buzon.\n");
        return 1;
    }





    

    
    //printf("\n");
    //fflush(stdout);
    for(int i=0;i<numFil;i++){
        pid_t pid=fork();
        
        if(pid==0)
        {
            
            memoria *mem = (memoria *)shmat(shm_inicio, NULL, 0);
            if (mem == (void *)-1)
            {
                perror("Error en shmat (hijo)");
                exit(1);
            }


            for(int i=0;i<5;i++)
            {
                mem->platos_libres[i]=0;
                mem->tenedores[i]=0;
            }

            mem->infoFil[i].pidFil=getpid();
            mem->infoFil[i].idFil=i;
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

            shmdt(mem);
            break;
        }
        else if(pid<=-1)
        {
            printf("Error al hacer fork...\n");
            return -1;
        }
        else
        {
            pid_hijo[i]=getpid();
        }
    }

    int errFI_puedo,errFI_pausa;
    if(getpid()!=pidPadre)
    {
        
        int nVueltas=0;
        int zona=-1;
        int zonaPrevia;
        while(nVueltas<numVuel)
        {
            
            errFI_puedo=FI_puedoAndar();
            errFI_pausa=FI_pausaAndar(); 
            
            int idFil=localizarSignal(getpid(),numFil); 

            if(idFil==0)
            {
                idFil=30;
            }

            if(errFI_puedo!=100)
            {   //NO PUEDES ANDAR

                if(errFI_puedo==0)
                {
                    errFI_puedo=30;
                }

                msg.tipo=errFI_puedo;
                msg.info=idFil;
                //printf("He entrado. Soy %d y %d no me deja moverme.\n",idFil,errFI_puedo);
                //fflush(stdout);
                int enviado=msgsnd(buzon,&msg,sizeof(mensaje)-sizeof(long),0);
                if(enviado==0)
                {
                    //printf("He avisado a %d de que no me puedo mover.\n",errFI_puedo);
                    //fflush(stdout); 
                }


                mensaje recibido;
                do
                {    
                    msgrcv(buzon,&recibido,sizeof(mensaje)-sizeof(long),80,0);
                    if(recibido.info!=idFil)
                    {
                        msgsnd(buzon,&recibido,sizeof(mensaje)-sizeof(long),0);
                    }
                }while(recibido.info!=idFil);
                //printf("%d me ha avisado de que me mueva. soy %d\n",errFI_puedo,idFil);
                //fflush(stdout);
            }
                //PUEDES ANDAR
                zonaPrevia=zona;
                zona=FI_andar();
                //printf("Soy %d y he andado. aviso de que he andado\n",idFil);
                //fflush(stdout);
                //comprueba si debe avisar a alguien que ha andado
                int recibido=msgrcv(buzon,&msg,sizeof(mensaje)-sizeof(long),idFil,IPC_NOWAIT);
                if(recibido>0) //si no recibe nada, sigue adelante
                {
                    mensaje aAvisar;
                    aAvisar.tipo=80;
                    aAvisar.info=msg.info;
                    //printf("Recibido. voy a avisar a %d de que se mueva. soy %d\n",msg.tipo,idFil);
                    //fflush(stdout);
                    msgsnd(buzon,&aAvisar,sizeof(mensaje)-sizeof(long),0);
                }else
                {
                    //printf("No habia nadie a quien avisar que andara. soy %d\n",idFil);
                    //fflush(stdout);
                }
            

                if(zonaPrevia==CAMPO&&zona==PUENTE)
                {
                    wait_semaforo(semid,1);
                }

                if(zonaPrevia==PUENTE&&zona==CAMPO)
                {
                    signal_semaforo(semid,1);
                }


            if(errFI_puedo ==-1)
            {
                return -1;
            }

            if(errFI_pausa ==-1)
            {;
                return -1;     
            }

                
             

            //seccion critica para elegir sitio en comedor
            //seccion critica en comedor y antesala



            //-------------------------------------//

            
            if(zona==ANTESALA){
            
                wait_semaforo(semid,2);//entra en la antesala o espera fuera
                //printf("Filosofo %d en la antesala\n", idFil);
                //fflush(stdout);
                //seccion critica para elegir si se puede entrar en el comedor
                
                //entrara a la zona critica de elegir si se puede entrara en el comedor
                //salir y entrar al comedor
                signal_semaforo(semid,2);//salir de antesala
                
            }


            if(zona == ENTRADACOMEDOR)
            {   

                memoria *mem = (memoria *)shmat(shm_inicio, NULL, 0);
                if (mem == (void *)-1)
                {
                    perror("Error en shmat (hijo)");
                    exit(1);
                }
                
                
                
                wait_semaforo(semid,3);
                int plato=-1;
                for(int i=0;i<5;i++)
                {
                   if (mem->platos_libres[i] == 0) {
                        plato = i;
                        mem->platos_libres[i] = getpid();  // Ocupar el plato
                        break;
                   }   
                    
                }


                //si plato es -1 está esperando en la antesala, no encontró plato
                
                FI_entrarAlComedor(plato);                
                signal_semaforo(semid,3);
                
                

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
                        
                        if(plato!=-1){
                            int tenedor_izq=plato;
                            int tenedor_der=(plato+4) % 5;
                        

                            while(1){
                                wait_semaforo(semid, 3);
                                if(mem->tenedores[tenedor_izq]==0 && mem->tenedores[tenedor_der]==0){
                                    mem->tenedores[tenedor_izq]=getpid();
                                    mem->tenedores[tenedor_der]=getpid();
                                    signal_semaforo(semid, 3);
                                    break;
                                }
                                signal_semaforo(semid, 3);

                            }


                            FI_cogerTenedor(TENEDORIZQUIERDO);
                            FI_cogerTenedor(TENEDORDERECHO);
                            
                            int comiendo;

                            do{
                                comiendo=FI_comer();
                            }while (comiendo==SILLACOMEDOR);
                            

                            wait_semaforo(semid,3);
                            FI_dejarTenedor(TENEDORDERECHO);
                            FI_dejarTenedor(TENEDORIZQUIERDO);
                            mem->tenedores[tenedor_der]=0;
                            mem->tenedores[tenedor_izq]=0;
                            signal_semaforo(semid,3);

                            mem->platos_libres[plato]=0;


                        }            
            
                       
                        //eliminar puntero memoria
                        shmdt(mem);
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
            else if(zona ==-1)
            {
                return -1;
            }

        }

        //FIN DEL CIRCUITO
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
    
    liberar_mem(shm_inicio);
    eliminar_buzon(buzon);


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
    Vale creo que si funciona, hasta que no confirmes tu tambien no aseguro nada por si acaso
    3.- Los pendejitos andan sin chocarse 
    */
