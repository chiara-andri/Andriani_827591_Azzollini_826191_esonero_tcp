
/*
 * protocol.h
 *
 * Server header file
 * Definitions, constants and function prototypes for the server
 */

#ifndef PROTOCOL_H_
#define PROTOCOL_H_

#define SERVER_PORT 27015       // Porta di default da traccia
#define BUFFER_SIZE 512         // Dimensione buffer generico
#define QUEUE_SIZE 5            // Lunghezza coda connessioni pendenti
#define CITY_NAME_MAX_LEN 64    // Lunghezza massima nome città

#define STATUS_SUCCESS 0            // Richiesta valida
#define STATUS_CITY_UNAVAILABLE 1   // Città non supportata
#define STATUS_INVALID_REQUEST 2    // Tipo di richiesta non valido

#define TYPE_TEMPERATURE 't'
#define TYPE_HUMIDITY    'h'
#define TYPE_WIND        'w'
#define TYPE_PRESSURE    'p'

typedef struct {
    char type;                      // 't','h','w','p'
    char city[CITY_NAME_MAX_LEN];   // Nome città (null-terminated)
} weather_request_t;

typedef struct {
    unsigned int status;   // Codice di stato (0,1,2)
    char type;             // Eco del tipo richiesto
    float value;           // Valore meteo
} weather_response_t;

float get_temperature(void);   // Range: -10.0 to 40.0 °C
float get_humidity(void);      // Range: 20.0 to 100.0 %
float get_wind(void);          // Range: 0.0 to 100.0 km/h
float get_pressure(void);      // Range: 950.0 to 1050.0 hPa

#endif /* PROTOCOL_H_ */


































