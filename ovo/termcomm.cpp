/**********************************************************
 *
 * Программа предназначена для контроля за устройством управления инкубатором, 
 * оснащенным подогревом, вентиляцией, увлажнителем воздуха, механизмом переворота.
 * Управление контроллером предусмотрено как со стороны компьютера по интерфейсу,
 * так и локально с помощью клавиатуры и простого дисплея (LCD или 7-сегментный)
 * Авторегулирование нагрева и вентиляции предусмотрено с пом. ПИД регуляторов.
 * Управление освещением с контролем по датчикам и времени суток.
 *
 * Для возможности настройки контроллера предусмотрено одновременное чтение компорта
 * и запись результата в окна консоли (в отдельном потоке)
 *
 * Данные ПО получает из порта, куда они поступают от Arduino
 * Arduino имеет на борту несколько необходимых датчиков и исполнительных узлов:
 *		тэн, испарители, реле, ШИМ, светодиодные ленты освещения, вентилятор, ..
 *
 * Через компорт поступает строка с числами в формате
 * float, разделенных пробелами и символом конца строки в конце
 * Частота прихода таких строк ограничена baudrate == {9600, 19200, .. 115200}
 * Частота опроса контроллера со стороны ПК с периодом от 1 до 5 сек,
 *
 * Author:  Starmark LN (Kornilov LN)
 * e-mail:  ln.KornilovStar@gmail.com
 *          ln.starmark@ekatra.io
 *          ln.starmark@gmail.com
 * date:    24.08.2023
 *
 *********************************************************/

#include "termcomm.h"
#include "comm.h"

#include <stdexcept>
#include <thread>
#include <mutex>

#define V_main		'0'
#define V_middle	'1'
#define V_little	'0'

#define ETAP_1		3		
#define ETAP_2		15
#define ETAP_3		18
#define ETAP_OUT	19

std::mutex cout_mutex;

stArrPayval dt;
stPortVal   dtport;

stFrameXY   frmTitle= {4,2};
stFrameXY   frmDatetime= {32,2};

stFrameXY   frmDay= {8,4};
stFrameXY   frmEtap= {25,4};

stFrameXY   frmAlarmDS1t= {9,8};
stFrameXY   frmAlarmDHTt= {9,9};
stFrameXY   frmAlarmLed = {9,10};
stFrameXY   frmAlarmVent= {9,11};

stFrameXY   frmTechEtap = {20,8};
stFrameXY   frmTechDay  = {20,9};
stFrameXY   frmTechRot  = {20,10};

stFrameXY   frmPwmTen   = {34,8};
stFrameXY   frmPwmLed   = {34,9};
stFrameXY   frmPwmVent  = {34,11};

stFrameXY   frmBoxD1t   = {50,8};
stFrameXY   frmBoxDHTt  = {50,9};
stFrameXY   frmBoxDHTh  = {50,10};
stFrameXY   frmBoxLight = {50,11};

stFrameXY   frmRoomt    = {66,8};
stFrameXY   frmRoomh    = {66,9};
stFrameXY   frmRoomp    = {66,10};
stFrameXY   frmRoomD2t  = {66,11};

stFrameXY   frmStreetD3t= {66,15};

stFrameXY   frmMess = {9,15};

stFrameXY   frmPortDataEtap = {20,8};
stFrameXY   frmPortDataDay  = {20,9};
stFrameXY   frmPortDataRot  = {20,10};


void frame_draw ();
void print_title_time_date (struct tm* tm_info); 
void print_Data(payval& pv, int ind, stFrameXY& frm, int clr);
void print_PortData(stPortVal& port, int ind, stFrameXY& frm, int clr);
void print_PortDataEtap(stPortVal& port, stFrameXY& frm, int clr);
void print_PortDataMess(stPortVal& port, stFrameXY& frm, int clr);
void print_alarm(stPortVal& port, int ind, stFrameXY& frm, int clr);
void controller ();

const char* Etap_text[] = { "Initial warming of eggs", 
							"The main warm-up period", 
							"Egg spitting           ", 
							"OUT from all etapes    "}; 
							
const char* Mess_text[] = { "The temperature has gone beyond ", 
							"Humidity has gone beyond        ", 
							"Violation of lighting conditions", 
							"Inadequate ventilation          "}; 							

//=======================================================================

void controller () {
	unsigned char  port = 0x01;

	int alarm_counter = 0;
	time_t timer;
	struct tm* tm_info;
	srand(time(NULL));

    frame_draw ();
 
	while(1){

   		//--- Взять дату и время и подготовить к выводу
		time(&timer);
		tm_info = localtime(&timer);

   		//==================================
   		print_title_time_date(tm_info);
   		
   		/*
   		//011 10001  01100110
   		dtport = {0x6000 + 0x1100 + 0x0066, 0}; //--- etap:3 bit; day: 5 bit; rot: 8 bit		
   		dt.DhtL = {36.30, 54.28, 1023.00};
   		dt.PWM = {77.13, 99.33, 20.00};
   		dt.Bme = {26.27, 34.77, 670.83};
   		dt.Ds = {36.00, 28.00, -19.50};
   		*/
   		
   		print_PortDataEtap(dtport, frmEtap, 2);     		 		 		
   		print_PortData(dtport, 1, frmDay, 2);
   		   		
   		print_PortData(dtport, 0, frmPortDataEtap, 2);
   		print_PortData(dtport, 1, frmPortDataDay, 2);
   		print_PortData(dtport, 2, frmPortDataRot, 2);
   		
   		print_alarm (dtport, 0, frmAlarmDS1t, 0);
        print_alarm (dtport, 1, frmAlarmDHTt, 0);
        print_alarm (dtport, 2, frmAlarmLed, 0);
        print_alarm (dtport, 3, frmAlarmVent, 0);
        
   		
   		print_Data(dt.PWM, 0, frmPwmTen, 4);
   		print_Data(dt.PWM, 1, frmPwmLed, 4);
   		print_Data(dt.PWM, 2, frmPwmVent, 4);   		
   		
  		print_Data(dt.DhtL, 0, frmBoxDHTt, 2);
   		print_Data(dt.DhtL, 1, frmBoxDHTh, 2);
   		print_Data(dt.DhtL, 2, frmBoxLight, 2);   	  		
   		
   		print_Data(dt.Ds, 0, frmBoxD1t, 2);
   		print_Data(dt.Ds, 1, frmRoomD2t, 1);
   		print_Data(dt.Ds, 2, frmStreetD3t, 3);  
   		
   		print_Data(dt.Bme, 0, frmRoomt, 1);
   		print_Data(dt.Bme, 1, frmRoomh, 1);
   		print_Data(dt.Bme, 2, frmRoomp, 1);  	         
        
        print_PortDataMess(dtport, frmMess, 2);
        
   		//==================================

		gotoxy(1,17);
		
		fflush(stdout);
        	usleep(PAUSE_LEN);
    	}
}

//=======================================================================

int main (void) {

	clPort prt;
    prt.Open();
	prt.SetAttributes();

    std::thread t1(controller);

    prt.Work(dt, dtport);

	prt.Close();
    t1.join();

	return 0;
}

//=======================================================================

void frame_draw (void) {
	home();
	clrscr();

	set_display_atrib(DISPLAY_BACKGROUND);
    puts(WIN);
	resetcolor();
}

void print_title_time_date (struct tm* tm_info) {
	char buffer[20];
	set_display_atrib(DISPLAY_BRIGHT);
	set_display_atrib(DISPLAY_BACKGROUND);
	
    gotoxy(frmTitle.X,frmTitle.Y);    
	sprintf(buffer, "  OvO    (%c.%c.%c)%c", V_main, V_middle, V_little, 0);
    puts(buffer);	

    strftime(buffer, 10, "%d:%m:%y", tm_info);
	gotoxy(frmTitle.X+40,frmTitle.Y)
	puts(buffer);

	strftime(buffer, 10, "%H:%M:%S", tm_info);
	gotoxy(frmTitle.X+50,frmTitle.Y)
	puts(buffer);

	resetcolor();
}

void print_PortDataEtap(stPortVal& port, stFrameXY& frm, int clr) {
   	char buf[40] = {'\0'};

	SETDISPLAYATTRIBS

	set_display_atrib(display_atrib + clr);
	gotoxy(frm.X,frm.Y);
	
	uint16_t dt = port.prt;
	dt &= 0x1F00; 
	dt >>= 8; 
	
	if(dt < ETAP_1)	{
		sprintf(buf, Etap_text[0], 0);		
	}else
	if(dt < ETAP_2)	{
		sprintf(buf, Etap_text[1], 0);		
	}else
	if(dt < ETAP_3)	{
		sprintf(buf, Etap_text[2], 0);		
	}else {
		sprintf(buf, Etap_text[3], 0);		
	}	

    puts(buf);

	resetcolor();
}

void print_PortData(stPortVal& port, int ind, stFrameXY& frm, int clr) {
   	char buf[16] = {'\0'};

	SETDISPLAYATTRIBS

	set_display_atrib(display_atrib + clr);
	gotoxy(frm.X,frm.Y);
	
	uint16_t dt = port.prt;
	switch(ind){
	case 0: dt >>= 13; break;
	case 1: dt &= 0x1F00; dt >>= 8; break;
	case 2: dt &= 0x00FF; break;
	}
	
    sprintf(buf, "% 4d%c", dt, 0);
    puts(buf);

	resetcolor();
}

void print_Data(payval& pv, int ind, stFrameXY& frm, int clr) {
   	char buf[16] = {'\0'};

	SETDISPLAYATTRIBS

	set_display_atrib(display_atrib + clr);
	gotoxy(frm.X,frm.Y);
	
	float dt = 0.0f;
	switch(ind){
	case 0: dt = pv.f0; break;
	case 1: dt = pv.f1; break;
	case 2: dt = pv.f2; break;
	default: dt = -1000.0f; break;
	}
	
    sprintf(buf, "%4.2f%c", dt, 0);
    puts(buf);

	resetcolor();
}

void print_alarm(stPortVal& port, int ind, stFrameXY& frm, int clr) {

	char buf[2];
	uint32_t msk = 0x00000000;
	switch(ind) {
	case 0: msk = 0x00000001; break;
	case 1: msk = 0x00000002; break;
	case 2: msk = 0x00000004; break;
	case 3: msk = 0x00000008; break;
	}
	
	SETDISPLAYATTRIBS

	set_display_atrib(display_atrib + clr);
	gotoxy(frm.X,frm.Y);
	
	if ((port.alarm & msk) == msk) {
		sprintf(buf, "%s%c", "*", 0);    	
    }else{
    	sprintf(buf, "%s%c", " ", 0);   	
    }
    
    puts(buf);
    
    resetcolor();
}

void print_PortDataMess(stPortVal& port, stFrameXY& frm, int clr) {
   	char buf[40] = {'\0'};

	SETDISPLAYATTRIBS

	set_display_atrib(display_atrib + clr);
	gotoxy(frm.X,frm.Y);
	
	if(port.alarm == 1)	{
		sprintf(buf, Mess_text[0], 0);		
	}else
	if(port.alarm == 2)	{
		sprintf(buf, Mess_text[1], 0);		
	}else
	if(port.alarm == 4)	{
		sprintf(buf, Mess_text[2], 0);		
	}else 
	if(port.alarm == 8) {
		sprintf(buf, Mess_text[3], 0);		
	}	

    puts(buf);

	resetcolor();
}

/*
void print_message (stPortVal& pr) {
	gotoxy(frmAlrm.X,frmAlrm.Y);
	if (pr.alarm == 1) {
		set_display_atrib(DISPLAY_BRIGHT);
		set_display_atrib(B_BLUE);
        	set_display_atrib(F_RED);
		printf("      Work ERROR       ");
	} else {
		set_display_atrib(DISPLAY_BRIGHT);
		set_display_atrib(B_GREEN);
		printf("        Work OK        ");
	}
}
*/

//=======================================================================


