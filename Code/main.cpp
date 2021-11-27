// Definiciones externas para el sistema de colas simple
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <math.h>
#include "lcgrand.cpp"  // Encabezado para el generador de numeros aleatorios

using namespace std;

#define LIMITE_COLA 100  // Capacidad maxima de la cola
#define OCUPADO 1 // Indicador de Servidor Ocupado
#define LIBRE 0 // Indicador de Servidor Libre

int     sig_tipo_evento,
        num_clientes_espera,
        num_esperas_requerido,
        num_eventos,
        num_entra_cola,
        num_servidores,
        estado_servidor,
        modo_operacion;

float   area_num_entra_cola,
        area_estado_servidor,
        media_entre_llegadas,
        media_atencion,
        tiempo_simulacion,
        tiempo_llegada[LIMITE_COLA + 1],
        tiempo_ultimo_evento,
        tiempo_sig_evento[3],
        total_de_esperas,
        tiempo_anterior,
        a1, b1, a2, b2;

FILE    *parametros,
        *resultados,
        *datosLlegada;

void  inicializar(void);
void  controltiempo(void);
void  llegada(void);
void  salida(void);
void  reportes(void);
void  actualizar_estad_prom_tiempo(void);
float expon(float mean);
float uniform(float a, float b);

// ------------------------------------------------------------------------------------------------------------------------------

int main(void)  // Función Principal
{
    // Inicializa la simulación.
    inicializar();

    // Corre la simulación mientras no se llegue al numero de clientes especificado en el archivo de entrada
    while (num_clientes_espera < num_esperas_requerido) {

        // Determina el siguiente evento
        controltiempo();

        // Actualiza los acumuladores estadisticos de tiempo promedio
        actualizar_estad_prom_tiempo();

        // Invoca la funcion del evento adecuado.
        switch (sig_tipo_evento) {
            case 1:
                llegada();
                break;
            case 2:
                salida();
                break;
        }
    }

    // Invoca el generador de reportes y termina la simulacion.
    reportes();

    fclose(parametros);
    fclose(resultados);

    return 0;
}


void inicializar(void)  // Funcion de inicializacion. 
{
    // Abre los archivos de entrada y salida
    parametros  = fopen("param.txt",  "r");
    resultados = fopen("result.txt", "w");
    datosLlegada = fopen("datosLlegada.txt", "w");

    // Especifica el número de eventos para la funcion controltiempo.
    num_eventos = 2;

    // Lectura de los parámetros de entrada
    fscanf(parametros, "%f %f %d %i %f %f %f %f %i",
            &media_entre_llegadas,
            &media_atencion,
            &num_esperas_requerido,
            &modo_operacion,
            &a1,
            &b1,
            &a2,
            &b2,
            &num_servidores);
    
    // Print de datos según Modo de Operación
    switch(modo_operacion){
        case 1: // Modo M/M/1
            fprintf(resultados, "Sistema de Colas M/M/1\n\n");
            fprintf(resultados, "Tiempo promedio de llegada%11.3f minutos\n\n", media_entre_llegadas);
            fprintf(resultados, "Tiempo promedio de atencion%16.3f minutos\n\n", media_atencion);
        break;
        case 2: // Modo U/M/1
            fprintf(resultados, "Sistema de Colas U/M/1\n\n");
            fprintf(resultados, "a1%14f\n\n", a1);
            fprintf(resultados, "b1%14f\n\n", b1);
            fprintf(resultados, "Tiempo promedio de atencion%16.3f minutos\n\n", media_atencion);
        break;
        case 3: // Modo U/U/1
            fprintf(resultados, "Sistema de Colas U/U/1\n\n");
            fprintf(resultados, "a1%14f\n\n", a1);
            fprintf(resultados, "b1%14f\n\n", b1);
            fprintf(resultados, "a2%14f\n\n", a2);
            fprintf(resultados, "b2%14f\n\n", b2);
        break;
        case 4: // Modo M/M/n
            fprintf(resultados, "Sistema de Colas M/M/n\n\n");
            fprintf(resultados, "Tiempo promedio de llegada%11.3f minutos\n\n", media_entre_llegadas);
            fprintf(resultados, "Tiempo promedio de atencion%16.3f minutos\n\n", media_atencion);
            fprintf(resultados, "Numero de servidores%14i\n\n", num_servidores);
        break;
    };

    fprintf(resultados, "Numero de clientes%14d\n\n", num_esperas_requerido);

    // Inicializa el reloj de la simulacion.
    tiempo_simulacion = 0.0;

    // Inicializa las variables de estado
    estado_servidor   = 0;
    num_entra_cola        = 0;
    tiempo_ultimo_evento = 0.0;

    // Inicializa los contadores estadisticos.
    num_clientes_espera  = 0;
    total_de_esperas    = 0.0;
    area_num_entra_cola      = 0.0;
    area_estado_servidor = 0.0;

    // Inicializa la lista de eventos.
    // Ya que no hay clientes, el evento salida (terminacion del servicio) no se tiene en cuenta
    switch(modo_operacion){
        case 1: // Modo M/M/1
            num_servidores = 1;
            tiempo_sig_evento[1] = tiempo_simulacion + expon(media_entre_llegadas);
        break;
        case 2: // Modo U/M/1
            num_servidores = 1;
            tiempo_sig_evento[1] = tiempo_simulacion + uniform(a1,b1);
        break;
        case 3: // Modo U/U/1
            num_servidores = 1;
            tiempo_sig_evento[1] = tiempo_simulacion + uniform(a1,b1);
        break;
        case 4: // Modo M/M/n
            tiempo_sig_evento[1] = tiempo_simulacion + expon(media_entre_llegadas);
        break;
    };
    
    tiempo_sig_evento[2] = 1.0e+30;
}


void controltiempo(void)  // Funcion controltiempo
{
    int   i;
    float min_tiempo_sig_evento = 1.0e+29;

    sig_tipo_evento = 0;

    //  Determina el tipo de evento del evento que debe ocurrir.
    for (i = 1; i <= num_eventos; ++i)
        if (tiempo_sig_evento[i] < min_tiempo_sig_evento) {
            min_tiempo_sig_evento = tiempo_sig_evento[i];
            sig_tipo_evento     = i;
        }

    // Revisa si la lista de eventos esta vacia.
    if (sig_tipo_evento == 0) {

        // La lista de eventos esta vacia, se detiene la simulacion.
        fprintf(resultados, "\nLa lista de eventos esta vacia %f", tiempo_simulacion);
        exit(1);
    }

    // La lista de eventos no esta vacia, adelanta el reloj de la simulacion.
    tiempo_simulacion = min_tiempo_sig_evento;
}


void llegada(void)  // Funcion de llegada 
{
    float espera;

    // PRINT:
    tiempo_anterior = tiempo_sig_evento[1];

    // Programa la siguiente llegada.
    switch(modo_operacion){
        case 1: // Modo M/M/1
            tiempo_sig_evento[1] = tiempo_simulacion + expon(media_entre_llegadas);
        break;
        case 2: // Modo U/M/1
            tiempo_sig_evento[1] = tiempo_simulacion + uniform(a1,b1);
        break;
        case 3: // Modo U/U/1
            tiempo_sig_evento[1] = tiempo_simulacion + uniform(a1,b1);
        break;
        case 4: // Modo M/M/n
            tiempo_sig_evento[1] = tiempo_simulacion + expon(media_entre_llegadas);
        break;
    };

    // Revisa si el servidor esta OCUPADO.
    // if (estado_servidor == OCUPADO) {
    if (estado_servidor >= num_servidores) {

        // Servidor OCUPADO, aumenta el numero de clientes en cola
        ++num_entra_cola;

        // Verifica si hay condición de desbordamiento
        if (num_entra_cola > LIMITE_COLA) {

            // Se ha desbordado la cola, detiene la simulacion
            fprintf(resultados, "\nDesbordamiento del arreglo tiempo_llegada a la hora");
            fprintf(resultados, "%f", tiempo_simulacion);
            exit(2);
        }

        // Todavia hay espacio en la cola, se almacena el tiempo de llegada
        // del cliente en el ( nuevo ) fin de tiempo_llegada
        tiempo_llegada[num_entra_cola] = tiempo_simulacion;
    }

    else {

        //  El servidor esta LIBRE, por lo tanto el cliente que llega tiene tiempo de espera=0
        // (Las siguientes dos lineas del programa son para claridad, y no afectan el reultado de la simulacion)
        espera = 0.0;
        total_de_esperas += espera;

        // Incrementa el numero de clientes en espera, y pasa el servidor a ocupado 
        ++num_clientes_espera;
        // estado_servidor = OCUPADO;
        if (estado_servidor < num_servidores) {
            estado_servidor = estado_servidor + 1;
        }

        // Programa una salida (servicio terminado).
        switch(modo_operacion){
            case 1: // Modo M/M/1
                tiempo_sig_evento[2] = tiempo_simulacion + expon(media_atencion);
            break;
            case 2: // Modo U/M/1
                tiempo_sig_evento[2] = tiempo_simulacion + expon(media_atencion);
            break;
            case 3: // Modo U/U/1
                tiempo_sig_evento[2] = tiempo_simulacion + uniform(a2,b2);
            break;
            case 4: // Modo M/M/n
                tiempo_sig_evento[2] = tiempo_simulacion + expon(media_atencion);
            break;
        };
    }
    fprintf (datosLlegada, "%f\n",(tiempo_sig_evento[1]-tiempo_anterior)*60);
}


void salida(void)  // Funcion de Salida. 
{
    int   i;
    float espera;

    // Revisa si la cola esta vacia
    if (num_entra_cola == 0) {

        // La cola esta vacia, pasa el servidor a LIBRE y no considera el evento de salida
        // estado_servidor = LIBRE;
        if (estado_servidor > 0) {
            estado_servidor = estado_servidor - 1;
        }
        tiempo_sig_evento[2] = 1.0e+30;
    }

    else {

        // La cola no esta vacia, disminuye el numero de clientes en cola.
        --num_entra_cola;

        //Calcula la espera del cliente que esta siendo atendido y actualiza el acumulador de espera
        espera = tiempo_simulacion - tiempo_llegada[1];
        total_de_esperas += espera;

        //Incrementa el numero de clientes en espera, y programa la salida.    
        ++num_clientes_espera;
        switch(modo_operacion){
            case 1: // Modo M/M/1
                tiempo_sig_evento[2] = tiempo_simulacion + expon(media_atencion);
            break;
            case 2: // Modo U/M/1
                tiempo_sig_evento[2] = tiempo_simulacion + expon(media_atencion);
            break;
            case 3: // Modo U/U/1
                tiempo_sig_evento[2] = tiempo_simulacion + uniform(a2,b2);
            break;
            case 4: // Modo M/M/n
                tiempo_sig_evento[2] = tiempo_simulacion + expon(media_atencion);
            break;
        };

        // Mueve cada cliente en la cola ( si los hay ) una posicion hacia adelante 
        for (i = 1; i <= num_entra_cola; ++i)
            tiempo_llegada[i] = tiempo_llegada[i + 1];
    }
}


void reportes(void)  // Funcion generadora de reportes. 
{
    // Calcula y estima los estimados de las medidas deseadas de desempe�o   
    fprintf(resultados, "\n\nEspera promedio en la cola%11.3f minutos\n\n",
            total_de_esperas / num_clientes_espera);
    fprintf(resultados, "Numero promedio en cola%10.3f\n\n",
            area_num_entra_cola / tiempo_simulacion);
    fprintf(resultados, "Uso del servidor%15.3f\n\n",
            area_estado_servidor / tiempo_simulacion);
    fprintf(resultados, "Tiempo de terminacion de la simulacion%12.3f minutos", tiempo_simulacion);
}


void actualizar_estad_prom_tiempo(void)  // Actualiza los acumuladores de área para las estadisticas de tiempo promedio.
{
    float time_since_last_event;

    // Calcula el tiempo desde el ultimo evento, y actualiza el marcador del ultimo evento.
    time_since_last_event = tiempo_simulacion - tiempo_ultimo_evento;
    tiempo_ultimo_evento = tiempo_simulacion;

    // Actualiza el area bajo la funcion de numero_en_cola
    area_num_entra_cola += num_entra_cola * time_since_last_event;

    //Actualiza el area bajo la funcion indicadora de servidor ocupado
    area_estado_servidor += estado_servidor * time_since_last_event;
}


float expon(float media)  // Funcion generadora de la exponencias 
{
    // Retorna una variable aleatoria exponencial con media "media"
    return -media * log(lcgrand(1));
}

float uniform(float a, float b) // Función generadora de intervalos uniformes
{
    return a + lcgrand(1) * (b-a);
}

float percentil(float param_poblacional) // Función Percentil
{
    return tiempo_simulacion + expon(param_poblacional);
}