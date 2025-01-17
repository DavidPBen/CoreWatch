/*

 * reloj.c
 *
 *  Created on: 9 de feb. de 2022
 *      Author: alumno
 */
#include "reloj.h"

fsm_trans_t g_fsmTransReloj[] = {{WAIT_TIC,CompruebaTic,WAIT_TIC,ActualizaReloj},{ -1 , NULL , -1 , NULL }};
const int DIAS_MESES[2][MAX_MONTH] = {{31,28,31,30,31,30,31,31,30,31,30,31},{31,29,31,30,31,30,31,31,30,31,30,31}}; //Primero no bisiestos, luego bisiestos.
static TipoRelojShared g_relojSharedVars;



void ResetReloj(TipoReloj *p_reloj){
	TipoCalendario calendario = {DEFAULT_DAY, DEFAULT_MONTH, DEFAULT_YEAR};
	TipoHora hora;
		hora.hh=DEFAULT_HOUR;
		hora.mm=DEFAULT_MIN;
		hora.ss=DEFAULT_SEC;
		hora.formato=DEFAULT_TIME_FORMAT;

	p_reloj->calendario = calendario;
	p_reloj->hora = hora;
	p_reloj->timestamp = 0;
	p_reloj->cronometro=0;		// Se reinicia el cronómetro.

	piLock(RELOJ_KEY);
	g_relojSharedVars.flags=0;
    piUnlock(RELOJ_KEY);
}

int ConfiguraInicializaReloj(TipoReloj *p_reloj){
	ResetReloj(p_reloj);
	tmr_t* tmrn=tmr_new(tmr_actualiza_reloj_isr);
	p_reloj->tmrTic=tmrn;
	tmr_startms_periodic(tmrn,PRECISION_RELOJ_MS);
	return 0;
}

//------------------------------------------------------
// FUNCIONES DE ENTRADA O DE TRANSICION DE LA MAQUINA DE ESTADOS
//------------------------------------------------------
int CompruebaTic(fsm_t *p_this) {
	int result = 0;

	piLock(RELOJ_KEY);
	result = g_relojSharedVars.flags & FLAG_ACTUALIZA_RELOJ;
	piUnlock(RELOJ_KEY);

	return result;
}

//------------------------------------------------------
// FUNCIONES DE SALIDA O DE ACCION DE LA MAQUINA DE ESTADOS
//------------------------------------------------------
void ActualizaReloj(fsm_t *p_this) {


	TipoReloj *p_miReloj = (TipoReloj *)p_this->user_data;

	piLock(RELOJ_KEY);
	g_relojSharedVars.flags &= ~FLAG_ACTUALIZA_RELOJ;
	piUnlock(RELOJ_KEY);

	p_miReloj->timestamp = p_miReloj->timestamp + 1;
	ActualizaHora(&(p_miReloj->hora));
	if(p_miReloj->hora.hh == 00 && p_miReloj->hora.mm == 00 && p_miReloj->hora.ss == 00){
		ActualizaFecha(&(p_miReloj->calendario));
	}

	piLock(RELOJ_KEY);
	g_relojSharedVars.flags |= FLAG_TIME_ACTUALIZADO;
	piUnlock(RELOJ_KEY);

#if VERSION == 1
	piLock(STD_IO_LCD_BUFFER_KEY);

	p_miReloj->cronometro = p_miReloj->cronometro+1;		// Se incrementa 1 el valor del cronómetro
	printf("Tiempo transcurrido: %d seg.\n",p_miReloj->cronometro);		// Muestra por consola el valor del cronómetro
	fflush(stdout);

	printf("Son las: %02d:%02d:%02d del %02d/%02d/%04d\n",
			p_miReloj->hora.hh,
			p_miReloj->hora.mm,
			p_miReloj->hora.ss,
			p_miReloj->calendario.dd,
			p_miReloj->calendario.MM,
			p_miReloj->calendario.yyyy);
	fflush(stdout);

	piUnlock(STD_IO_LCD_BUFFER_KEY);
#endif
}

void ActualizaHora(TipoHora *p_hora){
	int nuevoseg = p_hora->ss+1;
	int modseg = (nuevoseg)%(60);
	if(modseg==0){
		int nuevomin = p_hora->mm+1;
		int modmin = (nuevomin)%(60);
		if(modmin==0){
			p_hora->ss = 00;
			p_hora->mm = 00;
			if(p_hora->formato==TIME_FORMAT_12_H){
					p_hora->hh = (p_hora->hh+1)%(p_hora->formato);
			}
			else{
				p_hora->hh = (p_hora->hh+1)%(p_hora->formato);
			}
		}
		else{
			p_hora->ss = 00;
			p_hora->mm = p_hora->mm+1;
		}
	}else{
		p_hora->ss = p_hora->ss+1;
	}
}

void ActualizaFecha (TipoCalendario * p_fecha){
	int dias_mes=CalculaDiasMes(p_fecha->MM,p_fecha->yyyy);
	int nuevodia = p_fecha->dd+1;
	int nuevomes = p_fecha->MM+1;
	int mod = (nuevodia)%(dias_mes+1);
	if(mod==0){
		int modmes = (nuevomes % (MAX_MONTH+1));
		if(modmes!=0){
			p_fecha->dd = 1;
			p_fecha->MM = p_fecha->MM+1;
		}
		else{
			p_fecha->dd = 1;
			p_fecha->MM = 1;
			p_fecha->yyyy = p_fecha->yyyy+1;
		}
	}
	else{
		p_fecha->dd = p_fecha->dd+1;
	}
}

int CalculaDiasMes(int month,int year) {
	int bisiesto = EsBisiesto(year);
	return DIAS_MESES[bisiesto][month-1];
}

int EsBisiesto(int year){
	if(year%4==0){
		if(year%100==0){
			if(year%400==0){
				return 1;
			}
			else{
				return 0;
			}
		}
		else{
			return 1;
		}
	}
	else{
		return 0;
	}
}

int setHora(int horaInt,TipoHora *p_hora){
	int hhs = 0;
	int numero_digitos = 0;
	int auxiliar = 0;

	if(horaInt>=0){
		numero_digitos = 0;
		auxiliar = horaInt;
		do{
			auxiliar=auxiliar/10;
			numero_digitos = numero_digitos + 1;
		} while(auxiliar != 0);

		if (numero_digitos>0 && numero_digitos<5){
			hhs=(int)horaInt/100;
			if(hhs>MAX_HOUR){
				hhs=MAX_HOUR;
			}
			p_hora->hh=hhs;
			if(hhs==0 && p_hora->formato==TIME_FORMAT_12_H){
				p_hora->hh=12; 
			}
			if(hhs>12 && p_hora->formato==TIME_FORMAT_12_H){
				p_hora->hh=p_hora->hh-12;
			}

			if(horaInt-hhs*100>MAX_MIN){
				p_hora->mm = MAX_MIN;
			}
			else
				p_hora->mm = horaInt-hhs*100;

			p_hora->ss=00;
			return 0;
		}else{
			return 1;
		}
	}
	else{
		return 1;
	}
}

int setCalendario(int fechaInt,TipoCalendario *p_calendario){		//Método que actualiza el calendario
	int numero_digitos = 0;
	int auxiliar = 0;
	// yyyy mm dd
	int yys=0;							
	int MMs=0;
	int dds=0;

	if(fechaInt>=0){
		numero_digitos = 0;
		auxiliar = fechaInt;
		do{
			auxiliar=auxiliar/10;
			numero_digitos = numero_digitos + 1;
		} while(auxiliar != 0);

		if (numero_digitos>0 && numero_digitos<9){		//Comprueba el número de dígitos que tiene la fecha introducida
			yys=(int)fechaInt/10000;
			if(yys>MAX_YEAR){	//A�o maximo
				yys=MAX_YEAR;
			}
			else if(yys<MIN_YEAR){	//A�o m�nimo
				yys=MIN_YEAR;
			}
			p_calendario->yyyy=yys;	//A�o

			MMs=(fechaInt-yys*10000)/100;

			if(MMs>MAX_MONTH){	//Mes maximo
				MMs = MAX_MONTH;
			}
			else if(MMs<MIN_MONTH){	//Mes m�nimo
				MMs = MIN_MONTH;
			}
			p_calendario->MM = MMs;	//Mes

			int dia_max=CalculaDiasMes(MMs,yys);		//Calcula los día máximo que tiene ese mes
			dds=fechaInt-yys*10000-MMs*100;

			if(dds>dia_max){	//D�a m�ximo
				dds = dia_max;
			}
			else if(dds<MIN_DAY){	//D�a m�nimo
				dds = MIN_DAY;
			}
			p_calendario->dd = dds;	//D�a

			return 0;
		}else{
			return 1;
		}
	}
	else{
		return 1;
	}
}

void tmr_actualiza_reloj_isr(union sigval value) {
	piLock(RELOJ_KEY);
	g_relojSharedVars.flags |= FLAG_ACTUALIZA_RELOJ;
	piUnlock(RELOJ_KEY);
}

TipoRelojShared GetRelojSharedVar() {
	TipoRelojShared relojs;
	piLock(RELOJ_KEY);
	relojs = g_relojSharedVars;
	piUnlock(RELOJ_KEY);
	return relojs;
}

void SetRelojSharedVar(TipoRelojShared value) {
	piLock(RELOJ_KEY);
	g_relojSharedVars = value;
	piUnlock(RELOJ_KEY);
}

