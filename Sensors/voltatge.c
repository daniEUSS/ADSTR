/**
 *@brief Mesures de voltatge guardats a base de dades.
 *
 *@code
 *~$gcc voltatge.c -lsqlite3 -o volt
 *~$./temp
 *@endcode
 *
 *@file temperatura.c
 *@author Daniel Gonzalez
 *@version 1.1
 *@date 24.10.2020
 *
 *@param Guardem a base de dades Mesures.db el voltatge amb la data i hora de la lectura presa.
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

#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>
#include <linux/i2c-dev.h>
#include <string.h>
#include <time.h> 
#include <sqlite3.h>

int verbose = 1;

#define DEV_LED 0x54
#define DEV_PATH "/dev/i2c-1"
#define SHTDWN_REG 0x00
#define CTRL_REG1 0x13
#define CTRL_REG2 0x14
#define CTRL_REG3 0x15
#define PWM_UPDATE_REG 0x16
#define RST_REG 0x17
static char *cntdevice = "/dev/spidev0.0";

//ADC configurations segons manual MCP3008
#define SINGLE_ENDED_CH0 8
#define SINGLE_ENDED_CH1 9
#define SINGLE_ENDED_CH2 10
#define SINGLE_ENDED_CH3 11
#define SINGLE_ENDED_CH4 12
#define SINGLE_ENDED_CH5 13
#define SINGLE_ENDED_CH6 14
#define SINGLE_ENDED_CH7 15
#define DIFERENTIAL_CH0_CH1 0 //Chanel CH0 = IN+ CH1 = IN-
#define DIFERENTIAL_CH1_CH0 1 //Chanel CH0 = IN- CH1 = IN+
#define DIFERENTIAL_CH2_CH3 2 //Chanel CH2 = IN+ CH3 = IN-
#define DIFERENTIAL_CH3_CH2 3 //Chanel CH2 = IN- CH3 = IN+
#define DIFERENTIAL_CH4_CH5 4 //Chanel CH4 = IN+ CH5 = IN-
#define DIFERENTIAL_CH5_CH4 5 //Chanel CH4 = IN- CH5 = IN+
#define DIFERENTIAL_CH6_CH7 6 //Chanel CH6 = IN+ CH7 = IN-
#define DIFERENTIAL_CH7_CH6 7 //Chanel CH6 = IN- CH7 = IN+

// -----------------------------------------------------------------------------------------------

static void pabort(const char *s)
{
	perror(s);
	abort();
}

// -----------------------------------------------------------------------------------------------

static void spiadc_config_tx( int conf, uint8_t tx[3] )
{
	int i;

	uint8_t tx_dac[3] = { 0x01, 0x00, 0x00 };
	uint8_t n_tx_dac = 3;
	
	for (i=0; i < n_tx_dac; i++) {
		tx[i] = tx_dac[i];
	}
	
// Estableix el mode de comunicació en la parta alta del 2n byte
	tx[1]=conf<<4;
	
	if( verbose ) {
		for (i=0; i < n_tx_dac; i++) {
			printf("spi tx dac byte:(%02d)=0x%02x\n",i,tx[i] );
		}
	}		
}

// -----------------------------------------------------------------------------------------------
static int spiadc_transfer(int fd, uint8_t bits, uint32_t speed, uint16_t delay, uint8_t tx[3], uint8_t *rx, int len )
{
	int ret, value, i;

	struct spi_ioc_transfer tr = {
		.tx_buf = (unsigned long)tx,
		.rx_buf = (unsigned long)rx,
		.len = len*sizeof(uint8_t),
		.delay_usecs = delay,
		.speed_hz = speed,
		.bits_per_word = bits,
	};

	ret = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);

	if( verbose ) {

		for (i = 0; i < len; i++) {
			printf("0x%02x ", rx[i]);
		}
		value = ((rx[1] & 0x0F) << 8) + rx[2];
		printf("-->  %d\n", value);
	
	}

	return ret;

}



// -----------------------------------------------------------------------------------------------

static int spiadc_config_transfer( int conf, int *value )
{
	int ret = 0;
	int fd;
	uint8_t rx[3];
	char buffer[255];
	
	/* SPI parameters */
	char *device = cntdevice;
	//uint8_t mode = SPI_CPOL; //No va bé amb aquesta configuació, ha de ser CPHA
	uint8_t mode = SPI_CPHA;
	uint8_t bits = 8;
	uint32_t speed = 500000; //max 1500KHz
	uint16_t delay = 0;
	
	/* Transmission buffer */
	uint8_t tx[3];

	/* open device */
	fd = open(device, O_RDWR);
	if (fd < 0) {
		sprintf( buffer, "can't open device (%s)", device );
		pabort( buffer );
	}

	/* spi mode */
	ret = ioctl(fd, SPI_IOC_WR_MODE, &mode);
	if (ret == -1)
		pabort("can't set spi mode");
	ret = ioctl(fd, SPI_IOC_RD_MODE, &mode);
	if (ret == -1)
		pabort("can't get spi mode");

	/* bits per word 	 */
	ret = ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
	if (ret == -1)
		pabort("can't set bits per word");
	ret = ioctl(fd, SPI_IOC_RD_BITS_PER_WORD, &bits);
	if (ret == -1)
		pabort("can't get bits per word");

	/* max speed hz  */
	ret = ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
	if (ret == -1)
		pabort("can't set max speed hz");
	ret = ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ, &speed);
	if (ret == -1)
		pabort("can't get max speed hz");

	/* build data to transfer */
	spiadc_config_tx( conf, tx );
		
	/* spi adc transfer */
	ret = spiadc_transfer( fd, bits, speed, delay, tx, rx, 3 );
	if (ret == 1)
		pabort("can't send spi message");

	close(fd);

	*value = ((rx[1] & 0x03) << 8) + rx[2];

	return ret;
}

// -----------------------------------------------------------------------------------------------


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

int llums (float intensitat, int fdl)
{
    //int fdl = 0;
    int intens;
    
    intens = abs((intensitat - 3.3) * 256 / 3.3);
    if (intens<3){intens=0;}

    /* SET the PWM value for LEDs */
    i2c_smbus_write_byte_data(fdl, 0x01, intens);
    i2c_smbus_write_byte_data(fdl, 0x02, intens);   
	i2c_smbus_write_byte_data(fdl, 0x03, intens);
	i2c_smbus_write_byte_data(fdl, 0x04, intens);    
	i2c_smbus_write_byte_data(fdl, 0x05, intens);
	i2c_smbus_write_byte_data(fdl, 0x06, intens);
	i2c_smbus_write_byte_data(fdl, 0x0f, intens);    
	i2c_smbus_write_byte_data(fdl, 0x0e, intens);
	i2c_smbus_write_byte_data(fdl, 0x0d, intens);
	i2c_smbus_write_byte_data(fdl, 0x07, intens);
	i2c_smbus_write_byte_data(fdl, 0x08, intens);    
	i2c_smbus_write_byte_data(fdl, 0x09, intens);
	i2c_smbus_write_byte_data(fdl, 0x0a, intens);
	i2c_smbus_write_byte_data(fdl, 0x0b, intens);
	i2c_smbus_write_byte_data(fdl, 0x0c, intens);
	
	/* Values stored in a temporary register */
	
	/* Update values of registers*/
	i2c_smbus_write_byte_data(fdl, PWM_UPDATE_REG, 0x00); //write any value
	
	return(0);
	
}

void delay(int number_of_seconds) 
{ 
    // Converting time into milli_seconds 
    int milli_seconds = 1000 * number_of_seconds; 
  
    // Storing start time 
    clock_t start_time = clock(); 
  
    // looping till required time is not achieved 
    while (clock() < start_time + milli_seconds) 
        ; 
}




int main(int argc, char *argv[])
{
	sqlite3 *db;
    char *err_msg = 0;
	int ret = 0, value_int;
	float value_volts;
	int fd = 0;
		
	if ((fd = open(DEV_PATH, O_RDWR)) < 0) {
		perror("Unable to open i2c device");
		exit(1);
    }

    /* configure i2c slave */
    if (ioctl(fd, I2C_SLAVE, DEV_LED) < 0) {
		perror("Unable to configure i2c slave device");
		close(fd);
		exit(1);
	}
	/* Power down the device (clean start) */
    i2c_smbus_write_byte_data(fd, RST_REG, 0x00);

    /* Turn on the device in normal operation  */
    i2c_smbus_write_byte_data(fd, SHTDWN_REG, 0x01);

    /* Activate LEDs 1-F */
    i2c_smbus_write_byte_data(fd, CTRL_REG1, 0x3F);
    i2c_smbus_write_byte_data(fd, CTRL_REG2, 0x3F);
    i2c_smbus_write_byte_data(fd, CTRL_REG3, 0x3F);
	
	
	while(1){
		ret = spiadc_config_transfer( SINGLE_ENDED_CH0, &value_int );

		printf("valor llegit (0-1023) %d\n", value_int);
		value_volts=3.3*value_int/1023;
	
		printf("voltatge %.3f V\n", value_volts);
		delay(1000);
//--------------------------------------------------------------------------------------------------------------------	
		int rc = sqlite3_open("/home/pi/Desktop/ADSTR/Sensors/Mesures.db", &db);
    
		if (rc != SQLITE_OK) {
        
			fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
			sqlite3_close(db);
        
			return 1;
		}
		/*char sql [2056] = "DROP TABLE IF EXISTS Voltatges;" 
                "CREATE TABLE Voltatges(Id INTEGER PRIMARY KEY AUTOINCREMENT , Sensor TEXT, Voltatge FLOAT, Temps TEXT);" ;
                //"CREATE TABLE Voltatges(Id INTEGER PRIMARY KEY AUTOINCREMENT, Nom TEXT, Temperatura FLOAT, Temps TIMESTAMP DEFAULT CURRENT_TIMESTAMP);" ;*/
		char texto [2056];
		snprintf(texto, sizeof(texto), "INSERT INTO Voltatges (Sensor,Valor, Temps) VALUES('MCP3008', %.4f, DateTime('now'));", value_volts);
     
		rc = sqlite3_exec(db, texto, callback, 0, &err_msg);
    
		if (rc != SQLITE_OK ) {
        
			fprintf(stderr, "SQL error: %s\n", err_msg);
			sqlite3_free(err_msg);        
			sqlite3_close(db);
        
			return 1;
		} 
		
		llums(value_volts, fd);
    
		sqlite3_close(db);
	}
	return ret;
}
