# Temperature-Humidity-Server
Uses Raspberry Pi and temperature and humidity sensors to send and retrieve data from MariaDB server using C.

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
