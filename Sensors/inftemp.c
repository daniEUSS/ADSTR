/**
 * @brief Programa de lectura d'una base de dades
 * 
 * Programa que llegeix la informació d'una base de dades implementada amb SQLite i mostra per
 * pantalla els valors llegits.
 * 
 * @code
 * ~$ gcc inftemp.c -o inftemp
 * ~$ ./inftemp
 * @endcode
 * 
 * El programa llegeix l'arxiu .db de la base de dades ja existent i mostra per pantalla la
 * informació continguda en diferents columnes.
 * 
 * @file inftemp.c
 * @author Toni Vives Cabaleiro
 * @version Fita4
 * @date 03/11/2020
 * 
 * @param - No n'hi ha
 * @return 0 Si està ok
 * 
 * @todo Pròximes fites
 * 
 * @section LICENSE
 * This program is free software; you can redistribute it and/or modify  *
 * it under the terms of the GNU General Public License as published by  *
 * the Free Software Foundation; either version 2 of the License, or     *
 * (at your option) any later version.         
 */

#include <stdio.h>
#include <string.h>
#include <sqlite3.h>

FILE * fp;

char valors[1024];

static int callback(void *buffer, int argc, char **argv, char**azColName) {
	int i;
	
	for(i = 0; i < argc; i++){
		printf("%s = %s\n", azColName[i], argv[i] ? argv[i] :"NULL");
		strcpy(valors, argv[i]);
		
	}
	
	printf("\n");
	
	return 0;
}

int main (int argc, char **argv) {
	sqlite3 *db;
	char *zErrMsg = 0;
	char buffer[1024];
	char text[1024];
	char sql[1024];
	memset(buffer, '\0', 1024);
	int rc;
	
	rc = sqlite3_open("/home/pi/Desktop/ADSTR/Sensors/Mesures.db", &db);
	if( rc ) {
		fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
		sqlite3_close(db);
		return(1);
	}

	fp = fopen("/home/pi/Desktop/ADSTR/Sensors/Temperatures.txt", "w");
	if (fp == NULL) {
		printf("Can't open the file.\n");
	}

    //Data i hora inicial
    sprintf(sql, "SELECT MIN(Temps) FROM Temperatures %s", buffer);
    sqlite3_exec(db, sql, callback, (void *)buffer, &zErrMsg);
    sprintf(text, "Data i hora inici lectura: %s\n", valors) ;
	fprintf(fp, "%s", text);
    memset(text, '\0', sizeof(text));
 
    //Data i hora final
    sprintf(sql, "SELECT MAX(Temps) FROM Temperatures %s", buffer);
    sqlite3_exec(db, sql, callback, (void *)buffer, &zErrMsg);
    sprintf(text, "Data i hora final lectura: %s\n", valors);
    fprintf(fp, "%s", text);
    memset(text, '\0', sizeof(text));

    //Valor màxim
    sprintf(sql, "SELECT MAX(Valor) FROM Temperatures %s", buffer);
    sqlite3_exec(db, sql, callback, (void *)buffer, &zErrMsg);
    sprintf(text, "Valor màxim lectura: %s\n", valors);
    fprintf(fp, "%s", text);
    memset(text, '\0', sizeof(text));
    
    //Valor mínim
    sprintf(sql, "SELECT MIN(Valor) FROM Temperatures %s", buffer);
    sqlite3_exec(db, sql, callback, (void *)buffer, &zErrMsg);
    sprintf(text, "Valor mínim lectura: %s\n", valors);
    fprintf(fp, "%s", text);
    memset(text, '\0', sizeof(text));
    
    //Valor mig
    sprintf(sql, "SELECT AVG(Valor) FROM Temperatures %s", buffer);
    sqlite3_exec(db, sql, callback, (void*)buffer, &zErrMsg);
    sprintf(text, "Valor mig lectura: %s\n", valors);
    fprintf(fp, "%s", text);
    memset(text, '\0', sizeof(text));


/*	rc = sqlite3_exec(db, argv[2], callback, 0, &zErrMsg);
	if( rc!=SQLITE_OK ) {
		fprintf(stderr, "SQL error: %s\n", zErrMsg);
		sqlite3_free(zErrMsg);
	}*/
	
	fclose(fp);
	sqlite3_close(db);
	
	return 0;
}
