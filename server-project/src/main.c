/*
 * main.c
 *
 * TCP Server - Weather Service
 * Portabile su Windows, Linux e macOS
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#if defined WIN32
  #include <winsock.h>
#else
  #include <sys/types.h>
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <arpa/inet.h>
  #include <unistd.h>
  #define closesocket close
#endif

#include "protocol.h"

void clearwinsock() {
    #if defined WIN32
    WSACleanup();
    #endif
}

void errorHandler(const char* s) {
    printf("ERRORE: %s\n", s);
    exit(1);
}

const int NUM_SUPPORTED_CITIES = 10;
const char *SUPPORTED_CITIES[] = {
    "bari", "roma", "milano", "napoli", "torino",
    "palermo", "genova", "bologna", "firenze", "venezia"
};


void to_lowercase(char *str) {
    int i = 0;
    while (str[i] != '\0') {
        str[i] = tolower(str[i]);
        i++;
    }
}

int is_city_supported(const char *city) {
    char lower_city[CITY_NAME_MAX_LEN];
    strncpy(lower_city, city, CITY_NAME_MAX_LEN);
    lower_city[CITY_NAME_MAX_LEN - 1] = '\0';
    to_lowercase(lower_city);

    for (int i = 0; i < NUM_SUPPORTED_CITIES; i++) {
        if (strcmp(lower_city, SUPPORTED_CITIES[i]) == 0) {
            return 1;
        }
    }
    return 0;
}


float get_temperature() {
	return -10.0f + (rand() / (float)RAND_MAX) * 50.0f; }

float get_humidity()    {
	return 20.0f  + (rand() / (float)RAND_MAX) * 80.0f; }

float get_wind()        {
	return 0.0f   + (rand() / (float)RAND_MAX) * 100.0f; }

float get_pressure()    {
	return 950.0f + (rand() / (float)RAND_MAX) * 100.0f; }

void handle_client_connection(int client_socket, struct sockaddr_in *client_addr) {
    printf("Gestione client %s\n", inet_ntoa(client_addr->sin_addr));

    weather_request_t request;
    weather_response_t response;
    ssize_t bytes_received;

    bytes_received = recv(client_socket, (char *)&request, sizeof(request), 0);
    if (bytes_received <= 0) {
        printf("Connessione chiusa dal client o errore di ricezione.\n");
        #if defined WIN32
            closesocket(client_socket);
        #else
            close(client_socket);
        #endif
        return;
    }
    request.city[CITY_NAME_MAX_LEN - 1] = '\0';

    printf("Richiesta '%c %s' dal client ip %s\n",
           request.type, request.city, inet_ntoa(client_addr->sin_addr));

    response.status = STATUS_SUCCESS;
    response.type   = request.type;
    response.value  = 0.0f;

    if (request.type != 't' && request.type != 'h' &&
        request.type != 'w' && request.type != 'p') {
        response.status = STATUS_INVALID_REQUEST;
        response.type   = '\0';
        response.value  = 0.0f;
    }

    else if (!is_city_supported(request.city)) {
        response.status = STATUS_CITY_UNAVAILABLE;
        response.type   = '\0';
        response.value  = 0.0f;
    }
    else {
        switch (request.type) {
            case 't': response.value = get_temperature(); break;
            case 'h': response.value = get_humidity();    break;
            case 'w': response.value = get_wind();        break;
            case 'p': response.value = get_pressure();    break;
        }
    }

    if (send(client_socket, (char *)&response, sizeof(weather_response_t), 0) != sizeof(weather_response_t)) {
        printf("Errore nell'invio della risposta al client\n");
    }

    #if defined WIN32
        closesocket(client_socket);
    #else
        close(client_socket);
    #endif
}


int main(int argc, char *argv[]) {
    int port;

    if (argc > 1) {
        port = atoi(argv[1]);
    } else {
        port = SERVER_PORT;
    }
    if (port <= 0) {
        printf("Numero di porta non valido: %s\n", argv[1]);
        return 0;
    }
    srand(time(NULL));

    #if defined WIN32
        WSADATA wsa_data;
        int result = WSAStartup(MAKEWORD(2,2), &wsa_data);
        if (result != 0) {
            errorHandler("Errore in WSAStartup() \n");
        }
    #endif

    int serverSocket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocket < 0) {
        #if defined WIN32
            printf("Creazione del socket fallita (WSAGetLastError=%d)\n", WSAGetLastError());
        #else
            perror("Creazione del socket fallita");
        #endif
        clearwinsock();
        return -1;
    }

    int yes = 1;
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&yes, sizeof(yes));

    struct sockaddr_in sad;
    memset(&sad, 0, sizeof(sad));
    sad.sin_family = AF_INET;
    sad.sin_addr.s_addr = inet_addr("127.0.0.1");
    sad.sin_port = htons(port);

    if (bind(serverSocket, (struct sockaddr*) &sad, sizeof(sad)) < 0) {
        #if defined WIN32
            printf("Bind fallito (WSAGetLastError=%d)\n", WSAGetLastError());
        #else
            perror("Bind fallito");
        #endif
        closesocket(serverSocket);
        clearwinsock();
        return -1;
    }

    if (listen(serverSocket, QUEUE_SIZE) < 0) {
        #if defined WIN32
            printf("Listen fallito (WSAGetLastError=%d)\n", WSAGetLastError());
        #else
            perror("Listen fallito");
        #endif
        closesocket(serverSocket);
        clearwinsock();
        return -1;
    }


    printf("In attesa di un client...\n");


    while (1) {
    	struct sockaddr_in cad;
    	#if defined WIN32
    	int client_len = sizeof(struct sockaddr_in);
    	#else
    	socklen_t client_len = sizeof(struct sockaddr_in);
    	#endif
    	int client_socket = accept(serverSocket, (struct sockaddr*)&cad, &client_len);
    	if (client_socket < 0) {
    	printf("accept() fallita.\n");
    	continue;
    }
        handle_client_connection(client_socket, &cad);
    }

    return 0;
}
