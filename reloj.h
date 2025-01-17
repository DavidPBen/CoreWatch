
#ifndef RELOJ_H_
#define RELOJ_H_
 
 
// INCLUDES
#include "systemConfig.h"
#include "util.h"
 
 
// DEFINES Y ENUMS
#define FLAG_ACTUALIZA_RELOJ     0X01
#define FLAG_TIME_ACTUALIZADO    0X02
#define FLAG_TECLA_ON            0X04
#define FLAG_DEFFINE_OFF         0X08
 
#define MIN_DAY 01
#define MAX_MONTH 12
#define MIN_MONTH 01
#define MIN_YEAR 1970
 
#define MIN_HOUR 00
#define MAX_HOUR 23
#define DEFAULT_TIME_FORMAT 24
#define DEFAULT_MONTH 02
#define DEFAULT_YEAR 2004
#define DEFAULT_DAY 28
#define DEFAULT_HOUR 23
#define DEFAULT_MIN 59
#define DEFAULT_SEC 55
#define TIME_FORMAT_24_H 24
#define TIME_FORMAT_12_H 12
#define MAX_MIN 59
 
#define MAX_YEAR 2999
#define MIN_YEAR 0000
 
#define PRECISION_RELOJ_MS 1000
 
enum FSM_ESTADOS_RELOJ {WAIT_TIC};
 
// DECLARACIÓN DE ESTRUCTURAS
typedef struct {
    int dd;
    int MM;
    int yyyy;
} TipoCalendario;
 
typedef struct {
    int hh;
    int mm;
    int ss;
    int formato;
} TipoHora;
 
typedef struct {
    int timestamp;
    TipoHora hora;
    TipoCalendario calendario;
    tmr_t*tmrTic;
    int cronometro;
} TipoReloj;
 
typedef struct {
    int flags;
} TipoRelojShared;
 
 
// DECLARACIÓN DE VARIABLES
extern fsm_trans_t g_fsmTransReloj[];
extern const int DIAS_MESES[2][MAX_MONTH];
 
 
 
// FUNCIONES DE INICIALIZACION DE LAS VARIABLES
 
int ConfiguraInicializaReloj (TipoReloj *p_reloj);
void ResetReloj(TipoReloj *p_reloj);
 
// FUNCIONES PROPIAS
 
//void DelayUntil(unsigned int next);
void ActualizaFecha (TipoCalendario * p_fecha);
void ActualizaHora(TipoHora *p_hora);
int CalculaDiasMes(int month,int year);
int EsBisiesto(int year);
TipoRelojShared GetRelojSharedVar();
int SetFecha(int nuevaFecha, TipoCalendario *p_fecha);
int SetFormato(int nuevoFormato, TipoHora *p_hora);
int setHora(int horaInt,TipoHora *p_hora);
int setCalendario(int fechaInt,TipoCalendario *p_calendario);***--
void SetRelojSharedVar(TipoRelojShared value);
 
// FUNCIONES DE ENTRADA O DE TRANSICION DE LA MAQUINA DE ESTADOS
 
int CompruebaTic(fsm_t *p_this);
 
// FUNCIONES DE SALIDA O DE ACCION DE LA MAQUINA DE ESTADOS
void ActualizaReloj(fsm_t *p_this);
 
// SUBRUTINAS DE ATENCION A LAS INTERRUPCIONES
void tmr_actualiza_reloj_isr(union sigval value);
 
 
#endif /* RELOJ_H_ */
