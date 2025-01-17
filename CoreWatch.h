#ifndef COREWATCH_H_
#define COREWATCH_H_
 
// INCLUDES
// Propios:
#include "systemConfig.h"     // Sistema: includes, entrenadora (GPIOs, MUTEXes y entorno), setup de perifericos y otros otros.
#include "reloj.h"
#include "teclado_TL04.h"
 
// DEFINES Y ENUMS
enum FSM_ESTADOS_SISTEMA {START, STAND_BY, SET_TIME, SET_FECHA};
enum FSM_DETECCION_COMANDOS {WAIT_COMMAND};
 
 
// FLAGS FSM DEL SISTEMA CORE WATCH
#define FLAG_SETUP_DONE             0x10
#define FLAG_RESET                  0x20
#define FLAG_SET_CANCEL_NEW_TIME    0x40
#define FLAG_NEW_TIME_IS_READY      0x80
#define FLAG_DIGITO_PULSADO         0x100
//#define FLAG_TIME_ACTUALIZADO     0x02
//#define FLAG_TECLA_PULSADA        0x200
 
#define FLAG_CRONOMETRO             0x1000      // Implementamos el flag del cronómetro
#define FLAG_CALENDARIO             0x2000      // Flag de calendario
 
// DECLARACIÓN ESTRUCTURAS
typedef struct{
    TipoReloj reloj ;
    TipoTeclado teclado ;
    int lcdId ;
    int tempTime ;
    int tempFecha ;     // Almacena la fecha actual
    int digitosGuardados ;
    int digitoPulsado ;
    int cronometro;     // Implementamos el int cronometro.
}TipoCoreWatch;
 
 
// DECLARACIÓN VARIABLES
extern TipoCoreWatch g_coreWatch;
//extern static int g_flagsCoreWatch;
extern fsm_trans_t fsmTransCoreWatch[];
extern fsm_trans_t fsmTransDeteccionComandos[];
 
// DEFINICIÓN VARIABLES
#define TECLA_RESET 'F'
#define TECLA_SET_CANCEL_TIME 'E'
#define TECLA_EXIT 'B'
#define TECLA_CRONOMETRO 'C'        // Definimos la tecla del cronometro como la letra C
#define TECLA_CALENDARIO 'D'        // Tecla de calendario
#define ESPERA_MENSAJE_MS 1000
 
//------------------------------------------------------
// FUNCIONES DE INICIALIZACION DE LAS VARIABLES
//------------------------------------------------------
int ConfiguraInicializaReloj (TipoReloj *p_reloj);
void ResetReloj(TipoReloj *p_reloj);
 
//------------------------------------------------------
// FUNCIONES PROPIAS
//------------------------------------------------------
//void DelayUntil(unsigned int next);
void ActualizaFecha (TipoCalendario * p_fecha);
int ConfiguraInicializaSistema ( TipoCoreWatch * p_sistema ) ;
void DelayUntil ( unsigned int next ) ;
int EsNumero ( char value ) ;
 
//------------------------------------------------------
// FUNCIONES DE ENTRADA O DE TRANSICION DE LA MAQUINA DE ESTADOS
//------------------------------------------------------
int CompruebaDigitoPulsado ( fsm_t * p_this ) ;
int CompruebaNewTimeIsReady ( fsm_t * p_this ) ;
int CompruebaReset ( fsm_t * p_this ) ;
int CompruebaSetCancelNewTime ( fsm_t * p_this ) ;
int CompruebaSetupDone ( fsm_t * p_this ) ;
int CompruebaTeclaPulsada ( fsm_t * p_this ) ;
int CompruebaTimeActualizado ( fsm_t * p_this ) ;
int CompruebaCronometro ( fsm_t * p_this ) ;
int CompruebaSetCalendario ( fsm_t * p_this ) ;         //Comprueba el calendario

//------------------------------------------------------
// FUNCIONES DE SALIDA O DE ACCION DE LA MAQUINA DE ESTADOS
//------------------------------------------------------
void CancelSetNewTime ( fsm_t * p_this ) ;
void CancelSetCalendario ( fsm_t * p_this ) ;
void PrepareSetNewTime ( fsm_t * p_this ) ;
void PrepareSetCalendario ( fsm_t * p_this ) ;      // Prepara el calendario
void ProcesaDigitoTime ( fsm_t * p_this ) ;
void ProcesaTeclaPulsada ( fsm_t * p_this ) ;
void ProcesaDigitoFecha ( fsm_t * p_this ) ;        //Procesa la fecha
void Reset ( fsm_t * p_this ) ;
void SetNewTime ( fsm_t * p_this ) ;
void SetNewFecha ( fsm_t * p_this ) ;       // Pone la nueva fecha
void ShowTime ( fsm_t * p_this ) ;
void ShowCronometro ( fsm_t* p_this ) ;     // Muestra el cronómetro
void Start ( fsm_t * p_this ) ;
//------------------------------------------------------
// FUNCIONES LIGADAS A THREADS ADICIONALES
//------------------------------------------------------

#if VERSION ==2
PI_THREAD ( ThreadExploraTecladoPC ) ;
#endif
#endif /* EAGENDA_H */
