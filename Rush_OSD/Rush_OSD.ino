
// This software is a copy of the original Rushduino-OSD project was written by Jean-Gabriel Maurice.
// http://code.google.com/p/rushduino-osd/

// First of all I would like to thank those who labor Rushduino-OSD project before me.
// It's open source, simple and seems easy editable.
// I have the original since released.
// Rushduino_V9 beta 0.7 software was the last one that the autor coded and some problems of compatibility with MWC.
// Rushduino project has been forgotten for a long time. 
// Modelci started to make some arrangements without having the original hardware, it is a bit dificult to know the compatibility of the code.
// Traiblazer coded new and smoother AH.
// After all this I decided to keep it working for future releases of MWC with or without help, I both this Rushduino OSD and pretend to use it as much as other users.
// For now I will share my developments on Multiwii forum http://www.multiwii.com/forum/viewtopic.php?f=8&t=922
// In near future I pretend to have my one site dedicated to Aerial Drones/Photography and FPV stuff related. I will share it. 
// As I am not a coder professionaly, developments are going to be slower but I intend to keep them going with the new needs to come. 
// All the things that I possibly do with this software are intended to be my needs I just hope that they can meet yours.
// I wish you great flights with Rushduino OSD.
/***********************************************************************************************************************************************/
/*                                                          Created for Multiwii r1240                                                         */
/***********************************************************************************************************************************************/
// This software communicates using MSP via the serial port. Therefore Multiwii develop-dependent.
// Changes the values ​​of pid and rc-tuning, writes in eeprom of Multiwii FC.
// In config mode, can do acc and mag calibration. 
// In addition, it works by collecting information analogue inputs. Such as voltage, amperage, rssi, temperature.
// In addition displayed information provides status information using an LED and a buzzer.
// At the end of the flight may be useful to look at the statistics.

/***********************************************************************************************************************************************/
/*                                                           RUSH_KV_0.1 Kataventos                                                           */
/***********************************************************************************************************************************************/



#include <avr/pgmspace.h>
#include <EEPROM.h> //Needed to access eeprom read/write functions
#include <Metro.h>
#include "Config.h"
#include "GlobalVariables.h"

// Screen is the Screen buffer between program an MAX7456 that will be writen to the screen at 10hz
char screen[480];

// ScreenBuffer is an intermietary buffer to created Strings to send to Screen buffer
char screenBuffer[20];


char nextMSPrequest=0;
char MSPcmdsend=0;

#if defined(BUZZER)
static uint8_t  toggleBeep = 0;
static uint8_t  buzzerFreq;         // delay between buzzer ring
#endif


void setup()
{
  Serial.begin(SERIAL_SPEED);
  Serial.flush();
  pinMode(BST,OUTPUT);
  BUZZERPIN_PINMODE;
  MAX7456Setup();
  readEEPROM();
  analogReference(INTERNAL);

}


void loop()
{
  // Process AI
  temperature=(analogRead(temperaturePin)*1.1)/10.23;  
  voltage=(analogRead(voltagePin)*1.1*DIVIDERRATIO)/102.3; 
  rssiADC = (analogRead(rssiPin)*1.1)/1023;
  amperage = (analogRead(amperagePin)*1.1)/10.23;

  // Blink Basic Sanity Test Led at 1hz
  if(tenthSec>10) BST_ON else BST_OFF 

    if(MetroTimer.check()==1)  // this execute 20 times per second
  {
    tenthSec++;
    halfSec++;

    Blink10hz=!Blink10hz;

    calculateTrip();

    calculateRssi();

    MetroTimer.interval(TIMEBASE);
    if(!serialWait)
    {


      nextMSPrequest++;
      switch (nextMSPrequest) {
      case 1:
        MSPcmdsend=MSP_IDENT;
        break;
      
      case 2:
        MSPcmdsend=MSP_ATTITUDE;
        break;

      case 3:
        MSPcmdsend=MSP_STATUS;
        break;

      case 4:
        MSPcmdsend=MSP_ATTITUDE;
        break;
        
      case 5:
        MSPcmdsend=MSP_RAW_IMU;
        break;

      case 6:
        MSPcmdsend=MSP_ATTITUDE;
        break;
        
      case 7:
        MSPcmdsend=MSP_RAW_GPS;
        break;

      case 8:
        MSPcmdsend=MSP_ATTITUDE;
        break;
        
      case 9:
        MSPcmdsend=MSP_COMP_GPS;
        break;

      case 10:
        MSPcmdsend=MSP_ATTITUDE;
        break;

      case 11:
        MSPcmdsend=MSP_ALTITUDE;
        break;
        
      case 12:
        MSPcmdsend=MSP_ATTITUDE;
        break;
        
      case 13:
        MSPcmdsend=MSP_RC_TUNING;
        break;

      case 14:
        MSPcmdsend=MSP_ATTITUDE;
        break;
        
      case 15:
        MSPcmdsend=MSP_PID;
        break;
        
      case 16:
        MSPcmdsend=MSP_ATTITUDE;
        break;
      
      case 17:
        MSPcmdsend=MSP_BAT;
        break;
   
        case 201:
        MSPcmdsend=MSP_STATUS;
        break;

      case 202:
        MSPcmdsend=MSP_RAW_IMU;
        break;

      case 203:
        MSPcmdsend=MSP_ATTITUDE;
        break;

      default:
        MSPcmdsend=MSP_RC;
        if(askPID==0) 
        {
          nextMSPrequest = 0;
        }
        else
        {
          nextMSPrequest = 200;
        }
        break;
      }    

      blankserialRequest(MSPcmdsend);

    }



    MAX7456_DrawScreen(screen,0);
    if( allSec < 9 ) displayIntro();
    else
    { 

      if(configMode) 
      {
        displayPIDConfigScreen();

      }
      else
      {
        if(enableVoltage&&((voltage>lowVoltage)||(Blink2hz))) displayVoltage();
#if defined(BUZZER)
        if ((voltage<lowVoltage)&&(enableBuzzer)&&enableVoltage) {
          buzzerFreq = 1;
          buzzer(buzzerFreq); 
        }
#endif

        if(enableRSSI&&((rssi>lowrssiAlarm)||(Blink2hz))) displayRSSI();

        displayTime();
        displaySensors();
        displayMode();

        if(enableTemperature&&((temperature<highTemperature)||(Blink2hz))) displayTemperature();
#if defined(BUZZER)
        if ((temperature>highTemperature)&&(enableBuzzer)&&enableTemperature) {
          buzzerFreq = 2;
          buzzer(buzzerFreq); 
        }
#endif

#if defined(AMPERAGE)
        displayAmperage();
#endif

        displaypMeterSum();
        displayArmed();

#if defined(BUZZER)
        if ((armedTimer>armedtimeWarning)&&(armedtimeWarning>0)&&(enableBuzzer)){
          buzzerFreq = 0;
          buzzer(buzzerFreq); // external buzzer routine that handles buzzer events globally now
        }
#endif

        if(MwSensorPresent&ACCELEROMETER) displayHorizon(MwAngle[0],MwAngle[1]*-1);
        if(MwSensorPresent&MAGNETOMETER)  {
          displayHeadingGraph();
          displayHeading();
        }
        if(MwSensorPresent&BAROMETER)     {
          displayAltitude();
          displayClimbRate();
        }
        if(MwSensorPresent&GPSSENSOR)     {
          displayNumberOfSat();
          displayDirectionToHome();
          displayDistanceToHome();

          displayAngleToHome();

          displaySpeed();

          if (displayGPS) displayGPSPosition();

        }
      }
    }
  } 

  if(halfSec>=10){
    halfSec=0;
    Blink2hz=!Blink2hz;
    if(waitStick) waitStick=waitStick-1;
  }

  if(tenthSec>=20)     // this execute 1 time a second
  {
    onSecond++;
    
    amperageConsumed += amperage / 36; //(mAh)
    tenthSec=0;
    armedTimer++;



    if(!armed) {
      armedTimer=0;
      flyMinute=0;
      flySecond=0;
    } 
    else {
      flySecond++;
      configMode=0;
    }
    allSec++;  

    if((accCalibrationTimer==1)&&(configMode)) {
      MSPcmdsend = MSP_ACC_CALIBRATION;
      blankserialRequest(MSPcmdsend);
      accCalibrationTimer=0;
    }

    if((magCalibrationTimer==1)&&(configMode)) {
      MSPcmdsend = MSP_MAG_CALIBRATION;
      blankserialRequest(MSPcmdsend);
      magCalibrationTimer=0;
    }

    if((eepromWriteTimer==1)&&(configMode)) {
      MSPcmdsend = MSP_EEPROM_WRITE;
      blankserialRequest(MSPcmdsend);
      eepromWriteTimer=0;
    }

    if(accCalibrationTimer>0) accCalibrationTimer--;
    if(magCalibrationTimer>0) magCalibrationTimer--;
    if(eepromWriteTimer>0) eepromWriteTimer--;

    if((rssiTimer==1)&&(configMode)) {
      rssiMin=rssiADC;
      rssiTimer=0; 
    }
    if(rssiTimer>0) rssiTimer--;
  }     

  if(onSecond>=60)    // this execute each minute
  {  
    onMinute++;
    onSecond=0;
  }
  if(flySecond>=60)    // this execute each minute
  {  
    flyMinute++;
    flySecond=0;
  }
  serialMSPreceive();
}

void calculateTrip(void)
{
  if(GPS_fix && (GPS_speed>0)) trip += ((GPS_speed * 1000)/3600)*0.1;
}

void calculateRssi(void)
{
  float aa=0;
  aa =  analogRead(rssiPin)/4;  
  aa = ((aa-rssiMin) *101)/(rssiMax-rssiMin) ;       
  rssi_Int += ( ( (signed int)((aa*rssiSample) - rssi_Int )) / rssiSample );
  rssi = rssi_Int / rssiSample ;
  if(rssi<0) rssi=0;
  if(rssi>100) rssi=100;
}

void writeEEPROM(void)
{
  EEPROM.write(EEPROM_RSSIMIN,rssiMin);
  EEPROM.write(EEPROM_RSSIMAX,rssiMax);
  EEPROM.write(EEPROM_DISPLAYRSSI,enableRSSI);
  EEPROM.write(EEPROM_DISPLAYVOLTAGE,enableVoltage);
  EEPROM.write(EEPROM_VOLTAGEMIN,lowVoltage);
  EEPROM.write(EEPROM_DISPLAYTEMPERATURE,enableTemperature);
  EEPROM.write(EEPROM_TEMPERATUREMAX,highTemperature);
  EEPROM.write(EEPROM_DISPLAYGPS,displayGPS);
  EEPROM.write(EEPROM_ENABLEBUZZER,enableBuzzer);
  EEPROM.write(EEPROM_SCREENTYPE,screenType);
  EEPROM.write(EEPROM_UNITSYSTEM,unitSystem);
  EEPROM.write(EEPROM_ARMEDTIMEWARNING,armedtimeWarning);

}

void readEEPROM(void)
{
  rssiMin= EEPROM.read(EEPROM_RSSIMIN);
  rssiMax= EEPROM.read(EEPROM_RSSIMAX);
  enableRSSI= EEPROM.read(EEPROM_DISPLAYRSSI);
  enableVoltage= EEPROM.read(EEPROM_DISPLAYVOLTAGE);
  lowVoltage= EEPROM.read(EEPROM_VOLTAGEMIN);
  enableTemperature= EEPROM.read(EEPROM_DISPLAYTEMPERATURE);
  highTemperature= EEPROM.read(EEPROM_TEMPERATUREMAX);
  displayGPS= EEPROM.read(EEPROM_DISPLAYGPS);
  enableBuzzer= EEPROM.read(EEPROM_ENABLEBUZZER);
  screenType= EEPROM.read(EEPROM_SCREENTYPE);
  unitSystem= EEPROM.read(EEPROM_UNITSYSTEM);
  armedtimeWarning= EEPROM.read(EEPROM_ARMEDTIMEWARNING);
}






