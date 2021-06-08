/*==================[ Inclusiones ]============================================*/
#include "pulsador.h"

/*==================[ Definiciones ]===================================*/

#define T_REBOTE_MS     40
#define T_REBOTE        pdMS_TO_TICKS(T_REBOTE_MS)
//#define SALIDA_PRUEBA   GPIO_NUM_25

/*==================[Prototipos de funciones]======================*/

static void errorPulsador( uint8_t  indice );
static void botonPresionado( uint8_t  indice );
static void botonLiberado( uint8_t  indice );

void tareaPulsador( void* taskParmPtr );

/*==================[Variables]==============================*/
gpio_int_type_t pulsadorPines[N_PULSADOR] = { GPIO_NUM_18, GPIO_NUM_19};

pulsadorInfo pulsador [N_PULSADOR];
ledinfo ledA;

ledinfo *punteroled =  &ledA;
void inicializarPulsador( void )
{

comandosLed  = xQueueCreate( 1 , sizeof(ledinfo*) ); // Creación de la cola

    for(int i = 0; i < N_PULSADOR; i++)
    {
        pulsador[i].tecla             = pulsadorPines[i];
        pulsador[i].estado            = ALTO;                     //Estado inicial
        pulsador[i].tiempoBajo        = TIEMPO_NO_VALIDO;
        pulsador[i].tiempoAlto        = TIEMPO_NO_VALIDO;

        gpio_pad_select_gpio(pulsador[i].tecla);
        gpio_set_direction(pulsador[i].tecla , GPIO_MODE_INPUT);
        gpio_set_pull_mode(pulsador[i].tecla, GPIO_PULLDOWN_ONLY); //Habilita resistencia de PULLDOWN interna
    }

    // Crear tareas en freeRTOS
    BaseType_t res = xTaskCreatePinnedToCore(
    	tareaPulsador,                     	// Funcion de la tarea a ejecutar
        "tareaPulsador",   	                // Nombre de la tarea como String amigable para el usuario
        configMINIMAL_STACK_SIZE*2, 		// Cantidad de stack de la tarea
        NULL,                          	// Parametros de tarea
        tskIDLE_PRIORITY+1,         	// Prioridad de la tarea -> Queremos que este un nivel encima de IDLE
        NULL,                          		// Puntero a la tarea creada en el sistema
        1
    );

    // Gestion de errores
	if(res == pdFAIL)
	{
		printf( "Error al crear la tarea.\r\n" );
		while(true);					// si no pudo crear la tarea queda en un bucle infinito
	}
}

static void errorPulsador( uint8_t  indice )
{
    pulsador[indice].estado = ALTO;
}

// pulsador_ Update State Function
void actualizarPulsador( uint8_t  indice)
{
    switch( pulsador[indice].estado )
    {
        case BAJO:
            if( gpio_get_level( pulsador[indice].tecla ) ){
                pulsador[indice].estado = ASCENDENTE;
            }
            break;

        case ASCENDENTE:
            if( gpio_get_level( pulsador[indice].tecla ) ){
                pulsador[indice].estado = ALTO;
                botonPresionado(indice);
            }
            else{
                pulsador[indice].estado = BAJO;
            }
            break;

        case ALTO:
            if( !gpio_get_level( pulsador[indice].tecla ) ){
                pulsador[indice].estado = DESCENDENTE;
            }
            break;

        case DESCENDENTE:
            if( !gpio_get_level( pulsador[indice].tecla ) ){
                pulsador[indice].estado = BAJO;
                botonLiberado(indice);
            }
            else{
                pulsador[indice].estado = ALTO;
            }
            break;

        default:
            errorPulsador(indice);
            break;
    }
}

/* accion de el evento de tecla pulsada */
static void botonPresionado( uint8_t  indice)
{
     TickType_t conteoTicksActuales = xTaskGetTickCount();
    pulsador[indice].tiempoBajo = conteoTicksActuales;   
    //gpio_set_level( SALIDA_PRUEBA, 1 );
}

/* accion de el evento de tecla liberada */
static void botonLiberado( uint8_t  indice)
{
    TickType_t conteoTicksActuales = xTaskGetTickCount();   //Medimos el tiempo en ticks desde que inició el scheduler
    pulsador[indice].tiempoAlto = conteoTicksActuales;
    ledA.diferenciaTiempo  = pulsador[indice].tiempoAlto - pulsador[indice].tiempoBajo; //Da el tiempo que el pulsador estuvo en estado alto
    ledA.indice1 = indice ;
    //gpio_set_level( SALIDA_PRUEBA, 0 );
    xQueueSend( comandosLed , &punteroled,  portMAX_DELAY  ); // Envío dato a la la cola
}

void tareaPulsador( void* taskParmPtr )
{
    while( true )
    {
        for (int i = 0; i < N_PULSADOR; i++)
        {
            actualizarPulsador(i);
        }
        vTaskDelay( T_REBOTE );
    }
}