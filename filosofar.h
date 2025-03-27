#define NEGRO    0
#define ROJO     1
#define VERDE    2
#define AMARILLO 3
#define AZUL     4
#define MAGENTA  5
#define CYAN     6
#define BLANCO   7

#define MAXFILOSOFOS 21
#define MAXTENEDORES 21
#define MAXSITIOSTEMPLO 14

#define CAMPO            0
#define PUENTE           1
#define ANTESALA         2
#define ENTRADACOMEDOR   3
#define SILLACOMEDOR     4
#define SALIDACOMEDOR    5
#define TEMPLO           6
#define SITIOTEMPLO      7
#define CEMENTERIO       8

static char const *nombreZonas[]={"CAMPO","PUENTE","ANTESALA","ENTRADACOMEDOR",
                           "SILLACOMEDOR","SALIDACOMEDOR","TEMPLO","SITIOTEMPLO",
                           "CEMENTERIO"};


#define TENEDORDERECHO   0
#define TENEDORIZQUIERDO 1

struct DatosSimulaciOn
 {int maxFilOsofosEnPuente;  // FilOsofos como mAximo en el puente
                             //   Si es 10, sin restricciOn de peso
  int maxUnaDirecciOnPuente; // FilOsofos que pueden pasar en una direcciOn
                             // en el puente como mAximo si hay filOsofos
                             // esperando para pasar en la otra direcciOn
                             //   Si es 0, no existe esta restricciOn
  int sitiosTemplo;          // Sitios para meditar en el templo
  int nTenedores;            // NUmero de tenedores o puestos en el comedor
  };

int FI_getNSemAforos(void);
int FI_getTamaNoMemoriaCompartida(void);
int FI_inicio(int ret, unsigned long long clave,
              struct DatosSimulaciOn *ddssp, int semAforos, int memoCompartida,
              int const *dE);
int FI_inicioFilOsofo(int nFilOsofo);
int FI_pausaAndar(void);
int FI_puedoAndar(void);
int FI_andar(void);
int FI_entrarAlComedor(int puesto);
int FI_cogerTenedor(int t);
int FI_comer(void);
int FI_dejarTenedor(int t);
int FI_entrarAlTemplo(int puesto);
int FI_meditar(void);
int FI_finFilOsofo(void);
int FI_fin(void);

void pon_error(char *mensaje);
