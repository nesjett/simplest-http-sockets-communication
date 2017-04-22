#include "config_reader.h"  /* Include the header (not strictly necessary here) */

int foo(int x)    /* Function definition */
{
    return x + 5;
}


struct params read_config(char file[256]) {
	struct params t;
	char buf[512];
	char *command;
	FILE *fd;

	fd = fopen(file, "r"); // open file
	
	while (fgets(buf, sizeof buf, fd) != NULL) {
	    // process line here
		command = strtok(buf, " ");
		if(command != NULL && strcmp("#", command) != 0){

			// INT PARAMS
			if( strcmp("DEBUG", command) == 0 ){
				t.DEBUG = atoi(strtok(NULL, " "));
				continue;
			}


			if( strcmp("LISTEN_PORT", command) == 0 ){
				t.LISTEN_PORT = atoi(strtok(NULL, " "));
				continue;
			}

			if( strcmp("MAX_CLIENTS", command) == 0 ){
				t.MAX_CLIENTS = atoi(strtok(NULL, " "));
				continue;
			}


			// STRING PARAMS
			if( strcmp("DIRECTORY_INDEX", command) == 0 ){
				t.DIRECTORY_INDEX = strtok(NULL, " ");
				// TODO: the next printf work here, but not in the print_config_params(), which shows unespected string
				//printf("DIRECTORY_INDEX: %s\n", t.DIRECTORY_INDEX);
				continue;
			}

			if( strcmp("SECURITY_FILE", command) == 0 ){
				t.SECURITY_FILE = strtok(NULL, " ");
				continue;
			}

			if( strcmp("DOCUMENT_ROOT", command) == 0 ){
				t.DOCUMENT_ROOT = strtok(NULL, " ");
				continue;
			}


		}
	}

	fclose(fd); // close file

	if(t.DEBUG == 1)
		print_config_params(t);

	return t;
}

void print_config_params(struct params p){
	printf("DEBUG: %d\n", p.DEBUG);
	printf("LISTEN_PORT: %d\n", p.LISTEN_PORT);
	printf("MAX_CLIENTS: %d\n", p.MAX_CLIENTS);
	printf("DIRECTORY_INDEX: %s\n", p.DIRECTORY_INDEX);
	printf("SECURITY_FILE: %s\n", p.SECURITY_FILE);
	printf("DOCUMENT_ROOT: %s\n", p.DOCUMENT_ROOT);
}
















