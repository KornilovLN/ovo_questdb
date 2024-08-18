#include "comm.h"
#include <curl/curl.h>
#include <string>
#include <sstream>
#include <iostream>

const char* URL = "http://gitlab.ivl.ua:9000/exec";

clPort::clPort(){}
clPort::~clPort(){}

//--- Setting the Attributes of the serial port using termios structure
void clPort::Open(){
	fd = open(PORT, O_RDWR | O_NOCTTY);
	            /* ttyUSB? is the FT232 based USB2SERIAL Converter   */
				/* O_RDWR   - Read/Write access to serial port       */
				/* O_NOCTTY - No terminal will control the process   */
				/* Open in blocking mode,read will wait              */

        if(fd < 0) {		/* Error Checking */
		printf("\n  Error! in Opening %s", PORT);
		exit(0);
	}
	printf("\n  %s Opened Successfully", PORT);
}


void clPort::SetAttributes(){
	tcgetattr(fd, &oldSerialPortSettings);	 //--- Get the current attributes of the Serial port
    SerialPortSettings = oldSerialPortSettings;

	//--- Setting the Baud rate -----------------------------------------------------------------------
	cfsetispeed(&SerialPortSettings,B19200); //--- Set Read  Speed as 19200                      
	cfsetospeed(&SerialPortSettings,B19200); //--- Set Write Speed as 19200                      

	//--- 8N1 Mode ------------------------------------------------------------------------------------
	SerialPortSettings.c_cflag &= ~PARENB;   //--- Disables the Parity Enable bit(PARENB),So No Parity   
	SerialPortSettings.c_cflag &= ~CSTOPB;   //--- CSTOPB = 2 Stop bits,here it is cleared so 1 Stop bit 
	SerialPortSettings.c_cflag &= ~CSIZE;	 //--- Clears the mask for setting the data size             
	SerialPortSettings.c_cflag |=  CS8;      //--- Set the data bits = 8                                 

	SerialPortSettings.c_cflag &= ~CRTSCTS;       //--- No Hardware flow Control                         
	SerialPortSettings.c_cflag |= CREAD | CLOCAL; //--- Enable receiver,Ignore Modem Control lines       

	SerialPortSettings.c_iflag &= ~(IXON | IXOFF | IXANY);          //--- Disable XON/XOFF flow control both i/p and o/p
	SerialPortSettings.c_iflag &= ~(ICANON | ECHO | ECHOE | ISIG);  //--- Non Cannonical mode            

	SerialPortSettings.c_oflag &= ~OPOST;    //--- No Output Processing

	//--- Setting Time outs ---------------------------------------------------------------------------
	SerialPortSettings.c_cc[VMIN] = 40;      //--- Read at least 10 characters 
	SerialPortSettings.c_cc[VTIME] = 10;     //--- Wait indefinetly   

	//--- Set the attributes to the termios structure -------------------------------------------------
	if((tcsetattr(fd,TCSANOW,&SerialPortSettings)) != 0)
	    printf("\n  ERROR ! in Setting attributes");
	else			
        printf("\n  BaudRate = 19200 \n  data bits = 8 \n StopBits = 1 \n  Parity   = none");
}

//-----------------------------------------------------------------------------------------------------
//--- Функция отправки SQL-запросов в QuestDB ------------------------------------------------------------
bool sendSQLToQuestDB(const std::string& query) {
    CURL *curl;
    CURLcode res;
    bool success = false;

    // Инициализация libcurl
    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    if (curl) {
        std::string postData = "query=" + query;

        // Установка URL для отправки данных
        curl_easy_setopt(curl, CURLOPT_URL, URL);

        // Установка данных для отправки
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postData.c_str());

        // Выполнение запроса
        res = curl_easy_perform(curl);

        // Проверка на ошибки
        if (res == CURLE_OK) {
            success = true;
        } else {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        }

        // Очистка
        curl_easy_cleanup(curl);
    }

    curl_global_cleanup();
    return success;
}

//--- Функция создания таблицы в QuestDB с проверкой на существование -------------------------------------
void createTableIfNotExists() {
    CURL *curl;
    CURLcode res;
    //bool success = false;

    // Инициализация libcurl
    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    if (curl) {
        std::string createTableQuery = "CREATE TABLE IF NOT EXISTS sensors ("
                                       "timestamp TIMESTAMP, "
                                       "PWM_Ten DOUBLE, "
                                       "PWM_Led DOUBLE, "
                                       "PWM_Vent DOUBLE, "
                                       "DHT_Temperature DOUBLE, "
                                       "DHT_Humidity DOUBLE, "
                                       "DHT_Light DOUBLE, "
                                       "DS_Temperature1 DOUBLE, "
                                       "DS_Temperature2 DOUBLE, "
                                       "DS_Temperature3 DOUBLE, "
                                       "BME_Temperature DOUBLE, "
                                       "BME_Humidity DOUBLE, "
                                       "BME_Pressure DOUBLE, "
                                       "Port INT, "
                                       "Alarm INT)";

        std::string url = std::string(URL) + "?query=" + curl_easy_escape(curl, createTableQuery.c_str(), createTableQuery.length());

        // Установка URL для отправки данных
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

        // Выполнение запроса
        res = curl_easy_perform(curl);

        // Проверка на ошибки
        if (res == CURLE_OK) {
            //success = true;
            printf("Table 'sensors' created or already exists.\n");
        } else {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
            printf("Failed to create table 'sensors'.\n");
        }

        // Очистка
        curl_easy_cleanup(curl);
    }

    curl_global_cleanup();
}
//-----------------------------------------------------------------------------------------------------


//--- Функция отправки данных в БД questdb ------------------------------------------------------------
void sendDataToQuestDB(const stArrPayval& data, const stPortVal& port) {
    CURL *curl;
    CURLcode res;

    // Инициализация libcurl
    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    if (curl) {
        // Формирование данных в формате Line Protocol
        std::ostringstream oss;
        oss << "sensors,location=room1 "
            << "PWM_Ten=" << data.PWM.f0 << ","
            << "PWM_Led=" << data.PWM.f1 << ","
            << "PWM_Vent=" << data.PWM.f2 << ","
            << "DHT_Temperature=" << data.DhtL.f0 << ","
            << "DHT_Humidity=" << data.DhtL.f1 << ","
            << "DHT_Light=" << data.DhtL.f2 << ","
            << "DS_Temperature1=" << data.Ds.f0 << ","
            << "DS_Temperature2=" << data.Ds.f1 << ","
            << "DS_Temperature3=" << data.Ds.f2 << ","
            << "BME_Temperature=" << data.Bme.f0 << ","
            << "BME_Humidity=" << data.Bme.f1 << ","
            << "BME_Pressure=" << data.Bme.f2 << ","
            << "Port=" << port.prt << ","
            << "Alarm=" << port.alarm;

        std::string postData = oss.str();

        // Отладочное сообщение
        std::cout << "Отправка данных: " << postData << std::endl;

        // Установка URL для отправки данных
        std::string url = std::string(URL) + "?query=" + curl_easy_escape(curl, postData.c_str(), postData.length());
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

        // Выполнение запроса
        res = curl_easy_perform(curl);

        // Проверка на ошибки
        if (res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        } else {
            std::cout << "Данные успешно отправлены." << std::endl;
        }

        // Очистка
        curl_easy_cleanup(curl);
    }

    curl_global_cleanup();
}
//-----------------------------------------------------------------------------------------------------


void clPort::Close(){
    //--- Обязательно вернуть настройки и Close the serial port
    tcsetattr(fd,TCSANOW,&oldSerialPortSettings);
	close(fd); /* Close the serial port */
}

//--- Работа в цикле --------------------------------------------------------------------------------------
//--- Данные по ссылке будут наполняться из порта
//---------------------------------------------------------------------------------------------------------
void clPort::Work(stArrPayval& dt, stPortVal& port){
	int i;
	int  bytes_read = 0;        			//--- Number of bytes read by the read() system call 

    int stat = 0;
    int cnt = 0;
    int ind = 0;
	char sval[MAX_SVAL][MAX_SIMB] = {0,}; 

    //char res[LENBUFFER]; 

    // Вызов функции создания таблицы
    createTableIfNotExists();

    int dig=1;

	while (1) {

		tcflush(fd, TCIFLUSH);			//--- Discards old data in the rx buffer  

		bytes_read = read(fd,&read_buffer,LENPRIHOD);	//--- Read the data   
                
        if(EN_ADDINFO > 0){
			printf("Bytes Rxed: %d\n", bytes_read);		//--- Print the number of bytes read 
		}

		for(i=0; i<bytes_read; i++){				//---printing only the received characters
			unsigned char c = read_buffer[i];

		switch(stat){
		case 0: //--- с самого начала ищем разделитель строк
			if(c == '\n' || c == '\r'){
                        	cnt = 0;
                        	ind = 0;
				stat = 1;
            }
            break;

	        case 1: 
	        	if(c == ' '){
					//--- пропускаем пробелы и переходим к наполнению след буфера						
					ind = 0;
					cnt++;

				}else if(c == '\n' || c == '\r'){
                   	//--- Проверить, был ли приход и печатать его, если да. 
                   	//--- Иначе - выводит пустой массив
	     			if(CheckBuff(sval[0], MAX_SIMB) == 1){
						if(dig != 0){	//--- dig==1		                        
							//--- преобразуем к числам то, что в буферах	
                        	port.prt = atoi( sval[0] );
                        	port.alarm = atoi( sval[13] );
							StrToFloat(sval, dt);
#ifdef EN_PRINT                                  
	                   		printf("%4d %-3.2f %-3.2f %-3.2f %-3.2f %-3.2f %-3.2f %-3.2f %-3.2f %-3.2f %-3.2f %-3.2f %-3.2f %4d\n",
                            	port.prt, 
								dt.PWM.f0,  dt.PWM.f1, dt.PWM.f2, 
								dt.DhtL.f0, dt.DhtL.f1,dt.DhtL.f2, 
								dt.Ds.f0,   dt.Ds.f1,  dt.Ds.f2,
								dt.Bme.f0,  dt.Bme.f1, dt.Bme.f2,
								port.alarm);

                            //--- Отправка данных в QuestDB ---
                            sendDataToQuestDB(dt, port);  		
                            //---------------------------------					
#endif
						}else{		//--- dig==0
#ifdef EN_PRINT
        	            	//--- в конец добавить '0' - конец строки
							sprintf(res, "%s %s %s %s %s %s %s %s %s %s %s %s %s %s%c",
						    	sval[0],
						    	sval[1], sval[2], sval[3],
						    	sval[4], sval[5], sval[6],
						    	sval[7], sval[8], sval[9],
						    	sval[10],sval[11],sval[12],
						    	sval[13],
						    	0);
        	            	//--- puts выводит строку до символа '0' и добавляет символ '\n'
   							puts(res);
#endif
						}
					}

                	cnt = 0;
                	ind = 0;
                	memset(sval,0,MAX_SVAL*MAX_SIMB);
					stat = 1;

				}else{	//--- наполняем циферками буферы
					sval[cnt][ind++] = c;
				}
			
				break;
			
			}	
		}
	}
}



void clPort::StrToFloat(char ( &sv )[MAX_SVAL][MAX_SIMB], stArrPayval& dt){
	dt.PWM.f0 = atof(sv[1]);
	dt.PWM.f1 = atof(sv[2]);
	dt.PWM.f2 = atof(sv[3]);

	dt.DhtL.f0 = atof(sv[4]);
	dt.DhtL.f1 = atof(sv[5]);
	dt.DhtL.f2 = atof(sv[6]); //--- Light

	dt.Ds.f0 = atof(sv[7]);
	dt.Ds.f1 = atof(sv[8]);
	dt.Ds.f2 = atof(sv[9]);

	dt.Bme.f0 = atof(sv[10]);
	dt.Bme.f1 = atof(sv[11]);
	dt.Bme.f2 = atof(sv[12]);
}

int clPort::CheckBuff(char *buf, int len){
    //int cnt = 0;
	for(int i = 0; i < len; i++){
		if(buf[i] != 0)
        	return 1;			
	}
    return 0;
}




