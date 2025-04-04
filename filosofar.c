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
    int sentido_puente; //dentro del puente
    int contador_personas;
    int espera;
    int tenedores[5];
    int platos_libres[5];
    infoFils infoFil[21];
    
    int sitios_templo[3];
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

void wait_cero(int semid, int num_sem)
{
    struct sembuf operacion;
    operacion.sem_num=num_sem;
    operacion.sem_op=0;
    operacion.sem_flg=0;

    if(semop(semid, &operacion, 1)==-1)
    {
        perror("Error en wait_semaforo");
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
    shmdt(memm);
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
        return;
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
    ddssp.maxFilOsofosEnPuente=2;
    ddssp.maxUnaDirecciOnPuente=0;
    ddssp.sitiosTemplo=3;
    ddssp.nTenedores=5;


    int numSemaforos=FI_getNSemAforos();
    sem_inicio=semget(IPC_PRIVATE, numSemaforos, IPC_CREAT | 0600);
    if(sem_inicio<0)
    {
        printf("Error al crear semaforos...\n");
        return -1;
    }
    

    semid=semget(IPC_PRIVATE, 13, IPC_CREAT|0600);
    if(semid==-1)
    {
        printf("Error al crear semaforo...\n");
        return -1;
    }
     
    int ctl;
    ctl=semctl(semid,0,SETVAL,1); //semaforo de andar por comedor en general

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

    ctl=semctl(semid,2,SETVAL,1);   //semaforo de mitad de campo a puente

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

    ctl=semctl(semid,4,SETVAL,4);   //detenerse antes de entrar al comedor
    if(ctl==-1)
    {
        perror("Error al asignar el contador del semaforo.\n");
        return 1;
    }

    ctl=semctl(semid,5,SETVAL,4);  //semaforo entrada antesala
    if(ctl==-1)
    {
        perror("Error al asignar el contador del semaforo.\n");
        return 1;
    }

    ctl=semctl(semid,6,SETVAL,1);  //semaforo templo 
    if(ctl==-1)
    {
        perror("Error al asignar el contador del semaforo.\n");
        return 1;
    }

    ctl=semctl(semid,7,SETVAL,1);  //semaforo memoria comparitda puente
    if(ctl==-1)
    {
        perror("Error al asignar el contador del semaforo.\n");
        return 1;
    }

    ctl=semctl(semid,8,SETVAL,0);  //no puede entrar al puente
    if(ctl==-1)
    {
        perror("Error al asignar el contador del semaforo.\n");
        return 1;
    }

    ctl=semctl(semid,9,SETVAL,1);  //memoria compartida de ver si hay alguien esperando puente
    if(ctl==-1)
    {
        perror("Error al asignar el contador del semaforo.\n");
        return 1;
    }

    ctl=semctl(semid,10,SETVAL,1);  //escoger sitio en templo
    if(ctl==-1)
    {
        perror("Error al asignar el contador del semaforo.\n");
        return 1;
    }

    ctl=semctl(semid,11,SETVAL,1);  //andar templo
    if(ctl==-1)
    {
        perror("Error al asignar el contador del semaforo.\n");
        return 1;
    }

    ctl=semctl(semid,12,SETVAL,1);  //andar templo
    if(ctl==-1)
    {
        perror("Error al asignar el contador del semaforo.\n");
        return 1;
    }


    memoria memoriam;
    
    int tamMemComp=FI_getTamaNoMemoriaCompartida();
    


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
                mem->sitios_templo[i]=0;
            }

            mem->infoFil[i].pidFil=getpid();
            mem->infoFil[i].idFil=i;
            err=FI_inicioFilOsofo(i);
            if(err ==-1)
            {
                return -1;
            }
            mem->contador_personas=0;
            mem->sentido_puente=-1;
            mem->espera=0;
            printf("%d, %d", mem->contador_personas, mem->sentido_puente);
            fflush(stdout);

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
        
    }

    int errFI_puedo,errFI_pausa;
    if(getpid()!=pidPadre)
    {
        
        int nVueltas=0;
        int zona=-1;
        int zonaPrevia;
        while(nVueltas<numVuel)
        {
            //moverse con espera ocupada
            errFI_puedo=-1;
            
            while (errFI_puedo != 100) {
                errFI_puedo = FI_puedoAndar();
                if (errFI_puedo == 100) {
                    errFI_pausa = FI_pausaAndar();
                    zonaPrevia = zona;
                    zona = FI_andar();
                    //printf("%d",zona);
                    //    fflush(stdout);
                }
            }
            
            
            

            if(zonaPrevia==CAMPO&&zona==PUENTE) //metiendote en el puente
            {
                int puedo_entrar_puente=0;
                do{
                
                    wait_semaforo(semid, 7);    //memoria

                    memoria *mem = (memoria *)shmat(shm_inicio, NULL, 0);
                    if (mem == (void *)-1)
                    {
                        perror("Error en shmat (hijo)");
                        exit(1);
                    }


                    if(mem->contador_personas<2 && (mem->sentido_puente==-1 || mem->sentido_puente==0)){    //si puente esta vacio o estan en mi sentido y hay 0 o 1
                        puedo_entrar_puente=1;
                        mem->sentido_puente=0;
                        mem->contador_personas+=1;
                        
                        //printf("derecha a izquierda, puedo");
                        fflush(stdout);
                       
                    
               
                    }else{
                        //printf("no puedo pasar...");
                        //fflush(stdout);
                        wait_semaforo(semid, 9);    //solo uno modifica memoria o lee memoria de espera
                        //printf("pongo que estoy esperando");
                        //fflush(stdout);
                        mem->espera=1;  //estoy esperando
                        printf("espero: %d, %d\n", mem->contador_personas, mem->sentido_puente);
                        fflush(stdout);
                        signal_semaforo(semid, 9);
                        shmdt(mem);
                        signal_semaforo(semid, 7); //libero la memoria para que puedan miararla
                        //printf("me bloqueo pq no puedo pasar");
                        //fflush(stdout);
                        wait_semaforo(semid, 8);    //no puedo entrar al puente, me bloqueo
                        continue;
                    }

                    shmdt(mem);
                    signal_semaforo(semid, 7); //libero la memoria para que puedan mirarla
                    
                }while(puedo_entrar_puente!=1);

               
                wait_semaforo(semid,1);//puede entrar al puente, dos personas solo, QUITABLE!!!!!
                //printf("puedo pasar al puente");
                //fflush(stdout);
               
                
            }

            if(zonaPrevia==PUENTE&&zona==CAMPO)
            {
                printf("entro en cambiar");
                fflush(stdout);
                int paso=0;
                do{
                    errFI_puedo=0;
                    while (errFI_puedo != 100) {
                        errFI_puedo = FI_puedoAndar();
                        if (errFI_puedo == 100) {
                            errFI_pausa = FI_pausaAndar();
                            zonaPrevia = zona;
                            zona = FI_andar();
                            //printf("%d",zona);
                            //fflush(stdout);

                            paso+=1;
                        }
                    }
                    //printf("doy dos pasos");
                    //fflush(stdout);
                }while(paso<2);

                wait_semaforo(semid, 9);    //ver memeoria compartida
                memoria *mem = (memoria *)shmat(shm_inicio, NULL, 0);
                if (mem == (void *)-1)
                {
                    perror("Error en shmat (hijo)");
                    exit(1);
                }
                
                mem->contador_personas-=1;
                //printf("una persona menos ");
                //fflush(stdout);
                //printf("%d, %d", mem->contador_personas, mem->sentido_puente);
                //fflush(stdout);
                if(mem->contador_personas==0){
                    printf("sentido: %d, %d", mem->contador_personas, mem->sentido_puente);
                    fflush(stdout);
                    //printf("cambio el sentido");
                    //fflush(stdout);
                    mem->sentido_puente=-1;
                    printf("cambio de sentifo: %d, %d", mem->contador_personas, mem->sentido_puente);
                    fflush(stdout);
                }
                
                if(mem->espera==1){
                    //printf("hay alguien esperando le avisamos");
                    //fflush(stdout);
                    signal_semaforo(semid, 8);  //hay alguien esperando y le avisamos pa q pase
                    mem->espera=0;
                    
                }
                
                
                

                shmdt(mem);

                signal_semaforo(semid, 9);  
                signal_semaforo(semid,1);   //he salido del puente, conteo personas, QUITABLE!!!!!!
                
            }


                
             

            //seccion critica para elegir sitio en comedor
            //seccion critica en comedor y antesala



            //-------------------------------------//

            
            if(zona==ANTESALA&&zonaPrevia==CAMPO)
            {
                wait_semaforo(semid,5);

            }



            if(zona == ENTRADACOMEDOR)
            {   

                memoria *mem = (memoria *)shmat(shm_inicio, NULL, 0);
                if (mem == (void *)-1)
                {
                    perror("Error en shmat (hijo)");
                    exit(1);
                }
                
                //SE TIENE QUE PARAR AQUI ANTES DE LA ELECCION DEL PLATO 
                wait_semaforo(semid,4); 
                
                wait_semaforo(semid,3);
                int plato=-1;
                for(int i=0;i<5;i++)
                {
                   if (mem->platos_libres[i] == 0) {
                        plato = i;
                        mem->platos_libres[i] = getpid();  // Ocuparait_semaforo(semid,4);  el plato
                        break;
                   }   
                    
                }


                //si plato es -1 está esperando en la antesala, no encontró plato
                
                FI_entrarAlComedor(plato);               
                signal_semaforo(semid,3);    
               
                //DA EL PASO DE ENTRAR AL COMEDOR Y AVISA A UNO DE LA ANTESALA QUE ENTRE
                int paso=1;
                while(1)
                {       // 

                    
                    wait_semaforo(semid,0);
                    errFI_puedo=FI_puedoAndar();
                    if(errFI_puedo==100)
                    {
                        errFI_pausa=FI_pausaAndar();
                        zona=FI_andar();
                        zonaPrevia=zona;
                        if(paso==1)
                        {
                            signal_semaforo(semid,5);
                            paso-=1;
                        }
                    }
                    signal_semaforo(semid,0);
                   
                    if(zona == SILLACOMEDOR)
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
                        signal_semaforo(semid,4);


                        /*****************************************/
                        zonaPrevia=zona;
                        do{
                            wait_semaforo(semid,0); 
                            errFI_puedo=FI_puedoAndar();
                            if(errFI_puedo==100)
                            {
                                errFI_pausa=FI_pausaAndar();
                                zona=FI_andar();
                                zonaPrevia=zona;
                            }
                            signal_semaforo(semid,0);
                        }while(zona!=CAMPO);
                        /*****************************************/
                        int total=40;
                        do{
                            errFI_puedo=FI_puedoAndar();
                            if(errFI_puedo==100)
                            {signal_semaforo(semid,1);  
                                errFI_pausa=FI_pausaAndar();
                                zona=FI_andar();
                                zonaPrevia=zona;
                                total-=1;
                            }
                        }while(total>0);

                        /*****************************************/

                        do{
                            wait_semaforo(semid,2);
                            errFI_puedo=FI_puedoAndar();
                            if(errFI_puedo==100)
                            {
                                errFI_pausa=FI_pausaAndar();
                                zona=FI_andar();
                                zonaPrevia=zona;
                            }
                            signal_semaforo(semid,2);
                        }while(zona!=PUENTE);

                        if(zona==PUENTE){
                            int puedo_entrar_puente=0;
                            do{
                            
                                wait_semaforo(semid, 7);    //memoria

                                memoria *mem = (memoria *)shmat(shm_inicio, NULL, 0);
                                if (mem == (void *)-1)
                                {
                                    perror("Error en shmat (hijo)");
                                    exit(1);
                                }


                                if (mem->contador_personas<2 && (mem->sentido_puente==-1 || mem->sentido_puente==1)){//si puente esta vacio o estan en mi sentido y hay 0 o 1
                                    puedo_entrar_puente=1;
                                    mem->sentido_puente=1;
                                    mem->contador_personas+=1;
                                    //printf("izquierda a derecha, puedo");
                                    //fflush(stdout);

                        
                                }else{
                                    //printf("no puedo pasar...");
                                    //fflush(stdout);
                                    wait_semaforo(semid, 9);    //solo uno modifica memoria o lee memoria de espera
                                    //printf("pongo que estoy esperando");
                                    //fflush(stdout);
                                    mem->espera=1;  //estoy esperando
                                    printf("espero: %d, %d\n", mem->contador_personas, mem->sentido_puente);
                                    fflush(stdout);
                                    signal_semaforo(semid, 9);
                                    
                                    signal_semaforo(semid, 7); //libero la memoria para que puedan miararla
                                    //printf("me bloqueo pq no puedo pasar");
                                    //fflush(stdout);
                                    //printf("%d, %d", mem->contador_personas, mem->sentido_puente);
                                    //fflush(stdout);
                                    
                                    shmdt(mem);
                                    wait_semaforo(semid, 8);    //no puedo entrar al puente, me bloqueo
                                    continue;
                                }

                                shmdt(mem);
                                signal_semaforo(semid, 7); //libero la memoria para que puedan mirarla
                                
                            }while(puedo_entrar_puente!=1);

                        
                            wait_semaforo(semid,1);//puede entrar al puente, dos personas solo, QUITABLE!!!!!
                            //printf("puedo pasar al puente");
                            //fflush(stdout);


                            break;
                
                        }
                    }
                    /*****************************************/          


                        
                }
                    
            

            }
            else if(zona==TEMPLO)
            {
               memoria *mem = (memoria *)shmat(shm_inicio, NULL, 0);
                if (mem == (void *)-1)
                {
                    perror("Error en shmat (hijo)");
                    exit(1);
                }

                int elegido=-1;
                do{
                    wait_semaforo(semid,10);
                    
                    for (int i = 0; i < 3; i++)
                    {
                        if(mem->sitios_templo[i]==0){
                            mem->sitios_templo[i]=getpid();
                            elegido=i;
                        
                            break;
                        }
                    }

                    if(elegido==-1){
                        signal_semaforo(semid,10);
                    }else{
                        FI_entrarAlTemplo(elegido);
                        signal_semaforo(semid,10);
                    }
                    
                   
                }while(elegido==-1);

                while(1)
                {
                    
                    wait_semaforo(semid,11);
                    errFI_puedo=FI_puedoAndar();
                    if(errFI_puedo==100)
                    {
                        errFI_pausa=FI_pausaAndar();
                        zona=FI_andar();
                        zonaPrevia=zona;
                    }
                    signal_semaforo(semid,11);

                    if(zona==-1)
                    {
                        return -1;
                    }
                    else if(zona == SITIOTEMPLO){
                        int meditar;
                        do
                        {
                            meditar=FI_meditar();
                        } while (meditar==SITIOTEMPLO);


                        
                        wait_semaforo(semid,6);
                
                        for (int i = 0; i < 3; i++)
                        {
                            if(mem->sitios_templo[i]==getpid()){
                                mem->sitios_templo[i]=0;
                                break;
                            }
                        }
                
                        signal_semaforo(semid,6);

                        nVueltas+=1;
                        int pasos=0;

                        //mirar luego
                        if(nVueltas!=numVuel){

                        
                            do{
                                wait_semaforo(semid,11);
                                errFI_puedo=FI_puedoAndar();
                                if(errFI_puedo==100)
                                {
                                    errFI_pausa=FI_pausaAndar();
                                    zona=FI_andar();
                                    zonaPrevia=zona;
                                    pasos++;
                                }
                                signal_semaforo(semid,11);
                            }while(pasos<=13);
                        
                        }
                       
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
        
        exit(0);
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
