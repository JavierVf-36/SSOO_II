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
    int sitios_templo[3];
    int sentido_puente; //dentro del puente
    int contador_personas;
    int espera;
    int tenedores[5];
    int platos_libres[5];
    infoFils infoFil[21];
    
    
}memoria;

typedef struct mensaje
{
    long tipo;
    int info;
}mensaje;



//union semun {
//    int val;                   /* Valor para SETVAL */
//    struct semid_ds *buf;       /* Buffer para IPC_STAT, IPC_SET */
//    unsigned short *array;      /* Array para GETALL, SETALL */
//};



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
        perror("Error al hacer el signal");
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
    int i;
    for(i=0;i<numFil;i++)
    {
        if(memm->infoFil[i].pidFil==pid)
        {
            return memm->infoFil[i].idFil;
        }
    }
    int err=shmdt(memm);
    if(err==-1)
    {
        printf("Error al eliminar puntero a memoria...\n");
        return -1;
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
    int i;
    for(i=0;i<numFil;i++)
    {   
        if(getpid()==pidPadre){
            wait(NULL);
        }else{
            exit(0);
        }
        
    }

    printf("\nHas pulsado CTRL+C. Eliminando semáforo,memoria compartida y buzones.\n");
    fflush(stdout);
    

    eliminar_sem();
    liberar_mem();
    int err=eliminar_buzon(buzon);
    if(err==-1)
    {
        printf("Error al eliminar buzon...\n");
        return;
    }

    err=FI_fin();
    if(err==-1){
        perror("Error al finalizar el programa...\n");
        return;
    }

   
    exit(0);
}


/*******************/
/*       MAIN      */   
/*******************/


int main (int argc, char *argv[]){
    int err;
    mensaje msg;
   
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
    

    semid=semget(IPC_PRIVATE, 10, IPC_CREAT|0600);
    if(semid==-1)
    {
        printf("Error al crear semaforo...\n");
        return -1;
    }
     
    
    //union semun arg1, arg4;
    //arg.val1=1;
    //arg.val4=4;

    int ctl;
    //ctl=semctl(semid,0,SETVAL,arg1);
    ctl=semctl(semid,0,SETVAL,1); //semaforo de andar por comedor en general
    if(ctl==-1)
    {
        perror("Error al asignar el contador del semaforo.\n");
        return 1;
    }

    //ctl=semctl(semid,1,SETVAL,arg1);
    ctl=semctl(semid,1,SETVAL,1);   //inicio filosofos
    if(ctl==-1)
    {
        perror("Error al asignar el contador del semaforo.\n");
        return 1;
    }
    //ctl=semctl(semid,2,SETVAL,arg1);
    ctl=semctl(semid,2,SETVAL,1);   //semaforo de mitad de campo a puente

    if(ctl==-1)
    {
        perror("Error al asignar el contador del semaforo.\n");
        return 1;
    }
    //ctl=semctl(semid,3,SETVAL,arg1);
    ctl=semctl(semid,3,SETVAL,1);   //semaforo eleccion plato
    if(ctl==-1)
    {
        perror("Error al asignar el contador del semaforo.\n");
        return 1;
    }

    //ctl=semctl(semid,4,SETVAL,arg4);
    ctl=semctl(semid,4,SETVAL,4);   //detenerse antes de entrar al comedor
    if(ctl==-1)
    {
        perror("Error al asignar el contador del semaforo.\n");
        return 1;
    }

    //ctl=semctl(semid,5,SETVAL,arg4);
    ctl=semctl(semid,5,SETVAL,4);  //semaforo entrada antesala
    if(ctl==-1)
    {
        perror("Error al asignar el contador del semaforo.\n");
        return 1;
    }

    //ctl=semctl(semid,6,SETVAL,arg1);
    ctl=semctl(semid,6,SETVAL,1);  //semaforo templo 
    if(ctl==-1)
    {
        perror("Error al asignar el contador del semaforo.\n");
        return 1;
    }

    //ctl=semctl(semid,7,SETVAL,arg1);
    ctl=semctl(semid,7,SETVAL,1);  //semaforo memoria comparitda puente
    if(ctl==-1)
    {
        perror("Error al asignar el contador del semaforo.\n");
        return 1;
    }

    //ctl=semctl(semid,8,SETVAL,arg1);
    ctl=semctl(semid,8,SETVAL,1);  //escoger sitio en templo
    if(ctl==-1)
    {
        perror("Error al asignar el contador del semaforo.\n");
        return 1;
    }

    //ctl=semctl(semid,9,SETVAL,arg1);
    ctl=semctl(semid,9,SETVAL,1);  //andar templo
    if(ctl==-1)
    {
        perror("Error al asignar el contador del semaforo.\n");
        return 1;
    }





    memoria memoriam;
    
    int tamMemComp=FI_getTamaNoMemoriaCompartida();
    

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



    memoria *mem = (memoria *)shmat(shm_inicio, NULL, 0);
    if (mem == (void *)-1)
    {
        perror("Error en shmat");
        exit(1);
    }

    int i;
    for(i=0; i<21; i++){    //filosofos que no existen a -1
        mem->infoFil[i].pidFil=-1;
        mem->infoFil[i].idFil=-1;
    }

    for(i=0;i<5;i++)
    {
        mem->platos_libres[i]=0;
        mem->tenedores[i]=0;
        
    }

    for(i=0;i<3;i++)
    {
    mem->sitios_templo[i]=0;
    }

    mem->contador_personas=0;
    mem->sentido_puente=-1;
    mem->espera=0;

    
    for(i=0;i<numFil;i++){
        pid_t pid=fork();
        
        if(pid==0)
        {
            
    
            mem->infoFil[i].pidFil=getpid();
            mem->infoFil[i].idFil=i;
            err=FI_inicioFilOsofo(i);
            if(err ==-1)
            {
                printf("Error al iniciar filosofos...\n");
                fflush(stdout);
                return -1;
            }
            
            //En memoria compartida se esta guardando correctamente:
            //1.- El pid del filosofo
            //2.- El identificador del filosofo
            //Basta que un filosofo recorra la memoria 
            //buscando su PID para saber que identificador tiene

            err=shmdt(mem);
            if(err ==-1)
            {
                printf("Error al eliminar puntero a memoria...\n");
                fflush(stdout);
                return -1;
            }
            break;
        }
        else if(pid<=-1)
        {
            printf("Error al hacer fork...\n");
            fflush(stdout);
            return -1;
        }
        
    }

    int errFI_puedo,errFI_pausa;
    if(getpid()!=pidPadre)
    {
        
        int nVueltas=0;
        int zona=-1;
        int zonaPrevia=-2;

        int idFil=localizarSignal(getpid(),numFil);
        if(idFil!=numFil-1)
        {
            wait_cero(semid,1);
        }
        else
        {
            wait_semaforo(semid,1);
        }

        while(nVueltas<numVuel)
        {
            //moverse con espera ocupada
            errFI_puedo=-1;
            
            while (errFI_puedo != 100) {
                errFI_puedo = FI_puedoAndar();
                if (errFI_puedo == 100) {
                    errFI_pausa = FI_pausaAndar();
                    if(errFI_pausa==-1){
                        perror("Error pausa andar...\n");
                        return -1;
                    }
                    zonaPrevia = zona;
                    zona = FI_andar();
                    if(zona==-1){
                        perror("Error al andar..\n");
                        return -1;
                    }
                }else if(errFI_puedo==-1){
                    perror("Error puedo andar...\n");
                    return -1;
                }else{
                    usleep(50000);
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
                        err=shmdt(mem);
                        if(err ==-1)
                        {
                            printf("Error al borrar puntero memoria...\n");
                            fflush(stdout);
                            return -1;
                        }
                        signal_semaforo(semid, 7); //libero la memoria para que puedan mirarla            
                    }else{
                        //40: AVISAR A OTRO QUE ESTOY ESPERANDO
                        //41: ESPERAR HASTA QUE TE AVISEN DE LA DERECHA
                        //42: ESPERAR HASTA QUE TE AVISEN DE LA IZQUIERDA
                        mensaje msg;
                        mensaje recibido;
                        msg.info=0;
                        msg.tipo=40;
                        err=msgsnd(buzon,&msg,sizeof(mensaje)-sizeof(long),0);
                        if(err ==-1)
                        {
                            perror("Error al enviar mensaje puente...\n");
                            return -1;
                        }

                        err=shmdt(mem);
                        if(err ==-1)
                        {
                            perror("Error al borrar puntero memoria...\n");
                            return -1;
                        }
                        signal_semaforo(semid, 7); //libero la memoria para que puedan miararla

                        err=msgrcv(buzon,&recibido,sizeof(mensaje)-sizeof(long),41,0);
                        if(err ==-1)
                        {
                            perror("Error al recibir mensaje puente...\n");
                            return -1;
                        }
                        
                        continue;
                    }                    
                }while(puedo_entrar_puente!=1);
            }

            if(zonaPrevia==PUENTE&&zona==CAMPO)
            {

                int paso=0;
                do{
                    errFI_puedo=0;
                    while (errFI_puedo != 100) {
                        errFI_puedo = FI_puedoAndar();
                        if (errFI_puedo == 100) {
                            errFI_pausa = FI_pausaAndar();
                            if(errFI_pausa ==-1)
                            {
                                perror("Error pausa andar...\n");
                                return -1;
                            }
                            zonaPrevia = zona;
                            zona = FI_andar();
                            if(zona==-1){
                                perror("Error al andar...\n");
                                return -1;
                            }
                            paso+=1;
                        }else if(errFI_puedo==-1){
                            perror("Error puedo andar...\n");
                            return -1;
                        }else{
                            usleep(50000);
                        }

                    }
                    
                }while(paso<2);


                wait_semaforo(semid, 7); 

                memoria *mem = (memoria *)shmat(shm_inicio, NULL, 0);
                if (mem == (void *)-1)
                {
                    perror("Error en shmat (hijo)");
                    exit(1);
                }

                mem->contador_personas-=1;
                if(mem->contador_personas==0)
                {
                    mem->sentido_puente=-1;
                    mensaje recibido;
                    int mrecibido=msgrcv(buzon,&recibido,sizeof(mensaje)-sizeof(long),40,IPC_NOWAIT);
                    if (mrecibido != -1) {
                        if(recibido.info==0) {
                            mensaje msg;
                            msg.info=0;
                            msg.tipo=41;
                            msgsnd(buzon,&msg,sizeof(mensaje)-sizeof(long),0);
                        } else if(recibido.info==1) {
                            mensaje msg;
                            msg.info=0;
                            msg.tipo=42;
                            msgsnd(buzon,&msg,sizeof(mensaje)-sizeof(long),0);
                        }
                    }
                }

                err=shmdt(mem);
                if(err==-1){
                    perror("Error al eliminar puntero memoria...\n");
                    return -1;
                }                

                signal_semaforo(semid, 7); //arriba de mensajes

            }
 
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
                wait_semaforo(semid,4); //antesala
                
                wait_semaforo(semid,3); //elegir plato en memoria
                int plato=-1;
                int i;
                for(i=0;i<5;i++)
                {
                   if (mem->platos_libres[i] == 0) {
                        plato = i;
                        mem->platos_libres[i] = getpid();  
                        break;
                   }   
                    
                }
                
                err=FI_entrarAlComedor(plato);
                if(err==-1){
                    perror("Error al entrar al templo...\n");
                    return -1;
                }            
                signal_semaforo(semid,3);    
               
                //DA EL PASO DE ENTRAR AL COMEDOR Y AVISA A UNO DE LA ANTESALA QUE ENTRE
                int paso=1;
                while(1)
                {      

                    wait_semaforo(semid,0);
                    errFI_puedo=FI_puedoAndar();
                    if(errFI_puedo==100)
                    {
                        errFI_pausa=FI_pausaAndar();
                        if(errFI_pausa==-1){
                            perror("Error pausa andar...\n");
                            return -1;
                        }
                        zonaPrevia=zona;
                        zona=FI_andar();
                        if(zona==-1){
                            perror("Error al andar...\n");
                        }
                        

                        if(paso==1)
                        {
                            signal_semaforo(semid,5);   //entren en la antesala
                            paso-=1;
                        }
                    }else if(errFI_puedo==-1){
                        perror("Error al poder andar...\n");
                        return -1;
                    
                    }else{
                        usleep(50000);
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


                            err=FI_cogerTenedor(TENEDORIZQUIERDO);
                            if(err==-1){
                                perror("Error al coger tenedor..\n");
                                return -1;
                            }
                            err=FI_cogerTenedor(TENEDORDERECHO);
                            if(err==-1){
                                perror("Error al coger tenedor..\n");
                                return -1;
                            }
                            
                            int comiendo;
                            do{
                                comiendo=FI_comer();
                                if(comiendo==-1){
                                    perror("Error al comer...\n");
                                    return -1;
                                }
                            }while (comiendo==SILLACOMEDOR);
                            

                            wait_semaforo(semid,3);
                            err=FI_dejarTenedor(TENEDORDERECHO);
                            if(err==-1){
                                perror("Error al dejar tenedor..\n");
                                return -1;
                            }
                            err=FI_dejarTenedor(TENEDORIZQUIERDO);
                            if(err==-1){
                                perror("Error al dejar tenedor..\n");
                                return -1;
                            }
                            mem->tenedores[tenedor_der]=0;
                            mem->tenedores[tenedor_izq]=0;
                            signal_semaforo(semid,3);
                            
                            int paso2=0;
                            do{

                                wait_semaforo(semid,0);
                                errFI_puedo=FI_puedoAndar();
                                if(errFI_puedo==100)
                                {
                                    errFI_pausa=FI_pausaAndar();
                                    if(errFI_pausa==-1){
                                        perror("Error pausa andar...\n");
                                        return -1;
                                    }

                                    zonaPrevia=zona;
                                    zona=FI_andar();
                                    if(zona==-1){
                                        perror("Error al andar...");
                                        return -1;

                                    }
                                    
                                    paso2+=1;
                                    
                                }else if(errFI_puedo==-1){
                                    perror("Error puedo andar...\n");
                                    return -1;
                                }else{
                                    usleep(50000);
                                }
                                signal_semaforo(semid,0);
                            }while(paso2<2);
                            mem->platos_libres[plato]=0;


                        }    
                        
                        
            
                       
                        //eliminar puntero memoria
                        err=shmdt(mem);
                        if(err==-1){
                            perror("Error al eliminar puntero a memoria...\n");
                            return -1;
                        }
                        signal_semaforo(semid,4);


                        /*****************************************/
                        zonaPrevia=zona;
                        do{
                            wait_semaforo(semid,0); 
                            errFI_puedo=FI_puedoAndar();
                            if(errFI_puedo==100)
                            {
                                errFI_pausa=FI_pausaAndar();
                                if(errFI_pausa==-1){
                                    perror("Error puedo andar...\n");
                                    return -1;
                                }
                                zonaPrevia=zona;
                                zona=FI_andar();
                                if(zona==-1){
                                    perror("Error al andar...\n");
                                    return -1;
                                }
                                
                            }else if(errFI_puedo==-1){
                                perror("Error puedo andar...\n");
                                return -1;
                            }else{
                                usleep(50000);
                            }
                            signal_semaforo(semid,0);
                        }while(zona!=CAMPO);
                        /*****************************************/
                        int total=40;
                        do{
                            errFI_puedo=FI_puedoAndar();
                            if(errFI_puedo==100)
                            { 
                                errFI_pausa=FI_pausaAndar();
                                if(errFI_pausa==-1){
                                    perror("Error puedo andar...\n");
                                    return -1;
                                }
                                zonaPrevia=zona;
                                zona=FI_andar();
                                if(zona==-1){
                                    perror("Error al andar...\n");
                                    return -1;
                                }
                                
                                total-=1;
                            }else if(errFI_puedo==-1){
                                perror("Error puedo andar...\n");
                                return -1;
                            }else{
                                usleep(50000);
                            }

                        }while(total>0);

                        /*****************************************/

                        do{
                            wait_semaforo(semid,2);
                            errFI_puedo=FI_puedoAndar();
                            if(errFI_puedo==100)
                            {
                                errFI_pausa=FI_pausaAndar();
                                if(errFI_pausa==-1){
                                    perror("Error puedo andar...\n");
                                    return -1;
                                }
                                zonaPrevia=zona;
                                zona=FI_andar();
                                if(zona==-1){
                                    perror("Error al andar...\n");
                                    return -1;
                                }
                                
                            }else if(errFI_puedo==-1){
                                perror("Error puedo andar...\n");
                                return -1;
                            }else{
                                usleep(50000);
                            }
   
                            signal_semaforo(semid,2);
                        }while(zona!=PUENTE);
                        
                    }
                    
                    if(zona==PUENTE) //metiendote en el puente
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
        
        
                            if(mem->contador_personas<2 && (mem->sentido_puente==-1 || mem->sentido_puente==1)){    //si puente esta vacio o estan en mi sentido y hay 0 o 1
                                puedo_entrar_puente=1;
                                mem->sentido_puente=1;
                                mem->contador_personas+=1;
                                int err=shmdt(mem);
                                if(err==-1){
                                    perror("Error al eliminar puntero memoria...\n");
                                    return -1;
                                }
                                signal_semaforo(semid, 7); //libero la memoria para que puedan mirarla               
                            }else{
                                //40: AVISAR A OTRO QUE ESTOY ESPERANDO
                                //41: ESPERAR HASTA QUE TE AVISEN DE LA DERECHA
                                //42: ESPERAR HASTA QUE TE AVISEN DE LA IZQUIERDA
                                mensaje msg;
                                mensaje recibido;
                                msg.info=1;
                                msg.tipo=40;
                                int err=msgsnd(buzon,&msg,sizeof(mensaje)-sizeof(long),0);
                                if(err==-1){
                                    perror("Error al enviar mensaje...\n");
                                    return -1;
                                }
                                err=shmdt(mem);
                                if(err==-1){
                                    perror("Error al eliminar puntero a memoria...\n");
                                    return -1;
                                }
                                signal_semaforo(semid, 7); //libero la memoria para que puedan miararla
                                err=msgrcv(buzon,&recibido,sizeof(mensaje)-sizeof(long),42,0);
                                if(err==-1){
                                    perror("Error al recibir mensaje...\n");
                                    return -1;
                                }
                                continue;
                            }
                            
                        }while(puedo_entrar_puente!=1);

                        do{
                            errFI_puedo=FI_puedoAndar();
                            if(errFI_puedo==100)
                            {
                                errFI_pausa=FI_pausaAndar();
                                if(errFI_pausa==-1){
                                    perror("Error puedo andar...\n");
                                    return -1;
                                }
                                zonaPrevia=zona;
                                zona=FI_andar();
                                if(zona==-1){
                                    perror("Error al andar...\n");
                                    return -1;
                                }
                                
                            }else if(errFI_puedo==-1){
                                perror("Error puedo andar...\n");
                                return -1;
                            }else{
                                usleep(50000);
                            }
                        
                        }while(zona!=CAMPO);

                    }
        
                    if(zona==CAMPO)
                    {
        
                        /* AL SALIR, DE IZQUIERDA A DERECHA
                        -Actualizar los valores antes de avisar
                        -Cuando quede uno, que haga un wait
                        */
        
        
                        int paso=0;
                        do{
                            errFI_puedo=0;
                            while (errFI_puedo != 100) {
                                errFI_puedo = FI_puedoAndar();
                                if (errFI_puedo == 100) {
                                    errFI_pausa = FI_pausaAndar();
                                    if(errFI_pausa==-1){
                                        perror("Error pausa andar...\n");
                                        return -1;
                                    }
                                    zonaPrevia = zona;
                                    zona = FI_andar();
                                    if(zona==-1){
                                        perror("Error al andar");
                                        return -1;
                                    }
                                    
                                    paso+=1;
                                }else if(errFI_puedo==-1){
                                    perror("Error puedo andar...\n");
                                    return -1;
                                }else{
                                    usleep(50000);
                                }
            
                            }
                        }while(paso<2);
        
                        wait_semaforo(semid, 7); 
        
                        memoria *mem = (memoria *)shmat(shm_inicio, NULL, 0);
                        if (mem == (void *)-1)
                        {
                            perror("Error en shmat (hijo)");
                            exit(1);
                        }
        
                        mem->contador_personas-=1;
                        if(mem->contador_personas==0){
                            mem->sentido_puente=-1;
                            mensaje recibido;
                            int mrecibido=msgrcv(buzon,&recibido,sizeof(mensaje)-sizeof(long),40,IPC_NOWAIT);
                            if (mrecibido != -1) {
                                if(recibido.info==0) {
                                    mensaje msg;
                                    msg.info=0;
                                    msg.tipo=41;
                                    int err=msgsnd(buzon,&msg,sizeof(mensaje)-sizeof(long),0);
                                    if(err==-1){
                                        perror("Error al enviar mensaje..\n");
                                        return -1;
                                    }
                                } else if(recibido.info==1) {
                                    mensaje msg;
                                    msg.info=0;
                                    msg.tipo=42;
                                    int err=msgsnd(buzon,&msg,sizeof(mensaje)-sizeof(long),0);
                                    if(err==-1){
                                        perror("Error al enviar mensaje..\n");
                                        return -1;
                                    }
                                }
                            }
                        }
                        int err=shmdt(mem);
                        if(err==-1){
                            perror("Error al eliminar putero a memoria..\n");
                            return -1;
                        }
                        signal_semaforo(semid, 7);    
                        break;
                    }
                        
                }//fin de while(1)
                    
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
                    
                    wait_semaforo(semid,8);
                    int i;
                    for (i = 0; i < 3; i++)
                    {
                        if(mem->sitios_templo[i]==0){
                            mem->sitios_templo[i]=getpid();
                            elegido=i;
                            break;
                        }
                    }

                    if(elegido==-1){
                        signal_semaforo(semid,8);
                    }else{
                        int err=FI_entrarAlTemplo(elegido);
                        if(err==-1){
                            perror("Error al entrar al templo...\n");
                            return -1;
                        }
                        signal_semaforo(semid,8);

                        //avisar en la cola del elegido (la que sea)
                        //esperar en la cola 53 para que te despierten
                    }    
                }while(elegido==-1);

                while(1)
                {
                    wait_semaforo(semid,9);
                    errFI_puedo=FI_puedoAndar();
                    if(errFI_puedo==100)
                    {
                        errFI_pausa=FI_pausaAndar();
                        if(errFI_pausa==-1){
                            perror("Error pausa andar...\n");
                            return -1;
                        }
                        zonaPrevia=zona;
                        zona=FI_andar();
                        if(zona==-1){
                            perror("Error al andar...\n");
                            return -1;
                        }
                        
                    }else if(errFI_puedo==-1){
                        perror("Error puedo andar...\n");
                        return -1;
                    
                    }else{
                        usleep(50000);
                    }
    
                    signal_semaforo(semid,9);

                    if(zona==-1)
                    {
                        perror("Error al andar...\n");
                        return -1;
                    }
                    else if(zona == SITIOTEMPLO){
                        int meditar;
                        do
                        {
                            meditar=FI_meditar();
                            if(meditar==-1){
                                perror("Error al meditar...\n");
                                return -1;
                            }
                        } while (meditar==SITIOTEMPLO);

                        nVueltas+=1;
                        int pasos=0;

                        if(nVueltas!=numVuel){
                            if(elegido==0)
                            {
                                do{   
                                    wait_semaforo(semid,9);
                                    errFI_puedo=FI_puedoAndar();
                                    if(errFI_puedo==100)
                                    {
                                        errFI_pausa=FI_pausaAndar();
                                        if(errFI_puedo==-1){
                                            perror("Error puedo andar...\n");
                                            return -1;
                                        }
                                        zonaPrevia=zona;
                                        zona=FI_andar();
                                        if(zona==-1){
                                            perror("Error al andar...\n");
                                            return -1;
                                        }
                                        
                                        pasos++;
                                        
                                    }else if(errFI_puedo==-1){
                                        perror("Error puedo andar...\n");
                                        return -1;
                                    }else{
                                        usleep(50000);
                                    }
                        
                                
                                    signal_semaforo(semid,9);
                                }while(pasos<=19);
                            }
                            else if (elegido==1)
                            {
                                do{   
                                wait_semaforo(semid,9);
                                errFI_puedo=FI_puedoAndar();
                                if(errFI_puedo==100)
                                {
                                    errFI_pausa=FI_pausaAndar();
                                    if(errFI_puedo==-1){
                                        perror("Error puedo andar...\n");
                                        return -1;
                                    }
                                    zonaPrevia=zona;
                                    zona=FI_andar();
                                    if(zona==-1){
                                        perror("Error al andar...\n");
                                        return -1;
                                    }
                                    
                                    pasos++;
                                    
                                }else if(errFI_puedo==-1){
                                    perror("Error puedo andar...\n");
                                    return -1;
                                }else{
                                    usleep(50000);
                                }
    
                                signal_semaforo(semid,9);
                                }while(pasos<=14);
                            }
                            else if (elegido==2)
                            {
                               do{   
                                wait_semaforo(semid,9);
                                errFI_puedo=FI_puedoAndar();
                                if(errFI_puedo==100)
                                {
                                    errFI_pausa=FI_pausaAndar();
                                    if(errFI_puedo==-1){
                                        perror("Error puedo andar...\n");
                                        return -1;
                                    }
                                    zonaPrevia=zona;
                                    zona=FI_andar();
                                    if(zona==-1){
                                        perror("Error al andar...\n");
                                        return -1;
                                    }
                                    
                                    pasos++;
                                    
                                }else if(errFI_puedo==-1){
                                    perror("Error puedo andar...\n");
                                    return -1;
                                }else{
                                    usleep(50000);
                                }

                               
                                signal_semaforo(semid,9);
                                }while(pasos<=8); 
                            }
                            
                        }

                        wait_semaforo(semid,6);
                        int i;
                        for (i = 0; i < 3; i++)
                        {
                            if(mem->sitios_templo[i]==getpid()){
                                mem->sitios_templo[i]=0;
                                break;
                            }
                        }
                        
                        int err=shmdt(mem);
                        if(err==-1){
                            perror("Error al eliminar puntero memoria...\n");
                            return -1;
                        }
                        signal_semaforo(semid,6);
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
        int i;
        for(i=0;i<numFil;i++)
        {   
        wait(NULL);
        }
    }


    //ELIMINO IPCs
    eliminar_sem(sem_inicio);
    liberar_mem(shm_inicio);
    err=eliminar_buzon(buzon);
    if(err==-1){
        perror("Error al eliminar buzon...\n");
        return -1;
    }


    //FIN
    err=FI_fin();
    if(err==-1)
    {
        perror("Error al hacer FI_fin...\n");
        return -1;
    }



   return 0;
}

