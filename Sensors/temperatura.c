/**
 *@brief Mesures d'un sensor de temperatura guardat a base de dades.
 *
 *@code
 *~$gcc voltatge.c -Wall -lsqlite3 -o volt
 *~$./temp
 *@endcode
 *
 *@file voltatge.c
 *@author Daniel Gonzalez
 *@version 1.1
 *@date 24.10.2020
 *
 *@param Guardem a base de dades Mesures.db la temperatura amb la data i hora de la lectura presa.
 *@return 0 si ok
 *
 *@todo Següent Fita.
 *
 *@section LICENSE
 *License GNU/GPL, see COPYING
  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 **/


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <linux/i2c-dev.h>
#include <stdlib.h>
#include <fcntl.h>
#include <time.h> 
#include <sqlite3.h>
#include <string.h>

#define DEV_ID 0x48
#define DEV_LED 0x54
#define DEV_PATH "/dev/i2c-1"
#define CNF_REG 0x01
#define CNV_REG 0x00
#define SHTDWN_REG 0x00
#define CTRL_REG1 0x13
#define CTRL_REG2 0x14
#define CTRL_REG3 0x15
#define PWM_UPDATE_REG 0x16
#define RST_REG 0x17

#define EXPORT "/sys/class/gpio/export"
#define UNEXPORT "sys/class/gpio/unexport"

#define REDLED "/sys/class/gpio/gpio13"
#define YELLOWLED "/sys/class/gpio/gpio19"
#define GREENLED "/sys/class/gpio/gpio16"



void led_on(char addr[])
{
	int fdr;
	char m[] = "1";
	
	fdr = open(addr, O_WRONLY);
	if(fdr<0) {perror("Error a l'obrir el dispositiu\n"); exit(-1);}
	
	write (fdr,m,1);
	close(fdr);
}
void led_off(char addr[])
{
	int fdr;
	char m[] = "0";
	
	fdr = open(addr, O_WRONLY);
	if(fdr<0) {perror("Error a l'obrir el dispositiu\n"); exit(-1);}
	
	write (fdr,m,1);
	close(fdr);
}

static int callback(void *NotUsed, int argc, char **argv, char **azColName)
{
	int i;
	for(i=0; i<argc; i++)
	{
		printf("%s = %s\n", azColName[i], argv[i] ? argv[i]: "NULL");
	}
	printf("\n");
	return(0);
}
int cridasql(float valor_temp)
{
	sqlite3 *db;
    char *zErrMsg = 0;
    int rc;

    rc = sqlite3_open("/home/pi/Desktop/ADSTR/Sensors/Mesures.db", &db);
    if (rc != SQLITE_OK) {    
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);       
        return 1;
    }
    char texto [2056];
    snprintf(texto, sizeof(texto), "INSERT INTO Temperatures(Sensor,Valor, Temps) VALUES('LM35', %.2f, CURRENT_TIMESTAMP);", valor_temp*4700/6);
    rc = sqlite3_exec(db, texto, callback, 0, &zErrMsg);
 
    if (rc != SQLITE_OK ) {       
        
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        
        sqlite3_free(zErrMsg);        
        sqlite3_close(db);
        
        return 1;
    }
    sqlite3_close(db);
    return(0);
}

void delay(int number_of_seconds) 
{ 
    // Converting time into milli_seconds 
    int milli_seconds = 1000 * number_of_seconds; 
  
    // Storing start time 
    clock_t start_time = clock(); 
  
    // looping till required time is not achieved 
    while (clock() < start_time + milli_seconds); 
}


int main(int argc, char **argv)
{
    int fd = 0;
    int fdl = 0;
    uint8_t adc_l=0, adc_h=0;
    uint16_t adc_out = 0;
    int adc_ok=0;
    double adc_v = 0.0;
 //   int led = 1;

    /* open i2c comms */
    if ((fd = open(DEV_PATH, O_RDWR)) < 0) {
	perror("Unable to open i2c device");
	exit(1);
    }

    /* configure i2c slave */
    if (ioctl(fd, I2C_SLAVE, DEV_ID) < 0) {
	perror("Unable to configure i2c slave device");
	close(fd);
	exit(1);
    }
    
    if ((fdl = open(DEV_PATH, O_RDWR)) < 0) {
	perror("Unable to open i2c device");
	exit(1);
    }

    /* configure i2c slave */
    if (ioctl(fdl, I2C_SLAVE, DEV_LED) < 0) {
	perror("Unable to configure i2c slave device");
	close(fdl);
	exit(1);
    }
while(1){
   
    /* Run one-shot measurement (AIN1-gnd), FSR +-4.096V, 160 SPS, 
     * no windows, no comparation */
    // LowSByte MSByte  they are inverted
    i2c_smbus_write_word_data(fd, CNF_REG, 0x83D3);
	printf("memoria=%d",0x83D3);
    /* Wait until the measurement is complete */
	usleep(700);		/* 700 microseconds */
	adc_out = i2c_smbus_read_word_data(fd, CNV_REG);
	// swap bytes
	adc_l=adc_out >> 8;
	adc_h=adc_out;
	adc_ok=(adc_h<<8)|adc_l;
	// dichart 4 LSB
	adc_ok=(adc_ok>>4);
	
    printf("Value ADC = %d \n",adc_out);
    printf("Value ADC = %x \n",adc_out);
    printf("Value L   = %x \n",adc_l);
    printf("Value H   = %x \n",adc_h);
    printf("Value OK  = %x \n",adc_ok);
    
    /* calculate output values */
    adc_v = 4.095 * (adc_ok / 2047.0);

    /*output */
    printf("Value ADC in V = %.2fV\n", adc_v);
    printf("Value input in V = %.2fV\n", adc_v*47/6);
    printf("Value degrees(ºC) = %.2fºC\n", adc_v*4700/6);
    
    cridasql(adc_v);
    
    
    /* Power down the device (clean start) */
    i2c_smbus_write_byte_data(fdl, RST_REG, 0x00);

    /* Turn on the device in normal operation  */
    i2c_smbus_write_byte_data(fdl, SHTDWN_REG, 0x01);

    /* Activate LEDs 1-3 */
    i2c_smbus_write_byte_data(fdl, CTRL_REG1, 0x3F);
    i2c_smbus_write_byte_data(fdl, CTRL_REG2, 0x3F);
    i2c_smbus_write_byte_data(fdl, CTRL_REG3, 0x3F);

	i2c_smbus_write_byte_data(fdl, 0x01, 0x10);

    // SET the PWM value for LEDs
 /*   if(led==1){i2c_smbus_write_byte_data(fdl, 0x01, 0x01);}
    if(led==2){i2c_smbus_write_byte_data(fdl, 0x01, 0x10);    
				i2c_smbus_write_byte_data(fdl, 0x02, 0x01);
				//led_on(REDLED);
				}
	if(led==3){i2c_smbus_write_byte_data(fdl, 0x01, 0xff);    
				i2c_smbus_write_byte_data(fdl, 0x02, 0x10);
				i2c_smbus_write_byte_data(fdl, 0x03, 0x01);
				//led_off(REDLED);led_on(GREENLED);
				}
	if(led==4){i2c_smbus_write_byte_data(fdl, 0x01, 0x10);    
				i2c_smbus_write_byte_data(fdl, 0x02, 0xff);
				i2c_smbus_write_byte_data(fdl, 0x03, 0x10);
				i2c_smbus_write_byte_data(fdl, 0x04, 0x01);
				//led_on(YELLOWLED);led_off(GREENLED);
				}
	if(led==5){i2c_smbus_write_byte_data(fdl, 0x01, 0x01);    
				i2c_smbus_write_byte_data(fdl, 0x02, 0x10);
				i2c_smbus_write_byte_data(fdl, 0x03, 0xff);
				i2c_smbus_write_byte_data(fdl, 0x04, 0x10);
				i2c_smbus_write_byte_data(fdl, 0x05, 0x01);
				//led_off(YELLOWLED);
				}
	if(led==6){i2c_smbus_write_byte_data(fdl, 0x01, 0x00);    
				i2c_smbus_write_byte_data(fdl, 0x02, 0x01);
				i2c_smbus_write_byte_data(fdl, 0x03, 0x10);
				i2c_smbus_write_byte_data(fdl, 0x04, 0xff);
				i2c_smbus_write_byte_data(fdl, 0x05, 0x10);
				i2c_smbus_write_byte_data(fdl, 0x06, 0x01);}
	if(led==7){i2c_smbus_write_byte_data(fdl, 0x02, 0x00);    
				i2c_smbus_write_byte_data(fdl, 0x03, 0x01);
				i2c_smbus_write_byte_data(fdl, 0x04, 0x10);
				i2c_smbus_write_byte_data(fdl, 0x05, 0xff);
				i2c_smbus_write_byte_data(fdl, 0x06, 0x10);
				i2c_smbus_write_byte_data(fdl, 0x07, 0x01);}
	if(led==8){i2c_smbus_write_byte_data(fdl, 0x03, 0x00);    
				i2c_smbus_write_byte_data(fdl, 0x04, 0x01);
				i2c_smbus_write_byte_data(fdl, 0x05, 0x10);
				i2c_smbus_write_byte_data(fdl, 0x06, 0xff);
				i2c_smbus_write_byte_data(fdl, 0x0f, 0x10);
				i2c_smbus_write_byte_data(fdl, 0x0e, 0x01);}
	if(led==9){i2c_smbus_write_byte_data(fdl, 0x04, 0x00);    
				i2c_smbus_write_byte_data(fdl, 0x05, 0x01);
				i2c_smbus_write_byte_data(fdl, 0x06, 0x10);
				i2c_smbus_write_byte_data(fdl, 0x0f, 0xff);
				i2c_smbus_write_byte_data(fdl, 0x0e, 0x10);
				i2c_smbus_write_byte_data(fdl, 0x0d, 0x01);}
	if(led==10){i2c_smbus_write_byte_data(fdl, 0x05, 0x00);    
				i2c_smbus_write_byte_data(fdl, 0x06, 0x01);
				i2c_smbus_write_byte_data(fdl, 0x0f, 0x10);
				i2c_smbus_write_byte_data(fdl, 0x0e, 0xff);
				i2c_smbus_write_byte_data(fdl, 0x0d, 0x10);
				i2c_smbus_write_byte_data(fdl, 0x07, 0x01);}
	if(led==11){i2c_smbus_write_byte_data(fdl, 0x06, 0x00);    
				i2c_smbus_write_byte_data(fdl, 0x0f, 0x01);
				i2c_smbus_write_byte_data(fdl, 0x0e, 0x10);
				i2c_smbus_write_byte_data(fdl, 0x0d, 0xff);
				i2c_smbus_write_byte_data(fdl, 0x07, 0x10);
				i2c_smbus_write_byte_data(fdl, 0x08, 0x01);}
	if(led==12){i2c_smbus_write_byte_data(fdl, 0x0f, 0x00);    
				i2c_smbus_write_byte_data(fdl, 0x0e, 0x01);
				i2c_smbus_write_byte_data(fdl, 0x0d, 0x10);
				i2c_smbus_write_byte_data(fdl, 0x07, 0xff);
				i2c_smbus_write_byte_data(fdl, 0x08, 0x10);
				i2c_smbus_write_byte_data(fdl, 0x09, 0x01);}
	if(led==13){i2c_smbus_write_byte_data(fdl, 0x0e, 0x00);    
				i2c_smbus_write_byte_data(fdl, 0x0d, 0x01);
				i2c_smbus_write_byte_data(fdl, 0x07, 0x10);
				i2c_smbus_write_byte_data(fdl, 0x08, 0xff);
				i2c_smbus_write_byte_data(fdl, 0x09, 0x10);
				i2c_smbus_write_byte_data(fdl, 0x0a, 0x01);}				
	if(led==14){i2c_smbus_write_byte_data(fdl, 0x0d, 0x00);    
				i2c_smbus_write_byte_data(fdl, 0x07, 0x01);
				i2c_smbus_write_byte_data(fdl, 0x08, 0x10);
				i2c_smbus_write_byte_data(fdl, 0x09, 0xff);
				i2c_smbus_write_byte_data(fdl, 0x0a, 0x10);
				i2c_smbus_write_byte_data(fdl, 0x0b, 0x01);}				
	if(led==15){i2c_smbus_write_byte_data(fdl, 0x07, 0x00);    
				i2c_smbus_write_byte_data(fdl, 0x08, 0x01);
				i2c_smbus_write_byte_data(fdl, 0x09, 0x10);
				i2c_smbus_write_byte_data(fdl, 0x0a, 0xff);
				i2c_smbus_write_byte_data(fdl, 0x0b, 0x10);
				i2c_smbus_write_byte_data(fdl, 0x0c, 0x01);}
	if(led==16){i2c_smbus_write_byte_data(fdl, 0x08, 0x00);
				i2c_smbus_write_byte_data(fdl, 0x09, 0x01);
				i2c_smbus_write_byte_data(fdl, 0x0a, 0x10);
				i2c_smbus_write_byte_data(fdl, 0x0b, 0xff);
				i2c_smbus_write_byte_data(fdl, 0x0c, 0x10);}
	if(led==17){i2c_smbus_write_byte_data(fdl, 0x09, 0x00);
				i2c_smbus_write_byte_data(fdl, 0x0a, 0x01);
				i2c_smbus_write_byte_data(fdl, 0x0b, 0x10);
				i2c_smbus_write_byte_data(fdl, 0x0c, 0xff);}
	if(led==18){i2c_smbus_write_byte_data(fdl, 0x0a, 0x00);
				i2c_smbus_write_byte_data(fdl, 0x0b, 0x01);
				i2c_smbus_write_byte_data(fdl, 0x0c, 0x10);}
	if(led==19){i2c_smbus_write_byte_data(fdl, 0x0b, 0x00);
				i2c_smbus_write_byte_data(fdl, 0x0c, 0x01);}
	if(led==20){i2c_smbus_write_byte_data(fdl, 0x10, 0xff);
				i2c_smbus_write_byte_data(fdl, 0x11, 0xff);
				i2c_smbus_write_byte_data(fdl, 0x12, 0xff);}*/


	/* Values stored in a temporary register */
	
	/* Update values of registers*/
	i2c_smbus_write_byte_data(fdl, PWM_UPDATE_REG, 0x00); //write any value

	delay(100);
/*	if (led < 20)
	{
		led ++;
	}
	else
	{
		
		led = 1;*/
		i2c_smbus_write_byte_data(fdl, RST_REG, 0x00);
		i2c_smbus_write_byte_data(fdl, SHTDWN_REG, 0x00);
//	}
	delay (1000);
		
}
	close(fdl);
    close(fd);

    return (0);
}
