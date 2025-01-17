#include "teclado_TL04.h"

const char tecladoTL04[NUM_FILAS_TECLADO][NUM_COLUMNAS_TECLADO] = {
		{'1', '2', '3', 'C'},
		{'4', '5', '6', 'D'},
		{'7', '8', '9', 'E'},
		{'A', '0', 'B', 'F'}
};

// Maquina de estados: lista de transiciones
// {EstadoOrigen, CondicionDeDisparo, EstadoFinal, AccionesSiTransicion }
fsm_trans_t g_fsmTransExcitacionColumnas[] = {
		{ TECLADO_ESPERA_COLUMNA, CompruebaTimeoutColumna, TECLADO_ESPERA_COLUMNA, TecladoExcitaColumna },
		{-1, NULL, -1, NULL },
};

static TipoTecladoShared g_tecladoSharedVars;

//------------------------------------------------------
// FUCNIONES DE INICIALIZACION DE LAS VARIABLES ESPECIFICAS
//------------------------------------------------------
void ConfiguraInicializaTeclado(TipoTeclado *p_teclado) {
	piLock(KEYBOARD_KEY);
	g_tecladoSharedVars.flags=0;
	g_tecladoSharedVars.debounceTime[0]=0;
	g_tecladoSharedVars.debounceTime[1]=0;
	g_tecladoSharedVars.debounceTime[2]=0;
	g_tecladoSharedVars.debounceTime[3]=0;

	g_tecladoSharedVars.columnaActual = 0;

	g_tecladoSharedVars.teclaDetectada=0;
	piUnlock(KEYBOARD_KEY);

	// Inicializacion del HW:
	// 1. Configura GPIOs de las columnas:
	// 	  (i) Configura los pines y (ii) da valores a la salida
	// 2. Configura GPIOs de las filas:
	// 	  (i) Configura los pines y (ii) asigna ISRs (y su polaridad)

	int i=0;
	void *tecladofilaisr [] = {teclado_fila_1_isr, teclado_fila_2_isr, teclado_fila_3_isr, teclado_fila_4_isr};

	for(i=0;i<4;i++){
		pinMode(p_teclado->columnas[i], OUTPUT);
		digitalWrite(p_teclado->columnas[i],LOW);
		pinMode(p_teclado->filas[i], INPUT);
		pullUpDnControl(p_teclado->filas[i],PUD_DOWN);
		wiringPiISR(p_teclado->filas[i], INT_EDGE_RISING, (tecladofilaisr[i]));
	}

	// Inicializacion del temporizador:
	// 3. Crear y asignar temporizador de excitacion de columnas
	// 4. Lanzar temporizador
	p_teclado->tmr_duracion_columna = tmr_new(timer_duracion_columna_isr);
	tmr_startms(p_teclado->tmr_duracion_columna, TIMEOUT_COLUMNA_TECLADO_MS);
}

//------------------------------------------------------
// FUNCIONES PROPIAS
//------------------------------------------------------
/* Getter y setters de variables globales */
TipoTecladoShared GetTecladoSharedVar() {
	TipoTecladoShared result;
	piLock(KEYBOARD_KEY);
	result = g_tecladoSharedVars;
	piUnlock(KEYBOARD_KEY);
	return result;
}
void SetTecladoSharedVar(TipoTecladoShared value) {
	piLock(KEYBOARD_KEY);
	g_tecladoSharedVars=value;
	piUnlock(KEYBOARD_KEY);

}

void ActualizaExcitacionTecladoGPIO(int columna) {
	// ATENCION: Evitar que este mas de una columna activa a la vez.
	switch(columna){
		case 0:
		{
			digitalWrite (GPIO_KEYBOARD_COL_4, LOW);
			digitalWrite (GPIO_KEYBOARD_COL_1, HIGH);
			break;
		}
		case 1:
		{
			digitalWrite (GPIO_KEYBOARD_COL_1, LOW);
			digitalWrite (GPIO_KEYBOARD_COL_2, HIGH);
			break;
		}
		case 2:
		{
			digitalWrite (GPIO_KEYBOARD_COL_2, LOW);
			digitalWrite (GPIO_KEYBOARD_COL_3, HIGH);
			break;
		}	
		case 3:
		{
			digitalWrite (GPIO_KEYBOARD_COL_3, LOW);
			digitalWrite (GPIO_KEYBOARD_COL_4, HIGH);
			break;
		}
		default:
		{
			break;
		}
	}
}

//------------------------------------------------------
// FUNCIONES DE ENTRADA O DE TRANSICION DE LA MAQUINA DE ESTADOS
//------------------------------------------------------
int CompruebaTimeoutColumna(fsm_t* p_this) {
	int act;
	piLock(KEYBOARD_KEY);
	act = g_tecladoSharedVars.flags & FLAG_TIMEOUT_COLUMNA_TECLADO;
	piUnlock(KEYBOARD_KEY);
	return act;
}


//------------------------------------------------------
// FUNCIONES DE SALIDA O DE ACCION DE LAS MAQUINAS DE ESTADOS
//------------------------------------------------------
void TecladoExcitaColumna(fsm_t* p_this) {
	TipoTeclado *p_teclado = (TipoTeclado*)(p_this->user_data);

	// RESET FLAG
	piLock(KEYBOARD_KEY);
	g_tecladoSharedVars.flags &= ~FLAG_TIMEOUT_COLUMNA_TECLADO;
	piUnlock(KEYBOARD_KEY);

	piLock(KEYBOARD_KEY);
	// 1. Actualizo que columna SE VA a excitar
	// 2. Ha pasado el timer y es hora de excitar la siguiente columna:
		// Excitamos una columna
	g_tecladoSharedVars.columnaActual += 1;
	if(g_tecladoSharedVars.columnaActual > NUM_COLUMNAS_TECLADO)
		g_tecladoSharedVars.columnaActual = 0;

	//  (i) Llamada a ActualizaExcitacionTecladoGPIO con columna A ACTIVAR como argumento
	ActualizaExcitacionTecladoGPIO(g_tecladoSharedVars.columnaActual);

	piUnlock(KEYBOARD_KEY);

	// 4. Manejar el temporizador para que vuelva a avisarnos
	tmr_startms(p_teclado->tmr_duracion_columna, TIMEOUT_COLUMNA_TECLADO_MS);
}

//------------------------------------------------------
// SUBRUTINAS DE ATENCION A LAS INTERRUPCIONES
//------------------------------------------------------
void teclado_fila_1_isr(void) {
	// 1. Comprobar si ha pasado el tiempo de guarda de anti-rebotes
	int now=millis();
	if(now < g_tecladoSharedVars.debounceTime[FILA_1]){
		return;
	}
	// 2. Atender a la interrupcion:
	// 	  (i) Guardar la tecla detectada en g_tecladoSharedVars
	g_tecladoSharedVars.teclaDetectada = tecladoTL04[FILA_1][g_tecladoSharedVars.columnaActual];
	//    (ii) Activar flag para avisar de que hay una tecla pulsada
	piLock(KEYBOARD_KEY);
	g_tecladoSharedVars.flags |= FLAG_TECLA_PULSADA;
	piUnlock(KEYBOARD_KEY);
	// 3. Actualizar el tiempo de guarda del anti-rebotes
	g_tecladoSharedVars.debounceTime[FILA_1]=now+DEBOUNCE_TIME_MS;
}

void teclado_fila_2_isr(void) {
	// 1. Comprobar si ha pasado el tiempo de guarda de anti-rebotes
	int now=millis();
	if(now < g_tecladoSharedVars.debounceTime[FILA_2]){
		return;
	}
	// 2. Atender a la interrupcion:
	// 	  (i) Guardar la tecla detectada en g_tecladoSharedVars
	g_tecladoSharedVars.teclaDetectada = tecladoTL04[FILA_2][g_tecladoSharedVars.columnaActual];
	//    (ii) Activar flag para avisar de que hay una tecla pulsada
	piLock(KEYBOARD_KEY);
	g_tecladoSharedVars.flags |= FLAG_TECLA_PULSADA;
	piUnlock(KEYBOARD_KEY);
	// 3. Actualizar el tiempo de guarda del anti-rebotes
	g_tecladoSharedVars.debounceTime[FILA_2]=now+DEBOUNCE_TIME_MS;
}

void teclado_fila_3_isr(void) {
	// 1. Comprobar si ha pasado el tiempo de guarda de anti-rebotes
	int now=millis();
	if(now < g_tecladoSharedVars.debounceTime[FILA_3]){
		return;
	}
	// 2. Atender a la interrupcion:
	// 	  (i) Guardar la tecla detectada en g_tecladoSharedVars
	g_tecladoSharedVars.teclaDetectada = tecladoTL04[FILA_3][g_tecladoSharedVars.columnaActual];
	//    (ii) Activar flag para avisar de que hay una tecla pulsada
	piLock(KEYBOARD_KEY);
	g_tecladoSharedVars.flags |= FLAG_TECLA_PULSADA;
	piUnlock(KEYBOARD_KEY);
	// 3. Actualizar el tiempo de guarda del anti-rebotes
	g_tecladoSharedVars.debounceTime[FILA_3]=now+DEBOUNCE_TIME_MS;
}

void teclado_fila_4_isr (void) {
	// 1. Comprobar si ha pasado el tiempo de guarda de anti-rebotes
	int now=millis();
	if(now < g_tecladoSharedVars.debounceTime[FILA_4]){
		return;
	}
	// 2. Atender a la interrupcion:
	// 	  (i) Guardar la tecla detectada en g_tecladoSharedVars
	g_tecladoSharedVars.teclaDetectada = tecladoTL04[FILA_4][g_tecladoSharedVars.columnaActual];
	//    (ii) Activar flag para avisar de que hay una tecla pulsada
	piLock(KEYBOARD_KEY);
	g_tecladoSharedVars.flags |= FLAG_TECLA_PULSADA;
	piUnlock(KEYBOARD_KEY);
	// 3. Actualizar el tiempo de guarda del anti-rebotes
	g_tecladoSharedVars.debounceTime[FILA_4]=now+DEBOUNCE_TIME_MS;
}

void timer_duracion_columna_isr(union sigval value) {
	// Simplemente avisa que ha pasado el tiempo para excitar la siguiente columna
	piLock(KEYBOARD_KEY);
	g_tecladoSharedVars.flags |= FLAG_TIMEOUT_COLUMNA_TECLADO;
	piUnlock(KEYBOARD_KEY);

}
