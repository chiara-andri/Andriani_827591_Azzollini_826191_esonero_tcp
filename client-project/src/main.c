/*
 * main.c
 *
 * TCP Client - Weather Service (portable: Windows, Linux, macOS)
 */

#if defined WIN32
  #include <winsock.h>
  #ifndef NO_ERROR
  #define NO_ERROR 0
  #endif
#else

  #include <unistd.h>
  #include <sys/types.h>
  #include <sys/socket.h>
  #include <arpa/inet.h>
  #include <netinet/in.h>
  #include <netdb.h>
  #define closesocket close
#endif
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "protocol.h"

static void clearwinsock(void) {
#if defined WIN32
    WSACleanup();
#endif
}


static void usage(void) {
	    printf("Uso: client-project [-s server] [-p porta] -r \"tipo città\"\n");
	    printf("  -s server : nome host o IP (default: 127.0.0.1)\n");
	    printf("  -p porta  : porta del server (default: %d)\n", SERVER_PORT);
	    printf("  -r req    : stringa di richiesta, ad es. \"t Bari\" oppure \"p Reggio Calabria\"\n");
	    exit(1);
	}


int main(int argc, char *argv[]) {

#if defined WIN32
    WSADATA wsa_data;
    int result = WSAStartup(MAKEWORD(2,2), &wsa_data);
    if (result != NO_ERROR) {
        printf("Errore a WSAStartup()\n");
        return 1;
    }
#endif

    char server_host[256] = "127.0.0.1";
    int server_port = SERVER_PORT;
    char request_str[BUFFER_SIZE] = {0};

    if (argc < 3) usage();
    int have_r = 0;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-s") == 0) {
            if (++i >= argc) usage();
            strncpy(server_host, argv[i], sizeof(server_host)-1);
            server_host[sizeof(server_host)-1] = '\0';
        } else if (strcmp(argv[i], "-p") == 0) {
            if (++i >= argc) usage();
            server_port = atoi(argv[i]);
            if (server_port <= 0 || server_port > 65535) {
                printf("porta non valida : %s\n", argv[i]);
                usage();
            }
        } else if (strcmp(argv[i], "-r") == 0) {
            if (++i >= argc) usage();
            strncpy(request_str, argv[i], sizeof(request_str)-1);
            request_str[sizeof(request_str)-1] = '\0';
            have_r = 1;
        } else {
            usage();
        }
    }
    if (!have_r) usage();

    weather_request_t request;
    memset(&request, 0, sizeof(request));

    if (request_str[0] == '\0') {
        printf("argomento -r  non valido (vuoto)\n");
        usage();
    }
    request.type = request_str[0];

    const char *city_start = request_str + 1;
    while (*city_start == ' ') city_start++;

    strncpy(request.city, city_start, CITY_NAME_MAX_LEN - 1);
    request.city[CITY_NAME_MAX_LEN - 1] = '\0';

    int my_socket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (my_socket < 0) {
        perror("creazione socket fallita");
        clearwinsock();
        return 1;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);


    unsigned long ip = inet_addr(server_host);
    char resolved_ip[64] = {0};

    if (ip != INADDR_NONE) {
        server_addr.sin_addr.s_addr = ip;
        snprintf(resolved_ip, sizeof(resolved_ip), "%s", server_host);  // <-- aggiungi questa riga
    } else {
        struct hostent *host = gethostbyname(server_host);
        if (host == NULL || host->h_addr_list == NULL || host->h_addr_list[0] == NULL) {
            perror("risoluzione dell'hostname fallita");
            closesocket(my_socket);
            clearwinsock();
            return 1;
        }
        memcpy(&server_addr.sin_addr.s_addr, host->h_addr_list[0], host->h_length);
        strncpy(resolved_ip, inet_ntoa(*(struct in_addr*)host->h_addr_list[0]), sizeof(resolved_ip)-1);
    }


    if (connect(my_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("connessione fallita");
        closesocket(my_socket);
        clearwinsock();
        return 1;
    }

    int sent = send(my_socket, (char*)&request, sizeof(request), 0);
    if (sent != (int)sizeof(request)) {
        perror("invio fallito");
        closesocket(my_socket);
        clearwinsock();
        return 1;
    }

    weather_response_t response;
    int recvd = recv(my_socket, (char*)&response, sizeof(response), 0);
    if (recvd <= 0) {
        perror("recv fallita");
        closesocket(my_socket);
        clearwinsock();
        return 1;
    }

    printf("Ricevuto risultato dal server ip %s. ", resolved_ip[0] ? resolved_ip : "unknown");

    if (response.status == STATUS_SUCCESS) {
        char city_output[CITY_NAME_MAX_LEN];
        strncpy(city_output, request.city, CITY_NAME_MAX_LEN);
        city_output[CITY_NAME_MAX_LEN - 1] = '\0';

        city_output[0] = toupper(city_output[0]);

        for (int i = 1; city_output[i] != '\0'; i++) {
            city_output[i] = tolower(city_output[i]);
        }

        if (response.type == TYPE_TEMPERATURE)
            printf("%s: Temperatura = %.1f%cC\n", city_output, response.value, 248);
        else if (response.type == TYPE_HUMIDITY)
            printf("%s: Umidità = %.1f%%\n", city_output, response.value);
        else if (response.type == TYPE_WIND)
            printf("%s: Vento = %.1f km/h\n", city_output, response.value);
        else if (response.type == TYPE_PRESSURE)
            printf("%s: Pressione = %.1f hPa\n", city_output, response.value);
        else
            printf("Tipo sconosciuto nella risposta\n");

    } else if (response.status == STATUS_CITY_UNAVAILABLE) {
        printf("Città non disponibile\n");
    } else if (response.status == STATUS_INVALID_REQUEST) {
        printf("Richiesta non valida\n");
    } else {
        printf("Errore sconosciuto (status=%u)\n", response.status);
    }


    #if defined WIN32
        closesocket(my_socket);
    #else
        close(my_socket);
    #endif
    clearwinsock();
    return 0;
}

