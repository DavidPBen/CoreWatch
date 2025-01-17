#include "coreWatch.h"
#include <ctype.h>

TipoCoreWatch g_coreWatch;
static int g_flagsCoreWatch;
fsm_trans_t fsmTransCoreWatch[]={
		{START,CompruebaSetupDone,STAND_BY,Start},
		{STAND_BY,CompruebaTimeActualizado,STAND_BY,ShowTime},
		{STAND_BY,CompruebaCronometro,STAND_BY,ShowCronometro},		// Añadimos la nueva transición
		{STAND_BY,CompruebaReset,STAND_BY,Reset},
		{STAND_BY,CompruebaSetCancelNewTime,SET_TIME,PrepareSetNewTime},
		{SET_TIME,CompruebaSetCancelNewTime,STAND_BY,CancelSetNewTime},
		{SET_TIME,CompruebaDigitoPulsado,SET_TIME,ProcesaDigitoTime},
		{SET_TIME,CompruebaNewTimeIsReady,STAND_BY,SetNewTime},
		{STAND_BY,CompruebaSetCalendario,SET_FECHA,PrepareSetCalendario},		//Prepara el calendario
		{SET_FECHA,CompruebaSetCalendario,STAND_BY,CancelSetCalendario},		//Cancela el calendario
		{SET_FECHA,CompruebaDigitoPulsado,SET_FECHA,ProcesaDigitoFecha},		//Tras pulsar un nuevo dígito se añade en la nueva fecha
		{SET_FECHA,CompruebaNewTimeIsReady,STAND_BY,SetNewFecha},				// Añade la nueva fecha
		{ -1 , NULL , -1 , NULL }
};
fsm_trans_t fsmTransDeteccionComandos[]={
		{WAIT_COMMAND,CompruebaTeclaPulsada,WAIT_COMMAND,ProcesaTeclaPulsada},
		{ -1 , NULL , -1 , NULL }
};	

int ConfiguraInicializaSistema(TipoCoreWatch * p_sistema) {
	#if VERSION >= 2
	piLock(SYSTEM_KEY);
	g_flagsCoreWatch = 0;
	piUnlock(SYSTEM_KEY);

	p_sistema->tempTime = 0;		//Inicializa la fecha a 0
	p_sistema->tempFecha = 0;
	p_sistema->digitosGuardados = 0;
	p_sistema->cronometro = 0;		// Se reinicia el valor de cronómetro cambiando su valor a 0

	int res = ConfiguraInicializaReloj(&(p_sistema->reloj));
		if (res != 0) {
			return 1;
		}
	#endif

	#if VERSION == 2
	int thread = 0;
	if(res==0) {
		thread = piThreadCreate(ThreadExploraTecladoPC);
		if (thread != 0) {
			piLock(STD_IO_LCD_BUFFER_KEY);
			printf("[ERROR!!!][ThreadExploraTecladoPC]\n");
			fflush(stdout);
			piUnlock(STD_IO_LCD_BUFFER_KEY);
			return 1;
		}
	}
	#endif



	#if VERSION >= 3

	int arrayColumnas[4]={GPIO_KEYBOARD_COL_1, GPIO_KEYBOARD_COL_2, GPIO_KEYBOARD_COL_3, GPIO_KEYBOARD_COL_4};
	int arrayFilas[4]={GPIO_KEYBOARD_ROW_1, GPIO_KEYBOARD_ROW_2, GPIO_KEYBOARD_ROW_3, GPIO_KEYBOARD_ROW_4};

	memcpy(p_sistema->teclado.filas, arrayFilas, sizeof(arrayFilas));
	memcpy(p_sistema->teclado.columnas, arrayColumnas, sizeof(arrayColumnas));

	int wiring = wiringPiSetupGpio();
	if (wiring!=0) {
		return 1;
	}
	ConfiguraInicializaTeclado(&(p_sistema->teclado));
	#endif

	#if VERSION >= 4
		int rows = 2;
		int cols = 12;
		int bits = 8;
		int rs = GPIO_LCD_RS;
		int strb = GPIO_LCD_EN;
		int d0 = GPIO_LCD_D0;
		int d1 = GPIO_LCD_D1;
		int d2 = GPIO_LCD_D2;
		int d3 = GPIO_LCD_D3;
		int d4 = GPIO_LCD_D4;
		int d5 = GPIO_LCD_D5;
		int d6 = GPIO_LCD_D6;
		int d7 = GPIO_LCD_D7;

		p_sistema->lcdId = lcdInit(rows,cols,bits,rs,strb,d0,d1,d2,d3,d4,d5,d6,d7);	

		if (p_sistema->lcdId == -1) {
			return 1;
		}


	#endif

	piLock(SYSTEM_KEY);
	g_flagsCoreWatch |= FLAG_SETUP_DONE;
	piUnlock(SYSTEM_KEY);

	piLock(STD_IO_LCD_BUFFER_KEY);
	printf("Son las: %02d:%02d:%02d del %02d/%02d/%04d\n",								
			p_sistema->reloj.hora.hh, p_sistema->reloj.hora.mm,
			p_sistema->reloj.hora.ss, p_sistema->reloj.calendario.dd,
			p_sistema->reloj.calendario.MM, p_sistema->reloj.calendario.yyyy);
	fflush(stdout);
	piUnlock(STD_IO_LCD_BUFFER_KEY);

	return 0;
}

PI_THREAD ( ThreadExploraTecladoPC ) {
	int teclaPulsada ;	
	while (1) {
		delay (10) ; // WiringPi function : pauses program
			//execution for at least 10 ms
		piLock(STD_IO_LCD_BUFFER_KEY);
		if( kbhit () != 0 ) {
			teclaPulsada = kbread() ;
			piUnlock(STD_IO_LCD_BUFFER_KEY);

			teclaPulsada = toupper(teclaPulsada);

			if((char)teclaPulsada == TECLA_RESET){
				piLock(SYSTEM_KEY);
				g_flagsCoreWatch |= FLAG_RESET;
				piUnlock(SYSTEM_KEY);
			}
			else if((char)teclaPulsada == TECLA_SET_CANCEL_TIME){
				piLock(SYSTEM_KEY);
				g_flagsCoreWatch |= FLAG_SET_CANCEL_NEW_TIME;
				piUnlock(SYSTEM_KEY);
			}
			else if(EsNumero(teclaPulsada)){
				piLock(SYSTEM_KEY);
				g_coreWatch.digitoPulsado = teclaPulsada -48;
				g_flagsCoreWatch |= FLAG_DIGITO_PULSADO;
				piUnlock(SYSTEM_KEY);
			}
			else if((char)teclaPulsada == TECLA_CRONOMETRO){		// Comprobamos la tecla del cronómetro
				piLock(SYSTEM_KEY);
				g_flagsCoreWatch |= FLAG_CRONOMETRO;		// Activamos el flag del cronómetro
				piUnlock(SYSTEM_KEY);
			}
			else if((char)teclaPulsada == TECLA_EXIT){
				piLock(STD_IO_LCD_BUFFER_KEY);
				printf("Se va a salir del sistema \n");
				fflush(stdout);
				piUnlock(STD_IO_LCD_BUFFER_KEY);
				exit(0);
			}
			else if((char)teclaPulsada == TECLA_CALENDARIO){		// Comprobamos la tecla del calendario
				piLock(SYSTEM_KEY);
				g_flagsCoreWatch |= FLAG_CALENDARIO;		// Activamos el flag del calendario
				piUnlock(SYSTEM_KEY);
			}
			else if((teclaPulsada != '\n')&&(teclaPulsada != '\r') && (teclaPulsada != 0xA)) {
				piLock(STD_IO_LCD_BUFFER_KEY);
				printf("Tecla desconocida \n");
				fflush(stdout);
				piUnlock(STD_IO_LCD_BUFFER_KEY);
			}
		}
		piUnlock(STD_IO_LCD_BUFFER_KEY);
	}
}

int EsNumero (char value) {
	int val = (int)(value);
	if (val >= 48 && val <= 57){
		return 1;
	}
	return 0;
}

void Start ( fsm_t * p_this ) {
	piLock(SYSTEM_KEY);
		g_flagsCoreWatch &= ~FLAG_SETUP_DONE;
	piUnlock(SYSTEM_KEY);
}

void ShowTime ( fsm_t* p_this ) {
	TipoCoreWatch *p_sistema=(TipoCoreWatch *)p_this->user_data;
	
	TipoRelojShared relsh=GetRelojSharedVar();
	relsh.flags &= ~FLAG_TIME_ACTUALIZADO;
	SetRelojSharedVar(relsh);

	p_sistema->cronometro = p_sistema->cronometro+1;		// Se incrementa 1 el valor del cronómetro
	piLock(STD_IO_LCD_BUFFER_KEY);
		printf("Tiempo transcurrido: %d seg.\n",p_sistema->cronometro);		// Muestra por consola el valor del cronómetro
		fflush(stdout);
	piUnlock(STD_IO_LCD_BUFFER_KEY);

	#if VERSION < 4
	TipoReloj p_miReloj=p_sistema->reloj;
	piLock(STD_IO_LCD_BUFFER_KEY);
		printf("Son las: %02d:%02d:%02d del %02d/%02d/%04d\n",				//se mostrará por pantalla los valores de p_miReloj con 2 dígitos y 4 dígitos para el año
			p_miReloj.hora.hh,
			p_miReloj.hora.mm,
			p_miReloj.hora.ss,
			p_miReloj.calendario.dd,
			p_miReloj.calendario.MM,
			p_miReloj.calendario.yyyy);
		fflush(stdout); 
	piUnlock(STD_IO_LCD_BUFFER_KEY);   
	#endif

	#if VERSION >= 4
		int lcd=p_sistema->lcdId;			
		lcdClear(p_sistema->lcdId);
		lcdPrintf(lcd," %02d:%02d:%02d", p_sistema->reloj.hora.hh,p_sistema->reloj.hora.mm,p_sistema->reloj.hora.ss);
		lcdPosition(p_sistema->lcdId,0,1);		//se muestra por el lcd los valores de p_sistema mostrando 2 decimales
		lcdPrintf(lcd," %02d/%02d/%04d", p_sistema->reloj.calendario.dd,p_sistema->reloj.calendario.MM,p_sistema->reloj.calendario.yyyy);
		lcdPosition(p_sistema->lcdId,0,0);
	#endif
}

void ShowCronometro ( fsm_t* p_this ) {		// Función que muestra el valor actual
	TipoCoreWatch *p_sistema=(TipoCoreWatch *)p_this->user_data;

	piLock(SYSTEM_KEY);
	g_flagsCoreWatch &= ~FLAG_CRONOMETRO;		// Se limpia el flag
	piUnlock(SYSTEM_KEY);
	#if VERSION < 4
		piLock(STD_IO_LCD_BUFFER_KEY);
			printf("Tiempo transcurrido: %d seg.\n",p_sistema->cronometro);		// Muestra por consola el valor del cronómetro
			fflush(stdout);
		piUnlock(STD_IO_LCD_BUFFER_KEY);
	#endif
	#if VERSION >= 4
		int lcd=p_sistema->lcdId;
		lcdClear(p_sistema->lcdId);
		lcdPrintf(lcd," Crono:%d s", p_sistema->cronometro);		// Muestra por el LCD el valor del cronómetro
		delay(ESPERA_MENSAJE_MS);
	#endif
}

void Reset ( fsm_t * p_this ) {
	TipoCoreWatch *p_sistema = (TipoCoreWatch *) p_this->user_data;
	ResetReloj(&(p_sistema->reloj));
	piLock(SYSTEM_KEY);
	g_flagsCoreWatch &= ~FLAG_RESET;
	piUnlock(SYSTEM_KEY);
	p_sistema->cronometro=0;		// Se reinicia el valor de cronómetro cambiando su valor a 0

#if VERSION < 4
	piLock(STD_IO_LCD_BUFFER_KEY);
	printf("[RESET] Hora reiniciada \n");
	fflush(stdout);
	piUnlock(STD_IO_LCD_BUFFER_KEY);
#endif

#if VERSION >= 4
	lcdClear(p_sistema->lcdId);
	lcdPosition(p_sistema->lcdId,0,1);
	lcdPrintf(p_sistema->lcdId,"RESET");
	delay(ESPERA_MENSAJE_MS);
#endif
}

void PrepareSetNewTime ( fsm_t * p_this ) {
	TipoCoreWatch *p_sistema=(TipoCoreWatch *)p_this->user_data;

	piLock(SYSTEM_KEY);
		g_flagsCoreWatch &= ~FLAG_DIGITO_PULSADO;
	piUnlock(SYSTEM_KEY);
	piLock(SYSTEM_KEY);
		g_flagsCoreWatch &= ~FLAG_SET_CANCEL_NEW_TIME;
	piUnlock(SYSTEM_KEY);
	
	#if VERSION < 4
	int formato= p_sistema->reloj.hora.formato;
	piLock(STD_IO_LCD_BUFFER_KEY);
		printf("[SET_TIME] Introduzca la nueva hora en formato 0-%d\n", formato);
		fflush(stdout);
	piUnlock(STD_IO_LCD_BUFFER_KEY);
	#endif

	#if VERSION >= 4
		lcdClear(p_sistema->lcdId);
		lcdPosition(p_sistema->lcdId,0,1);
		lcdPrintf(p_sistema->lcdId,"FORMAT: 0-%d",p_sistema->reloj.hora.formato);
	#endif


}

void PrepareSetCalendario ( fsm_t * p_this ) {		//Prepara el calendario
	TipoCoreWatch *p_sistema=(TipoCoreWatch *)p_this->user_data;

	piLock(SYSTEM_KEY);		//Limpia la flag calendario y de digito
		g_flagsCoreWatch &= ~FLAG_DIGITO_PULSADO;
	piUnlock(SYSTEM_KEY);
	piLock(SYSTEM_KEY);
		g_flagsCoreWatch &= ~FLAG_CALENDARIO;		
	piUnlock(SYSTEM_KEY);

	#if VERSION < 4
	piLock(STD_IO_LCD_BUFFER_KEY);
		printf("[SET_FECHA] Introduzca la nueva fecha (YYYY/MM/DD)");		//Pregunta la nueva fecha
		fflush(stdout);
	piUnlock(STD_IO_LCD_BUFFER_KEY);
	#endif

	#if VERSION >= 4
		lcdClear(p_sistema->lcdId);
		lcdPosition(p_sistema->lcdId,0,1);
		lcdPrintf(p_sistema->lcdId,"FECHA: ");		//Pregunta la nueva fecha por el lcd
	#endif


}

void CancelSetNewTime ( fsm_t * p_this ) {
	TipoCoreWatch *p_sistema=(TipoCoreWatch *)p_this->user_data;
	p_sistema->tempTime=0;
	p_sistema->digitosGuardados=0;
	piLock(SYSTEM_KEY);
		g_flagsCoreWatch &= ~FLAG_SET_CANCEL_NEW_TIME;
	piUnlock(SYSTEM_KEY);

	#if VERSION < 4
	piLock(STD_IO_LCD_BUFFER_KEY);
		printf("[SET_TIME] Operacion cancelada\n");
		fflush(stdout);
	piUnlock(STD_IO_LCD_BUFFER_KEY);
	#endif

	#if VERSION >= 4
		lcdClear(p_sistema->lcdId);
		lcdPosition(p_sistema->lcdId,0,1);
		lcdPrintf(p_sistema->lcdId,"CANCELADO");
		delay(ESPERA_MENSAJE_MS);
	#endif
}

void CancelSetCalendario ( fsm_t * p_this ) {		//Cancela la operación de calendario
	TipoCoreWatch *p_sistema=(TipoCoreWatch *)p_this->user_data;
	p_sistema->tempFecha=0;		//Reinicia la fecha
	p_sistema->digitosGuardados=0;
	piLock(SYSTEM_KEY);
		g_flagsCoreWatch &= ~FLAG_CALENDARIO;		//Limpia la flag
	piUnlock(SYSTEM_KEY);

	#if VERSION < 4
	piLock(STD_IO_LCD_BUFFER_KEY);
		printf("[SET_FECHA] Operacion cancelada\n");		
		fflush(stdout);
	piUnlock(STD_IO_LCD_BUFFER_KEY);
	#endif

	#if VERSION >= 4
		lcdClear(p_sistema->lcdId);
		lcdPosition(p_sistema->lcdId,0,1);
		lcdPrintf(p_sistema->lcdId,"CANCELADO");
		delay(ESPERA_MENSAJE_MS);
	#endif
}

void ProcesaDigitoTime ( fsm_t * p_this ) {
	TipoCoreWatch *p_sistema = (TipoCoreWatch *)p_this->user_data;

	int tempTime=p_sistema->tempTime;
	int digitosGuardados=p_sistema->digitosGuardados;
	int ultimoDigito = p_sistema->digitoPulsado;
	piLock(SYSTEM_KEY);
	g_flagsCoreWatch &= ~FLAG_DIGITO_PULSADO;
	piUnlock(SYSTEM_KEY);
	
	if(digitosGuardados==0){
		if(p_sistema->reloj.hora.formato==12){
			ultimoDigito= MIN(1,ultimoDigito);
		}
		else{
			ultimoDigito= MIN(2,ultimoDigito);
		}
		tempTime=tempTime*10+ultimoDigito;
		digitosGuardados++;
	}
	else{
		if(digitosGuardados==1){
			if(p_sistema->reloj.hora.formato==12){
				if(tempTime==0){
					ultimoDigito=MAX(1,ultimoDigito);
				}
				else{
					ultimoDigito= MIN(2,ultimoDigito);
				}
			}
			else{
				if(tempTime==2){
					ultimoDigito= MIN(3,ultimoDigito);
				}
			}
		tempTime=tempTime*10+ultimoDigito;
		digitosGuardados++;
		}
		else if(digitosGuardados==2){
			tempTime=tempTime*10 + MIN(5, ultimoDigito);
			digitosGuardados++;
		}
		else{
			tempTime=tempTime*10+ultimoDigito;

			piLock(SYSTEM_KEY);
			g_flagsCoreWatch |= FLAG_NEW_TIME_IS_READY;
			piUnlock(SYSTEM_KEY);
		}
	}

	if ( digitosGuardados < 3) {
		if ( tempTime > 2359) {
			tempTime %= 10000;
			tempTime = 100* MIN (( int) ( tempTime /100) , 23) + MIN(
			tempTime %100 , 59) ;
		}
	}
	p_sistema->tempTime=tempTime;
	p_sistema->digitosGuardados=digitosGuardados;
	#if VERSION < 4
	piLock(STD_IO_LCD_BUFFER_KEY);
	printf("[SET_TIME] Nueva hora temporal %d\n", tempTime);
	fflush(stdout);
	piUnlock(STD_IO_LCD_BUFFER_KEY);
	#endif
	#if VERSION >= 4
		lcdClear(p_sistema->lcdId);
		lcdPrintf(p_sistema->lcdId,"SET: %d",p_sistema->tempTime);
	#endif
}

void ProcesaDigitoFecha ( fsm_t * p_this ) {		//Procesa cada valor de la fecha
	TipoCoreWatch *p_sistema = (TipoCoreWatch *)p_this->user_data;
	//Guarda las variables necesarias
	int tempFecha=p_sistema->tempFecha;
	int digitosGuardados=p_sistema->digitosGuardados;
	int ultimoDigito = p_sistema->digitoPulsado;
	piLock(SYSTEM_KEY);
	g_flagsCoreWatch &= ~FLAG_DIGITO_PULSADO;		//Limpa la flag
	piUnlock(SYSTEM_KEY);

	if(digitosGuardados==0){		//Guarda el año durante los 4 primeros dígitos
		tempFecha=tempFecha*10+ultimoDigito;
		digitosGuardados++;
	}
	else if(digitosGuardados==1){
		tempFecha=tempFecha*10+ultimoDigito;
		digitosGuardados++;
	}
	else if(digitosGuardados==2){
		tempFecha=tempFecha*10+ultimoDigito;
		digitosGuardados++;
	}
	else if(digitosGuardados==3){
		tempFecha=tempFecha*10+ultimoDigito;
		digitosGuardados++;
	}
	else if(digitosGuardados==4){		//Amacena los meses en los dos siguientes dígitos
		ultimoDigito= MIN(1,ultimoDigito);
		tempFecha=MIN(tempFecha,2999)*10+ultimoDigito;		//Si el año es mayor que 2999 pondrá 2999 como año
		digitosGuardados++;
	}
	else if(digitosGuardados==5){
		int ant = (tempFecha%10);		//Permite saber el último digito guardado
		if(ant==0){
			tempFecha=tempFecha*10+ultimoDigito;		//Si el último dígito guardado no es 1 valdrá cualquier valor
			digitosGuardados++;
		}
		else{		//En caso contrario el máximo valor en ese dígito es un 2
			ultimoDigito= MIN(2,ultimoDigito);
			tempFecha=tempFecha*10+ultimoDigito;
			digitosGuardados++;
		}
	}
	else if(digitosGuardados>5 && digitosGuardados<8){		//Almaccena los días
		int year = tempFecha/100;
		int mes = tempFecha-year*100;
		int diamax = CalculaDiasMes(mes,year);		//Calcula los días para el mes y año seleccionados
		if(digitosGuardados==6){
			tempFecha=tempFecha*10+ultimoDigito;
			digitosGuardados++;
		}
		else{
			tempFecha=tempFecha*10+ultimoDigito;
			digitosGuardados++;
				// Coge cada dígito almacenado en tempFecha
				int nuevaFecha3=(p_sistema->tempFecha/10000000);
				int nuevaFecha2=((p_sistema->tempFecha/1000000)-nuevaFecha3*10);
				int nuevaFecha1=((p_sistema->tempFecha/100000)-(nuevaFecha3)*100-(nuevaFecha2)*10);
				int nuevaFecha0=((p_sistema->tempFecha/10000)-(nuevaFecha3)*1000-(nuevaFecha2)*100-(nuevaFecha1)*10);
				int nuevaFecha5=((p_sistema->tempFecha/1000)-(nuevaFecha3)*10000-(nuevaFecha2)*1000-(nuevaFecha1)*100-(nuevaFecha0)*10);
				int nuevaFecha4=((p_sistema->tempFecha/100)-(nuevaFecha3)*100000-(nuevaFecha2)*10000-(nuevaFecha1)*1000-(nuevaFecha0)*100-(nuevaFecha5)*10);
				int nuevaFecha7=((p_sistema->tempFecha/10)-(nuevaFecha3)*1000000-(nuevaFecha2)*100000-(nuevaFecha1)*10000-(nuevaFecha0)*1000-(nuevaFecha5)*100-(nuevaFecha4)*10);
				int nuevaFecha6=((p_sistema->tempFecha)-(nuevaFecha3)*10000000-(nuevaFecha2)*1000000-(nuevaFecha1)*100000-(nuevaFecha0)*10000-(nuevaFecha5)*1000-(nuevaFecha4)*100-(nuevaFecha7)*10);
				int nuevaFecha = nuevaFecha6+nuevaFecha7*10;		//Coge los dígitos de los días

			if(nuevaFecha>diamax){
				tempFecha = (tempFecha-nuevaFecha) + diamax;		//Si el día es mayor que el día máximo lo cambia por el dia máximo en tempFecha
			}
			piLock(SYSTEM_KEY);
			g_flagsCoreWatch |= FLAG_NEW_TIME_IS_READY;		//Activa la flag al haber terminado
			piUnlock(SYSTEM_KEY);
		}
	}
	p_sistema->tempFecha=tempFecha;
	p_sistema->digitosGuardados=digitosGuardados;
	#if VERSION < 4
	piLock(STD_IO_LCD_BUFFER_KEY);
	printf("[SET_FECHA] Nuevo calendario temporal (YYYY/MM/DD) %d\n", tempFecha);		//Muestra la fecha
	fflush(stdout);
	piUnlock(STD_IO_LCD_BUFFER_KEY);
	#endif
	#if VERSION >= 4
		lcdClear(p_sistema->lcdId);
		lcdPrintf(p_sistema->lcdId,"SET: %d",p_sistema->tempFecha);		//Muestra la fecha
	#endif
}

void SetNewTime ( fsm_t * p_this ) {
	TipoCoreWatch *p_sistema=(TipoCoreWatch *)p_this->user_data;
	piLock(SYSTEM_KEY);
		g_flagsCoreWatch &= ~FLAG_NEW_TIME_IS_READY;
	piUnlock(SYSTEM_KEY);
	setHora(p_sistema->tempTime,&(p_sistema->reloj.hora));
	p_sistema->tempTime=0;
	p_sistema->digitosGuardados=0;
}

void SetNewFecha ( fsm_t * p_this ) {		//Cambia la fecha
	TipoCoreWatch *p_sistema=(TipoCoreWatch *)p_this->user_data;
	piLock(SYSTEM_KEY);
		g_flagsCoreWatch &= ~FLAG_NEW_TIME_IS_READY;		//Limpia la flag
	piUnlock(SYSTEM_KEY);	// Coge cada dígito almacenado en tempFecha
	int nuevaFecha3=(p_sistema->tempFecha/10000000);
	int nuevaFecha2=((p_sistema->tempFecha/1000000)-nuevaFecha3*10);
	int nuevaFecha1=((p_sistema->tempFecha/100000)-(nuevaFecha3)*100-(nuevaFecha2)*10);
	int nuevaFecha0=((p_sistema->tempFecha/10000)-(nuevaFecha3)*1000-(nuevaFecha2)*100-(nuevaFecha1)*10);
	int nuevaFecha5=((p_sistema->tempFecha/1000)-(nuevaFecha3)*10000-(nuevaFecha2)*1000-(nuevaFecha1)*100-(nuevaFecha0)*10);
	int nuevaFecha4=((p_sistema->tempFecha/100)-(nuevaFecha3)*100000-(nuevaFecha2)*10000-(nuevaFecha1)*1000-(nuevaFecha0)*100-(nuevaFecha5)*10);
	int nuevaFecha7=((p_sistema->tempFecha/10)-(nuevaFecha3)*1000000-(nuevaFecha2)*100000-(nuevaFecha1)*10000-(nuevaFecha0)*1000-(nuevaFecha5)*100-(nuevaFecha4)*10);
	int nuevaFecha6=((p_sistema->tempFecha)-(nuevaFecha3)*10000000-(nuevaFecha2)*1000000-(nuevaFecha1)*100000-(nuevaFecha0)*10000-(nuevaFecha5)*1000-(nuevaFecha4)*100-(nuevaFecha7)*10);
	int nuevaFecha = nuevaFecha3*10000000+nuevaFecha2*1000000+nuevaFecha1*100000+nuevaFecha0*10000+nuevaFecha5*1000+nuevaFecha4*100+nuevaFecha7*10+nuevaFecha6;		//Cambia de orden los dígitos de la fecha
	setCalendario(nuevaFecha,&(p_sistema->reloj.calendario));
	p_sistema->tempFecha=0;		//reinicia el valor de la fecha y los dígitos guardados
	p_sistema->digitosGuardados=0;
}

// Wait until next_activation (absolute time)
// Necesita de la función "delay" de WiringPi.
void DelayUntil(unsigned int next) {
	unsigned int now = millis();
	if (next > now) {
		delay(next - now);
	}
}

int CompruebaDigitoPulsado(fsm_t* p_this){
	piLock(SYSTEM_KEY); 
		int act = g_flagsCoreWatch & FLAG_DIGITO_PULSADO; 
	piUnlock(SYSTEM_KEY);
	return act;
}

int CompruebaNewTimeIsReady ( fsm_t * p_this ) {
	piLock(SYSTEM_KEY); 
		int act = g_flagsCoreWatch & FLAG_NEW_TIME_IS_READY; 
	piUnlock(SYSTEM_KEY);
	return act;
}

int CompruebaReset ( fsm_t * p_this ) {
	piLock(SYSTEM_KEY); 
		int act = g_flagsCoreWatch & FLAG_RESET; 
	piUnlock(SYSTEM_KEY);
	return act;
}

int CompruebaCronometro ( fsm_t * p_this ) {		// Comprueba si se ha levantado la flag
	piLock(SYSTEM_KEY);
		int act = g_flagsCoreWatch & FLAG_CRONOMETRO;
	piUnlock(SYSTEM_KEY);
	return act;
}

int CompruebaSetCancelNewTime ( fsm_t * p_this ) {
	piLock(SYSTEM_KEY); 
		int act = g_flagsCoreWatch & FLAG_SET_CANCEL_NEW_TIME; 
	piUnlock(SYSTEM_KEY);
	return act;
}

int CompruebaSetCalendario ( fsm_t * p_this ) {		//Comprueba la flag de calendario
	piLock(SYSTEM_KEY);
		int act = g_flagsCoreWatch & FLAG_CALENDARIO;
	piUnlock(SYSTEM_KEY);
	return act;
}

int CompruebaSetupDone ( fsm_t * p_this ) {
	int act;
	piLock(SYSTEM_KEY); 
		act = g_flagsCoreWatch & FLAG_SETUP_DONE;
	piUnlock(SYSTEM_KEY);
	return act;
}

int CompruebaTeclaPulsada ( fsm_t * p_this ) {

	TipoTecladoShared flag = GetTecladoSharedVar();
	return flag.flags & FLAG_TECLA_PULSADA;
}

int CompruebaTimeActualizado ( fsm_t * p_this ) {
	TipoRelojShared aux = GetRelojSharedVar();
	return aux.flags & FLAG_TIME_ACTUALIZADO;
}

void ProcesaTeclaPulsada ( fsm_t * p_this ){
	TipoCoreWatch *p_sistema = (TipoCoreWatch *)p_this->user_data;
	TipoTecladoShared flag = GetTecladoSharedVar();
	flag.flags &= ~FLAG_TECLA_PULSADA;
	SetTecladoSharedVar(flag);
	char teclaPulsada = flag.teclaDetectada;
		delay (10) ; // WiringPi function : pauses program
		//execution for at least 10 ms
		teclaPulsada = toupper(teclaPulsada);

		if((char)teclaPulsada == TECLA_RESET){
			piLock(SYSTEM_KEY);
			g_flagsCoreWatch |= FLAG_RESET;
			piUnlock(SYSTEM_KEY);
		}
		else if((char)teclaPulsada == TECLA_SET_CANCEL_TIME){
			piLock(SYSTEM_KEY);
			g_flagsCoreWatch |= FLAG_SET_CANCEL_NEW_TIME;
			piUnlock(SYSTEM_KEY);
		}
		else if((char)teclaPulsada == TECLA_CRONOMETRO){		// Comprobamos la tecla del cronómetro
			piLock(SYSTEM_KEY);
			g_flagsCoreWatch |= FLAG_CRONOMETRO;		// Activamos el flag del cronómetro
			piUnlock(SYSTEM_KEY);
		}
		else if(EsNumero(teclaPulsada)){
			piLock(SYSTEM_KEY);
			g_coreWatch.digitoPulsado = teclaPulsada -48;
			g_flagsCoreWatch |= FLAG_DIGITO_PULSADO;
			piUnlock(SYSTEM_KEY);
		}
		else if((char)teclaPulsada == TECLA_CALENDARIO){		// Comprobamos la tecla del calendario
			piLock(SYSTEM_KEY);
			g_flagsCoreWatch |= FLAG_CALENDARIO;		// Activamos el flag del calendario
			piUnlock(SYSTEM_KEY);
		}
		else if((char)teclaPulsada == TECLA_EXIT){
			piLock(STD_IO_LCD_BUFFER_KEY);
			printf("Se va a salir del sistema \n");
			fflush(stdout);
			piUnlock(STD_IO_LCD_BUFFER_KEY);
			#if VERSION ==4
					lcdClear(p_sistema->lcdId);
					lcdPrintf(p_sistema->lcdId,"APAGANDO");
					delay(ESPERA_MENSAJE_MS);
			#endif
			exit(0);
		}
		else if((teclaPulsada != '\n')&&(teclaPulsada != '\r') && (teclaPulsada != 0xA)) {
			piLock(STD_IO_LCD_BUFFER_KEY);
			printf("Tecla desconocida \n");
			fflush(stdout);
			piUnlock(STD_IO_LCD_BUFFER_KEY);
			#if VERSION ==4
					lcdClear(p_sistema->lcdId);
					lcdPrintf(p_sistema->lcdId,"TECLA DESCONOCIDA");
					delay(ESPERA_MENSAJE_MS);
			#endif
		}
	#if VERSION >= 4
		lcdClear(p_sistema->lcdId);
		lcdPrintf(p_sistema->lcdId,"TEC: %d",teclaPulsada);
	#endif
}

//------------------------------------------------------
// MAIN
//------------------------------------------------------
int main() {
	unsigned int next;

#if VERSION <= 1
	TipoReloj miReloj;

	ConfiguraInicializaReloj(&miReloj);
	setCalendario(20050507, &(miReloj.calendario));

	fsm_t* fsmReloj = fsm_new(WAIT_TIC, g_fsmTransReloj, &miReloj);
#endif

#if VERSION == 2
	fsm_t* fsmReloj = fsm_new(WAIT_TIC, g_fsmTransReloj, &(g_coreWatch.reloj));
	fsm_t* fsmCoreWatch= fsm_new(START, fsmTransCoreWatch, &g_coreWatch);

	int ini_sis=ConfiguraInicializaSistema(& g_coreWatch);
	if(ini_sis != 0) {


		piLock(STD_IO_LCD_BUFFER_KEY);
		printf("Ha habido un error\n");
		fflush(stdout);
		piUnlock(STD_IO_LCD_BUFFER_KEY);
		exit(0);
	}
#endif

#if VERSION >= 3
	fsm_t* fsmReloj = fsm_new(WAIT_TIC, g_fsmTransReloj, &(g_coreWatch.reloj));
	fsm_t* fsmCoreWatch = fsm_new(START, fsmTransCoreWatch, &g_coreWatch);
	fsm_t* deteccionComandosFSM = fsm_new(WAIT_COMMAND, fsmTransDeteccionComandos, &g_coreWatch);
	fsm_t* tecladoFSM = fsm_new(TECLADO_ESPERA_COLUMNA, g_fsmTransExcitacionColumnas, &(g_coreWatch.teclado));

	int ini_sis=ConfiguraInicializaSistema(& g_coreWatch);
	if(ini_sis != 0) {


		piLock(STD_IO_LCD_BUFFER_KEY);
		printf("Ha habido un error\n");
		fflush(stdout);
		piUnlock(STD_IO_LCD_BUFFER_KEY);
		exit(0);
	}
#endif

	while (1) {
		fsm_fire(fsmReloj);
#if VERSION == 2
		fsm_fire(fsmCoreWatch);
#endif
#if VERSION >= 3
		fsm_fire(deteccionComandosFSM);
		fsm_fire(tecladoFSM);
		fsm_fire(fsmCoreWatch);
#endif
		next += CLK_MS;
		DelayUntil(next);
	}

	tmr_destroy(g_coreWatch.reloj.tmrTic);
	tsm_destroy(g_coreWatch.teclado.tmr_duracion_columna);
	#if VERSION > 1
		fsm_destroy(fsmCoreWatch);
	#endif
	fsm_destroy(fsmReloj);
}





