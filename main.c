#include <wiringPi.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <mysql/mysql.h>
#include <wiringPi.h>
#include <wiringPiI2C.h>
#define MAXTIMINGS 85
#define DHTPIN 7 //Sensor Pin 7
#define GREENLED 2 //Pin 13
#define REDLED 3 // Pin 15
#define REDBUTTON 0 // Pin 11
#define GREENBUTTON 1 // Pin 12

int dht11_dat[5] = { 0, 0, 0, 0, 0 }; //Sensor
void insertSQLElement(char *humidity, char *tempreture);
void getSQLData(char *data, char *date1, char *date2);
void Interface();

// Change LCDAddr to current LCD address
// To find address connect the I2C LCD to Pi and type in the terminal i2cdetect -y 1
// use the number shown in the terminal and replace the number in LCDAddr after 0x
int LCDAddr = 0x27;
int BLEN = 1;
int fd;

void write_word(int data){
	int temp = data;
	if ( BLEN == 1 )
		temp |= 0x08;
	else
		temp &= 0xF7;
	wiringPiI2CWrite(fd, temp);
}

void send_command(int comm){
	int buf;
	// Send bit7-4 firstly
	buf = comm & 0xF0;
	buf |= 0x04;			// RS = 0, RW = 0, EN = 1
	write_word(buf);
	delay(2);
	buf &= 0xFB;			// Make EN = 0
	write_word(buf);

	// Send bit3-0 secondly
	buf = (comm & 0x0F) << 4;
	buf |= 0x04;			// RS = 0, RW = 0, EN = 1
	write_word(buf);
	delay(2);
	buf &= 0xFB;			// Make EN = 0
	write_word(buf);
}

void send_data(int data){
	int buf;
	// Send bit7-4 firstly
	buf = data & 0xF0;
	buf |= 0x05;			// RS = 1, RW = 0, EN = 1
	write_word(buf);
	delay(2);
	buf &= 0xFB;			// Make EN = 0
	write_word(buf);

	// Send bit3-0 secondly
	buf = (data & 0x0F) << 4;
	buf |= 0x05;			// RS = 1, RW = 0, EN = 1
	write_word(buf);
	delay(2);
	buf &= 0xFB;			// Make EN = 0
	write_word(buf);
}

void init(){
	send_command(0x33);	// Must initialize to 8-line mode at first
	delay(5);
	send_command(0x32);	// Then initialize to 4-line mode
	delay(5);
	send_command(0x28);	// 2 Lines & 5*7 dots
	delay(5);
	send_command(0x0C);	// Enable display without cursor
	delay(5);
	send_command(0x01);	// Clear Screen
	wiringPiI2CWrite(fd, 0x08);
}

void clear(){
	send_command(0x01);	//clear Screen
}

void write(int x, int y, char data[]){
	int addr, i;
	int tmp;
	if (x < 0)  x = 0;
	if (x > 15) x = 15;
	if (y < 0)  y = 0;
	if (y > 1)  y = 1;

	// Move cursor
	addr = 0x80 + 0x40 * y + x;
	send_command(addr);

	tmp = strlen(data);
	for (i = 0; i < tmp; i++){
		send_data(data[i]);
	}
}

//Read Sensors
void read_dht11_dat()
{
    uint8_t laststate = HIGH;
    uint8_t counter = 0;
    uint8_t j = 0, i;
    dht11_dat[0] = dht11_dat[1] = dht11_dat[2] = dht11_dat[3] = dht11_dat[4] = 0;
    pinMode( DHTPIN, OUTPUT );
    digitalWrite( DHTPIN, LOW );
    delay( 18 );
    digitalWrite( DHTPIN, HIGH );
    delayMicroseconds( 40 );
    pinMode( DHTPIN, INPUT );
    //reads the tempreture and humidity from sensors
    for ( i = 0; i < MAXTIMINGS; i++ )
    {
        counter = 0;
        while ( digitalRead( DHTPIN ) == laststate )
        {
            counter++;
            delayMicroseconds( 1 );
            if ( counter == 255 )
            {
                break;
            }
        }
        laststate = digitalRead( DHTPIN );
        if ( counter == 255 )
            break;
        if ( (i >= 4) && (i % 2 == 0) )
        {
            dht11_dat[j / 8] <<= 1;
            if ( counter > 16 )
                dht11_dat[j / 8] |= 1;
            j++;
        }
    }
    //Get tempreture and humidity from sensors if data is valid
    if ( (j >= 40) &&
            (dht11_dat[4] == ( (dht11_dat[0] + dht11_dat[1] + dht11_dat[2] + dht11_dat[3]) & 0xFF) ) )
    {
        //Convert tempreture and humidity into string

        char decimal = '.';
        char humidString[10] = {0};
        sprintf(humidString, "%.2d%c%.2d", dht11_dat[0], decimal, dht11_dat[1]);


        char tempString[10] = {0};
        sprintf(tempString, "%.2d%c%.2d", dht11_dat[2], decimal, dht11_dat[3]);

        // insert tempreture and humidity into SQL table
        insertSQLElement(&humidString, &tempString);


    }
}


//Send data of tempreture and humitity to the SQL server
void insertSQLElement(char *humidity, char *tempreture){
    char insertString[50];
    char comma = ',';
    // SQL command
    sprintf(insertString, "insert into data values (%s%c %s%c CURRENT_DATE)", humidity, comma, tempreture, comma);

    MYSQL *conn;
    MYSQL_RES *res;
    MYSQL_ROW row;
    // SQL user
    char *server = "localhost";
    char *user = "weather";
    char *password = "password";
    char *database = "weatherdb";

    conn = mysql_init(NULL);
    /* Connect to database */

    if (!mysql_real_connect(conn, server, user, password, database, 0, NULL, 0))
    {
        fprintf(stderr, "%s\n", mysql_error(conn));
        exit(1);
    }

    if (mysql_query(conn, insertString)){
        fprintf(stderr, "%s\n", mysql_error(conn));
        exit(1);
    }
    mysql_free_result(res);
    mysql_close(conn);
}

//gets the average, min, max, and median from SQL and prints them.
void getSQLData(char *data, char *date1, char *date2){
	fd = wiringPiI2CSetup(LCDAddr);
	init();
    MYSQL *conn;
    MYSQL_RES *res;
    MYSQL_ROW row;
    // SQL user
    char *server = "localhost";
    char *user = "weather";
    char *password = "password";
    char *database = "weatherdb";

    conn = mysql_init(NULL);
    /* Connect to database */

    if (!mysql_real_connect(conn, server, user, password, database, 0, NULL, 0))
    {
        fprintf(stderr, "%s\n", mysql_error(conn));
        exit(1);
    }
    // Get Max of data
    char MAXcommand[100] = {0};
    sprintf(MAXcommand, "SELECT MAX(%s) FROM data WHERE day BETWEEN '%s' AND '%s'",  data, date1, date2);
    if (mysql_query(conn, MAXcommand))
    {
        fprintf(stderr, "%s\n", mysql_error(conn));
        exit(1);
    }
    res = mysql_use_result(conn);

    //Display Max data
    char MaxDisplay[10] = {0};
    while ((row = mysql_fetch_row(res)) != NULL){
        if(strcmp(data, "Humid") == 0){
        sprintf(MaxDisplay, "H%.2s%%", row[0]);
        write(0, 0, MaxDisplay);
            }
        else{
        sprintf(MaxDisplay, "H%.4sC", row[0]);
        write(0, 0, MaxDisplay);
            }
        }

    //Get Median of data
    char MEDcommand[150] = {0};
    sprintf(MEDcommand, "select distinct percentile_cont(0.5) within group (order by %s) over () median from data where day between '%s' AND '%s'",  data, date1, date2);
    if (mysql_query(conn, MEDcommand))
    {
        fprintf(stderr, "%s\n", mysql_error(conn));
        exit(1);
    }
    res = mysql_use_result(conn);
    //Display Median data
    char MedDisplay[10] = {0};
    while ((row = mysql_fetch_row(res)) != NULL){
        if(strcmp(data, "Humid") == 0){
        sprintf(MedDisplay, "M%.2s%%", row[0]);
        write(8, 0, MedDisplay);
            }
        else{
        sprintf(MedDisplay, "M%.4sC", row[0]);
        write(8, 0, MedDisplay);
            }
    }

    //Get Minimum of data
    char MINcommand[100] = {0};
    sprintf(MINcommand, "SELECT MIN(%s) FROM data WHERE day BETWEEN '%s' AND '%s'",  data, date1, date2);

    if (mysql_query(conn, MINcommand))
    {
        fprintf(stderr, "%s\n", mysql_error(conn));
        exit(1);
    }

    //Display Minimum data
    res = mysql_use_result(conn);
    char MinDisplay[10] = {0};
    while ((row = mysql_fetch_row(res)) != NULL){
        if(strcmp(data, "Humid") == 0){
        sprintf(MinDisplay, "L%.2s%%", row[0]);
        write(0, 1, MinDisplay);
            }
        else{
        sprintf(MinDisplay, "L%.4sC", row[0]);
        write(0, 1, MinDisplay);
            }
        }

     //Get average of data
    char AVGcommand[100] = {0};
    sprintf(AVGcommand, "SELECT AVG(%s) FROM data WHERE day BETWEEN '%s' AND '%s'",  data, date1, date2);

    if (mysql_query(conn, AVGcommand))
    {
        fprintf(stderr, "%s\n", mysql_error(conn));
        exit(1);
    }

    //Display average data
    res = mysql_use_result(conn);
    char AvgDisplay[10] = {0};
    while ((row = mysql_fetch_row(res)) != NULL){
    if(strcmp(data, "Humid") == 0){
    sprintf(AvgDisplay, "Av%.2s%%", row[0]);
    write(8, 1, AvgDisplay);
            }
    else{
    sprintf(AvgDisplay, "Av%.4sC", row[0]);
    write(8, 1, AvgDisplay);
            }
    }

    mysql_free_result(res);
    mysql_close(conn);
}


void Interface(){
    printf( "\nReading data stopped\n" );
    int day, month, year;
    char startDate[20] = {0};
    char endDate[20] = {0};
    digitalWrite (REDLED, 1);
    digitalWrite (GREENLED, 0);

    printf("\nEnter start date (dd-mm-yyyy) : \n");
    scanf("%2d%*c%2d%*c%4d", &day, &month, &year);
    sprintf(startDate, "%d-%02d-%02d", year, month, day);

    printf("\nEnter end date (dd-mm-yyyy) : \n");
    scanf("%2d%*c%2d%*c%4d", &day, &month, &year);
    sprintf(endDate, "%d-%02d-%02d", year, month, day);

    while(digitalRead(REDBUTTON) != HIGH){
    getSQLData("Temp", startDate, endDate);
    delay(5000);
    getSQLData("Humid", startDate, endDate);
    delay(5000);
        }

    clear();
    digitalWrite (GREENLED, 1);
    digitalWrite (REDLED, 0);
    printf( "\nReading data \n" );
}

int main( void )
{
    printf( "Reading data\n" );
    if ( wiringPiSetup() == -1 ){
        exit( 1 );
    }
    pinMode (REDLED, OUTPUT) ;
    pinMode (GREENLED, OUTPUT) ;
    pinMode (REDBUTTON, INPUT);
    pinMode (GREENBUTTON, INPUT);
    digitalWrite (GREENLED, 1);
    digitalWrite (REDLED, 0);
    clear();

    while ( digitalRead(REDBUTTON) != HIGH )
    {
        if(digitalRead(GREENBUTTON) == HIGH){
        Interface();
        }
        read_dht11_dat();
        delay( 1000 );
    }
    digitalWrite (GREENLED, 0);
    digitalWrite (REDLED, 0);
    clear();
    return(0);
}

/************************************************************************************
* Project setup:                                                                    *
* 1. Launch terminal on rapberry pi and enter the following commands                *
* 2. sudo apt-get update                                                            *
* 3. cd /tmp                                                                        *
* 4. wget https://project-downloads.drogon.net/wiringpi-latest.deb                  *
* 5. sudo dpkg -i wiringpi-latest.deb                                               *
* 6. sudo apt-get install codeblocks                                                *                                                                                    
* 7. sudo apt-get install i2c-tools                                                 *
* 8. sudo apt-get install mariadb-server                                            *
* 9. sudo apt-get install default-libmysqlclient-dev                                *
* 10. Restart Pi                                                                    *
* 11. Check to see if latest version of gpio is installed with gpio –v              *
* 12. Create a folder to place the project files                                    *
* 13. Launch codeblock, set GNU GCC as default. Create a new project                *
* 14. Run new project to see if codeblock is running properly                       *
* 15. Open temperature and humidity project,                                        *
*            then go to Settings/Compiler/Link settings,                            *
*            link libraries to add /usr/lib/libwiringPi.so and                      *
*            /usr/lib/libwiringPiDev.so                                             *
* 16. Then Under Project/Build Options/Compiler settings/Other Compiler Options,    *
*            specify `mysql_config --cflags` (must use back ticks).                 *
* 17. Then under Project/Build Options/LInker settings/Other Linker Options,        *
*              specify `mysql_config --libs` (must use back ticks)                  *                                                      
* 18. Save all, then restart project.                                               *
* 19. In the terminal enter sudo raspi-config, Then arrow down and                  *
*             select "Advanced Settings" (or "Interfacing Options...")              *
* 20. Arrow down and select "I2C Enable/Disable automatic loading"                  *
* 21. Choose "Yes" at the next prompt, exit the configuration menu,                 *
*              and reboot the Pi to activate the settings                           *
* 22. With your LCD connected, enter i2cdetect -y 1 at the command prompt to see    *
*               a table of addresses for each I2C device connected to your Pi:      *
* 23. In the Temperature and Humidity project replace the LCDAddr variable after 0x *
*             with the address of the current LCD                                   *
* 24. In the terminal enter sudo mysql                                              *
* 25. create user ‘weather’@’localhost’ identified by ‘password’;                   *
* 26. select host, user from mysql.user;                                            *
* 27. grant all on *.* to ‘weather’@’localhost’;                                    *
* 28  exit;                                                                         *
* 29. mysql –u weather –p                                                           *
* 30. create database if not exists weatherdb;                                      *
* 31. use weatherdb;                                                                *
* 32. show weatherdb;                                                               *
* 33. drop table if exists data;                                                    *
* 34. create table data (Temp float, Humid float, day DATE);                        *
* 35. exit;                                                                         *
* 36. Launch codeblock and open the Temperature and humidiy project                 *
* 37. Open main.c and run it.                                                       *
*                                                                                   *
*************************************************************************************/
