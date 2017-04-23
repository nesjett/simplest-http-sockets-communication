/*
 ** server.c -- Ejemplo de servidor de sockets de flujo
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>

// CUSTOM INCLUDES
#include "colors.h"
#include "config_manager.h"
#include "constants.h"
#include "log_manager.h"
#include "header_manager.h"




struct params p;

void sigchld_handler(int s)
{
    while(wait(NULL) > 0);
}

void processHTTP_REQUEST(int sd, struct sockaddr_in their_addr)
{
    	char buf[MAXDATASIZE];
   	char *comando, *recurso, *protocol;
    	char archivo[256];
    	int fd;
    	int bLeidos;
				
	if ((read(sd, buf, MAXDATASIZE)) == -1) {
		perror("recv--");
		return;
	}

	
	comando = strtok(buf, " ");
	recurso = strtok(NULL, " ");
	protocol = strtok(NULL, "\r\n");

printf("%s", p.DOCUMENT_ROOT);
	sprintf(archivo, "./%s%s", p.DOCUMENT_ROOT, recurso); // add a "." for security reasons (prevent people from accessing system folders)

	
	// check if the protocol asked by client is valid on this server (HTTP/1.1 or HTTP/1.0)
	if( is_valid_protocol(protocol) == false ){
		if(p.DEBUG == 1){
			time_t t = time(NULL);
			struct tm tm = *localtime(&t);
			printf(ANSI_COLOR_RED "[%d:%d:%d] Server: got connection for unsupported protocol from %s -> " ANSI_COLOR_BLUE "asking for %s" ANSI_COLOR_RESET "\n", tm.tm_hour, tm.tm_min, tm.tm_sec, inet_ntoa(their_addr.sin_addr), archivo);
		}
		
		log_write_error_registry("Unsupported protocol requested");
		close(fd);
		exit(0);
	}

	

	

	


	/*
	printf(" %s\n", strtok(buf, " "));
	if( strcmp("GET", strtok(buf, " ")) != 0 ){
		perror("no GET request found");
		exit(1);
	}
	*/


	//procesar archivo
	fd = open(archivo, O_RDONLY);
	
	if(fd==-1)
	{
		// file not found
		time_t t = time(NULL);
		struct tm tm = *localtime(&t);
		printf(ANSI_COLOR_GREEN "[%d:%d:%d] Server: got connection from %s -> " ANSI_COLOR_YELLOW "file not found %s" ANSI_COLOR_RESET "\n", tm.tm_hour, tm.tm_min, tm.tm_sec, inet_ntoa(their_addr.sin_addr), archivo);
		exit(0);
	} 
	

	char *header = get_header();
	sprintf(buf, "%s", header);
	free(header); // free the memory allocated for the header string
	write(sd, buf, strlen(buf));

	while (bLeidos=read(fd, buf, sizeof(buf))>0){
		write(sd, buf, strlen(buf));
	}

	close(fd);


	if(p.DEBUG == 1){
		time_t t = time(NULL);
		struct tm tm = *localtime(&t);
		printf(ANSI_COLOR_GREEN "[%d:%d:%d] Server: got connection from %s -> " ANSI_COLOR_BLUE "serving %s" ANSI_COLOR_RESET "\n", tm.tm_hour, tm.tm_min, tm.tm_sec, inet_ntoa(their_addr.sin_addr), archivo);
		
		sprintf(buf, "%s \n", archivo); // DEBUG
		write(sd, buf, strlen(buf)); // DEBUG
	}


	// log the connection to the server
	log_write_access_registry(inet_ntoa(their_addr.sin_addr), recurso, "200"); // TODO: Use the right status code, not just 200
}


void init_server_configuration(int argc, char *argv[]){
	p = read_config(CONFIG_FILE);

	// TODO: we could check for manual entered parameters on the server launch. ex: ./mi_http port=8888
}
			


int main(int argc, char *argv[])
{
	int sockfd, new_fd;  // Escuchar sobre sock_fd, nuevas conexiones sobre new_fd
	struct sockaddr_in my_addr;    // información sobre mi dirección
	struct sockaddr_in their_addr; // información sobre la dirección del cliente
	int sin_size;
	struct sigaction sa;
	int yes=1;

	/**** init server config ****/
	init_server_configuration(argc, argv);

        
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
	printf(ANSI_COLOR_RED "Error creating the socket" ANSI_COLOR_RESET "\n");
	log_write_error_registry("Error creating the socket");
        exit(1);
    }
    
    if (setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(int)) == -1) {
	printf(ANSI_COLOR_RED "Error in setting sockopt (setsockopt)" ANSI_COLOR_RESET "\n");
	log_write_error_registry("Error in setting sockopt (setsockopt)");
        exit(1);
    }
    
    my_addr.sin_family = AF_INET;         // Ordenación de bytes de la máquina
    my_addr.sin_port = htons(p.LISTEN_PORT);     // short, Ordenación de bytes de la red
    my_addr.sin_addr.s_addr = INADDR_ANY; // Rellenar con mi dirección IP
    memset(&(my_addr.sin_zero), '\0', 8); // Poner a cero el resto de la estructura
    
    if (bind(sockfd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr))
        == -1) {
	printf(ANSI_COLOR_RED "Error binding the socket" ANSI_COLOR_RESET "\n");
	log_write_error_registry("Error binding the socket");
        exit(1);
    }
    
    if (listen(sockfd, p.MAX_CLIENTS) == -1) {
	printf(ANSI_COLOR_RED "Error listening on the socket" ANSI_COLOR_RESET "\n");
	log_write_error_registry("Error listening on the socket");
        exit(1);
    }
    
    sa.sa_handler = sigchld_handler; // Eliminar procesos muertos
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
	printf(ANSI_COLOR_RED "SigAction error" ANSI_COLOR_RESET "\n");
        exit(1);
    }
    

    /******* ACTUALLY ACCEPT CONNECTIONS CODE ********/
    while(1) {  // main accept() loop
        sin_size = sizeof(struct sockaddr_in);
        if ((new_fd = accept(sockfd, (struct sockaddr *)&their_addr, (socklen_t*)&sin_size)) == -1) {
		printf(ANSI_COLOR_RED "Error accepting connection" ANSI_COLOR_RESET "\n");
		log_write_error_registry("Error accepting connection");
            continue;
        }else{
		if (!fork()) { // Child proccess
			close(sockfd); // Children doesnt needs descriptor
			processHTTP_REQUEST(new_fd, their_addr);				
			close(new_fd);
			exit(0);
		}
		
        }
        close(new_fd);  // El proceso padre no lo necesita
    }
    
    return 0;
}
