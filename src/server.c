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
#include <errno.h>

// CUSTOM INCLUDES
#include "colors.h"
#include "config_manager.h"
#include "constants.h"
#include "log_manager.h"
#include "header_manager.h"




struct params p = {
	DEFAULT_DEBUG,
	DEFAULT_LISTEN_PORT,
	DEFAULT_MAX_CLIENTS, 
	DEFAULT_DIRECTORY_INDEX,
	DEFAULT_SECURITY_FILE,
	DEFAULT_DOCUMENT_ROOT,
	DEFAULT_LOG_FOLDER
}; // init defaults

void sigchld_handler(int s)
{
    while(wait(NULL) > 0);
}

void write_HEADER(int sd, char *resource, int request_status, int size){
	char buf[MAXDATASIZE];
	// PROCESS HEADER & BODY
	char *header = get_header(resource, request_status, size);
	sprintf(buf, "%s", header);
	free(header); // free the memory allocated for the header string
	write(sd, buf, strlen(buf));
}

void proccess_GET(int sd, struct sockaddr_in their_addr){

}


void processHTTP_REQUEST(int sd, struct sockaddr_in their_addr)
{
    	char buf[MAXDATASIZE];
   	char *comando, *resource, *protocol;
    	char archivo[256], archivo_error[256];
    	int fd = -1;
    	int bLeidos;
	int bodyLength = 0;

	int request_status = 0;
				
	if ((read(sd, buf, MAXDATASIZE)) == -1) {
		perror("recv--");
		return;
	}

	
	comando = strtok(buf, " ");
	resource = strtok(NULL, " ");
	protocol = strtok(NULL, "\r\n");


	// check if the protocol asked by client is valid on this server (HTTP/1.1 or HTTP/1.0)
	if( is_valid_protocol(protocol) == false ){
		request_status = 505; // HTTP version not supported

		if(p.DEBUG == 1){
			time_t t = time(NULL);
			struct tm tm = *localtime(&t);
			printf(ANSI_COLOR_RED "[%d:%d:%d] Server: got connection for unsupported protocol from %s -> " ANSI_COLOR_BLUE "asking for %s" ANSI_COLOR_RESET "\n", tm.tm_hour, tm.tm_min, tm.tm_sec, inet_ntoa(their_addr.sin_addr), resource);
		}
		
		log_write_error_registry("Unsupported protocol requested");

		//exit(0);
	}

	
	if( is_valid_command(comando) == false && request_status == 0){
		request_status = 405; // Method not allowed


		if(p.DEBUG == 1){
			time_t t = time(NULL);
			struct tm tm = *localtime(&t);
			printf(ANSI_COLOR_RED "[%d:%d:%d] Server: got connection for unsupported command from %s -> " ANSI_COLOR_BLUE "asking for %s" ANSI_COLOR_RESET "\n", tm.tm_hour, tm.tm_min, tm.tm_sec, inet_ntoa(their_addr.sin_addr), resource);
		}
		
		log_write_error_registry("Unsupported command requested");



		// get body for error message
		sprintf(archivo_error, "../%s%d.html", DEFAULT_ERROR_RESPONSES_FOLDER, request_status);
		fd = open(archivo_error, O_RDONLY);

		if(fd==-1){
			strcpy(buf , "<html><head></head><body>Error file not found, please, do not delete /default_responses/ folder nor its content</body></html>");
			bodyLength = strlen(buf);
		}else{
			while (bLeidos=read(fd, buf, sizeof(buf))>0){
				bodyLength += strlen(buf);
			}

			close(fd);
		}


		// PROCESS HEADER & BODY
		write_HEADER(sd, resource, request_status, bodyLength);

	
		// TODO: Should be done with fopen, fread, so we could rewind() to read the file again, or just seek to the final of the file and get its size
		// get body for error message
		fd = open(archivo_error, O_RDONLY);

		if(fd==-1){
			strcpy(buf , "<html><head></head><body>Error file not found, please, do not delete /default_responses/ folder nor its content</body></html>");
			write(sd, buf, strlen(buf));
		}else{
			while (bLeidos=read(fd, buf, sizeof(buf))>0){
				write(sd, buf, strlen(buf));
			}

			close(fd);
		}
	
		


		// log the connection to the server
		log_write_access_registry(inet_ntoa(their_addr.sin_addr), archivo, request_status);
		exit(0);
	}
	
	

	if(request_status == 0){

		if( strcmp("GET", comando) == 0 ){


			/******
			*
			*  get resource from URL or from DIRECTORY_INDEX
			*
			*******/
			if( strcmp("", resource) == 0 || strcmp("/", resource) == 0 || strcmp(" ", resource) == 0 ){
			
				resource = strtok(p.DIRECTORY_INDEX, ",");
				if( resource != NULL ){
					// try to open the first index
					sprintf(archivo, "../%s/%s", p.DOCUMENT_ROOT, resource); // add another slash between /%s/s% because if url specifies a resource, it will be /file.html but if its empty, there is no slash
					fd = open(archivo, O_RDONLY);
		
					if(fd == -1){
						while( resource != NULL && fd == -1 ){
							resource = strtok(NULL, ",");
							sprintf(archivo, "../%s/%s", p.DOCUMENT_ROOT, resource); // add another slash between /%s/s% because if url specifies a resource, it will be /file.html but if its empty, there is no slash
							fd = open(archivo, O_RDONLY);
						}
					}
				}

			
			}else{
				sprintf(archivo, "../%s%s", p.DOCUMENT_ROOT, resource); // add a ".." for security reasons (prevent people from accessing system folders)
				//procesar archivo
				fd = open(archivo, O_RDONLY);
			}



			/******
			*
			*  process request body
			*
			*******/
			if(fd==-1)
			{		
				if(errno == 2) //no such file or directory
					request_status = 404; // FILE NOT FOUND 404
				if(errno == 13) // permision denied
					request_status = 403; // FORBRIDDEN 403


				if(p.DEBUG == 1){
					// file not found
					time_t t = time(NULL);
					struct tm tm = *localtime(&t);
					printf(ANSI_COLOR_GREEN "[%d:%d:%d] GET: got connection from %s -> " ANSI_COLOR_YELLOW "file not found %s" ANSI_COLOR_RESET "\n", tm.tm_hour, tm.tm_min, tm.tm_sec, inet_ntoa(their_addr.sin_addr), archivo);
				}

			}else{
				request_status = 200; // 200 OK


				if(p.DEBUG == 1){
					time_t t = time(NULL);
					struct tm tm = *localtime(&t);
					printf(ANSI_COLOR_GREEN "[%d:%d:%d] GET: got connection from %s -> " ANSI_COLOR_BLUE "serving %s" ANSI_COLOR_RESET "\n", tm.tm_hour, tm.tm_min, tm.tm_sec, inet_ntoa(their_addr.sin_addr), archivo);
				}
			}

			
			/******
			*
			*  contar el body length para la cabecera
			*
			*******/
			if(request_status == 200){
				while (bLeidos=read(fd, buf, sizeof(buf))>0){
					bodyLength += strlen(buf);
				}
				close(fd);
				// do not close fd here
			}else{
				// get body for error message
				sprintf(archivo_error, "../%s%d.html", DEFAULT_ERROR_RESPONSES_FOLDER, request_status);
				if(fd != -1){
					close(fd);
					printf("OJO!!!!!!!!!!!!!");
				}
				fd = open(archivo_error, O_RDONLY);
	
				if(fd==-1){
					strcpy(buf , "<html><head></head><body>Error file not found, please, do not delete /default_responses/ folder nor its content</body></html>");
					bodyLength = strlen(buf);
				}else{
					while (bLeidos=read(fd, buf, sizeof(buf))>0){
						bodyLength += strlen(buf);
					}

					close(fd);
				}
		
			}

			//bodyLength = 800;

			// PROCESS HEADER & BODY
			write_HEADER(sd, resource, request_status, bodyLength);

		
			// TODO: Should be done with fopen, fread, so we could rewind() to read the file again, or just seek to the final of the file and get its size

			if(request_status == 200){
				fd = open(archivo, O_RDONLY);

				if(fd != -1){
					while (bLeidos=read(fd, buf, sizeof(buf))>0){
						write(sd, buf, strlen(buf));
					}

					close(fd);
				}
			}else{
				// get body for error message
				sprintf(archivo_error, "../%s%d.html", DEFAULT_ERROR_RESPONSES_FOLDER, request_status);
				fd = open(archivo_error, O_RDONLY);
	
				if(fd==-1){
					strcpy(buf , "<html><head></head><body>Error file not found, please, do not delete /default_responses/ folder nor its content</body></html>");
					write(sd, buf, strlen(buf));
				}else{
					while (bLeidos=read(fd, buf, sizeof(buf))>0){
						write(sd, buf, strlen(buf));
					}

					close(fd);
				}
		
			}

		}
		if( strcmp("HEAD", comando) == 0 ){
			sprintf(archivo, "../%s%s", p.DOCUMENT_ROOT, resource); // add a ".." for security reasons (prevent people from accessing system folders)

			fd = open(archivo, O_RDONLY);


			/******

			*
			*  process request body
			*
			*******/
			if(fd==-1)
			{		
				if(errno == 2) //no such file or directory
					request_status = 404; // FILE NOT FOUND 404
				if(errno == 13) // permision denied
					request_status = 403; // FORBRIDDEN 403


				if(p.DEBUG == 1){
					// file not found
					time_t t = time(NULL);
					struct tm tm = *localtime(&t);
					printf(ANSI_COLOR_GREEN "[%d:%d:%d] HEAD: got connection from %s -> " ANSI_COLOR_YELLOW "file not found %s" ANSI_COLOR_RESET "\n", tm.tm_hour, tm.tm_min, tm.tm_sec, inet_ntoa(their_addr.sin_addr), archivo);
				}

			}else{
				request_status = 200; // 200 OK


				if(p.DEBUG == 1){
					time_t t = time(NULL);
					struct tm tm = *localtime(&t);
					printf(ANSI_COLOR_GREEN "[%d:%d:%d] HEAD: got connection from %s -> " ANSI_COLOR_BLUE "serving %s" ANSI_COLOR_RESET "\n", tm.tm_hour, tm.tm_min, tm.tm_sec, inet_ntoa(their_addr.sin_addr), archivo);
				}
			}

			close(fd);

	

			// PROCESS HEADER
			write_HEADER(sd, resource, request_status, 0);

			if(p.DEBUG == 1){
				time_t t = time(NULL);
				struct tm tm = *localtime(&t);
				printf(ANSI_COLOR_GREEN "[%d:%d:%d] HEAD: got connection from %s -> " ANSI_COLOR_BLUE "serving %s" ANSI_COLOR_RESET "\n", tm.tm_hour, tm.tm_min, tm.tm_sec, inet_ntoa(their_addr.sin_addr), archivo);
			}
		}
		if( strcmp("DELETE", comando) == 0 ){
			sprintf(archivo, "../%s%s", p.DOCUMENT_ROOT, resource); // add a ".." for security reasons (prevent people from accessing system folders)


			// PROCESS HEADER
			request_status = 200;
			write_HEADER(sd, resource, request_status, 250);

			int done;
			done = remove(archivo);

			if(done == 0){
				// success on deletion
				strcpy(buf , "<html><head></head><body>Success deleting file</body></html>");
				write(sd, buf, strlen(buf));


				if(p.DEBUG == 1){
					time_t t = time(NULL);

					struct tm tm = *localtime(&t);
					printf(ANSI_COLOR_GREEN "[%d:%d:%d] DELETE: got connection from %s -> " ANSI_COLOR_BLUE "deleted %s" ANSI_COLOR_RESET "\n", tm.tm_hour, tm.tm_min, tm.tm_sec, inet_ntoa(their_addr.sin_addr), archivo);
				}
			}else{
				// failed to delete
				strcpy(buf , "<html><head></head><body>Error deleting file</body></html>");
				write(sd, buf, strlen(buf));

				if(p.DEBUG == 1){
					time_t t = time(NULL);

					struct tm tm = *localtime(&t);
					printf(ANSI_COLOR_GREEN "[%d:%d:%d] DELETE: got connection from %s -> " ANSI_COLOR_YELLOW "tried to delete %s" ANSI_COLOR_RESET "\n", tm.tm_hour, tm.tm_min, tm.tm_sec, inet_ntoa(their_addr.sin_addr), archivo);
				}
			}

		}


		

		
	}


	


	// log the connection to the server
	log_write_access_registry(inet_ntoa(their_addr.sin_addr), archivo, request_status);
}






void init_server_configuration(int argc, char *argv[]){
	read_config(CONFIG_FILE, &p);

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
