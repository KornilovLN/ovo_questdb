#pragma once

#include <string.h>
#include <cstdio>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>   //--- File Control Definitions          
#include <termios.h> //--- POSIX Terminal Control Definitions
#include <unistd.h>  //--- UNIX Standard Definitions 	   
#include <errno.h>   //--- ERROR Number Definitions       

// Включение заголовочного файла <string> для C++
#include <string>

#define EN_ADDINFO	0
//#define EN_PRINT	
#define PORT		"/dev/ttyUSB0"
#define LENBUFFER   256
#define LENPRIHOD	150

#define MAX_SVAL	14	//--- Количество чисел в строке прихода	
#define MAX_SIMB	8	//--- количество символов в подстроке представления числа


//--- Добавить в stArrPayval, генерировать в ардуино и гнать в порт 
typedef struct {
	uint32_t 	prt;	//--- etap:3 bit (3 или 4); day: 5 bit (~ 17); rot: 8 bit (>= 102)	 	 	
    uint32_t 	alarm;  //--- Мл. 4 бита - алармы {гл. температура, гл. влажность, свет, вент.}
}stPortVal;

typedef struct {
    float f0;
    float f1;
    float f2;
}payval;

typedef struct {
	payval PWM; 		//--- Ten, Led, Vent duty(%) {Ten-наполнение, а также: Led, Vent в %}   
	payval DhtL;		//--- DHT+Light{t,h,l}		 {температура, влажность, свет}
	payval Ds;  		//--- array DS {t1,t2,t3}    {3 температуры с массива DS18B20}
    payval Bme;			//--- Bme280   {t,h,p} 		 {температура, влажность, давление}
}stArrPayval;

class clPort
{
public:
	clPort();
	~clPort();

    void Open();
	void SetAttributes();
	void Close();
    void Work(stArrPayval& dt, stPortVal& port);

	char read_buffer[LENBUFFER];    //--- Buffer to store the data received  
protected:

private:
	int fd;							//--- File Descriptor
    int CheckBuff(char *buf, int len);
	void StrToFloat(char ( &sv )[MAX_SVAL][MAX_SIMB], stArrPayval& dt);

	//--- Setting the Attributes of the serial port using termios structure 
	struct termios SerialPortSettings;
	struct termios oldSerialPortSettings;
};


// Объявление функции sendSQLToQuestDB
bool sendSQLToQuestDB(const std::string& query);
