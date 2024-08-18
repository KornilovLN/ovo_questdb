#ifndef __TERM_EXAMPLE__
#define __TERM_EXAMPLE__

//--- Includes for Display section
#include <signal.h>
#include <sys/ioctl.h>

//--- Common
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>



//--- Includes for Comm port section
#include <fcntl.h>   //--- File Control Definitions           
#include <termios.h> //--- POSIX Terminal Control Definitions 
#include <unistd.h>  //--- UNIX Standard Definitions 	   
#include <errno.h>   //--- ERROR Number Definitions     

//--- Display section ---------------------------------------------- 

#define ESC "\033"

//Format text
#define RESET 		0
#define BRIGHT 		1
#define DIM			  2
#define UNDERSCORE	3
#define BLINK		  4
#define REVERSE		5
#define HIDDEN		6

//Foreground Colours (text)
#define F_BLACK 	30
#define F_RED		  31
#define F_GREEN		32
#define F_YELLOW	33
#define F_BLUE		34
#define F_MAGENTA 35
#define F_CYAN		36
#define F_WHITE		37

//Background Colours
#define B_BLACK 	40
#define B_RED		  41
#define B_GREEN		42
#define B_YELLOW	44
#define B_BLUE		44
#define B_MAGENTA 45
#define B_CYAN		46
#define B_WHITE		47


/*ANSI/VT100 Terminal using example */

#define home() 			 printf(ESC "[H") 	//Move cursor to the indic row, clmn (origin at 1,1)
#define clrscr()		 printf(ESC "[2J") 	//clear the screen, move to (1,1)
#define gotoxy(x,y)		 printf(ESC "[%d;%dH", y, x);
#define visible_cursor() printf(ESC "[?251");
/*
Set Display Attribute Mode	<ESC>[{attr1};...;{attrn}m
*/
#define resetcolor()                printf(ESC "[0m")
#define set_display_atrib(color) 	printf(ESC "[%dm",color)

#define DISPLAY_BACKGROUND  B_BLACK
#define DISPLAY_BRIGHT      BRIGHT

#define SETDISPLAYATTRIBS    set_display_atrib(DISPLAY_BACKGROUND);\
                             set_display_atrib(DISPLAY_BRIGHT);\
                             int display_atrib = F_RED;\
                             set_display_atrib(display_atrib);

#define PAUSE_LEN   100000

//--- ANSI/VT100 Terminal----------------------------------
/*
0123456789012345678901234567890123456789012345678901234567890123456
*/
#define	WIN "\
┌────────────────────────────────────────────────────────────────────────┐\n\ 
│                                                                        │\n\
│┌──────────┐┌──────────────────────────────────────────────────────────┐│\n\
││ Day:     ││ Etap:                                                    ││\n\
│└──────────┘└──────────────────────────────────────────────────────────┘│\n\
│  Alarm     Technologia      PWM(%)           Box            Room       │\n\
│┌────────┐┌────────────┐┌──────────────┐┌──────────────┐┌──────────────┐│\n\
││DS1.t[ ]││ Etap       ││ Ten          ││ D1.t         ││ BME.t        ││\n\
││DHT.t[ ]││ Day        ││ Led          ││ DHT.t        ││ BME.h        ││\n\
││led  [ ]││ Rot        ││              ││ DHT.h        ││ BME.p        ││\n\
││vent [ ]││            ││ Vent         ││ Light        ││ D2.t         ││\n\
│└────────┘└────────────┘└──────────────┘└──────────────┘└──────────────┘│\n\
└────────────────────────────────────────────────────────────────────────┘\n\
┌─────────────────────────────────────────────────┐┌─────────────────────┐\n\
│ Mess:                                           ││ D3.t str.           │\n\
└─────────────────────────────────────────────────┘└─────────────────────┘"   


//--- Comm port section -------------------------------------------- 

typedef struct {
    uint16_t X;
    uint16_t Y;
}stFrameXY;

#endif /*__TERM_EXAMPLE__*/
