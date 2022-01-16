/*
 * File:   main.c
 * Author: Johann
 *
 * Created on 30 de Maio de 2021, 16:52
 * Last update on December 23td of 2021 at 17:55
 */

/*Este c�digo � de um rob� seguidor de linha da equipe Wolfbotz.
 * Aqui n�s vemos o controle do rob� bem como as tomadas de decis�o de acordo com os padr�es da pista*/
#include "main.h"

/*Vari�veis globais*/
unsigned int PWMA = 0, PWMB = 0; // Modula��o de largura de pulso enviada pelo PID
//vari�veis de controle
bool f_parada= 0;       //vari�vel que comanda quando o rob� deve parar e n�o realizar mais sua rotina
bool flag = 0;          //vari�vel de controle para identificar o momento de parada
bool flag_curva = 0;    //cronometragem entre as retas e as cruvas
bool flag_parada = 0;   //inicia e encerra a cronometragem da pista
bool f_stop = 0;              //encerra a rotina de dados
unsigned int millisegundos = 0;        //millisegundos

/*Vari�veis do encoder*/
unsigned char pulse_numberR = 0, pulse_numberL = 0; //vari�veis para contagem dos pulsos dos encoders

/*Vari�veis da UART*/
volatile char ch; //armazena o caractere lido
volatile char flag_com = 0; //flag que indica se houve recep��o de dado*/

unsigned char max_timer1, max_timer2, max_timer3_ms, max_timer_3, max_timer4, max_timer5;   


int  u_X = 0;

/*Interrup��es*/
ISR(USART_RX_vect) {
    ch = UDR0; //Faz a leitura do buffer da serial

    UART_enviaCaractere(ch); //Envia o caractere lido para o computador
    flag_com = 1; //Aciona o flag de comunica��o
}

ISR(TIMER0_OVF_vect) 
{
    TCNT0 = 56; //Recarrega o Timer 0 para que a contagem seja 100us novamente
    
    f_timers(); //fun��o de temporiza��o das rotinas   
}//end TIMER_0

ISR(ADC_vect)
{
    sensors_ADC_maq();  //m�quina de estado do conversor AD
}//end ADC_int


ISR(INT0_vect)
{
    count_pulsesD();
}

ISR(PCINT0_vect)
{
    count_pulsesE();
}


/*Fun��o principal*/
int main(void) 
{
    setup();
    while (1) loop();
    return 0;
}//end main

/*RTOS primitivo*/

/*Parte n�o vis�vel ao usu�rio*/
void f_timers (void) 
{
    static unsigned char c_timer1 = 1;
    static unsigned char c_timer2 = 1;
    static unsigned char c_timer3 = 1;
    static unsigned char c_timer4 = 1;
    static unsigned char c_timer5 = 1;
        
    
    //fun��es a cada 200us
    if(c_timer1 < max_timer1)      //tempo que quer menos 1
    {
        c_timer1++;
    }

    else
    {
        f_timer1();
        c_timer1 = 1;
    }

    /*300us*/
    if (c_timer2 < max_timer2)   //o 0 conta na contagem -> 3-1
    {
        c_timer2++; //100us -1; 200us-2;300 us-3; 300us de intervalo de tempo
    }

    else    //a cada 300us
    {
        f_timer2();
        c_timer2 = 1;
    }

    if(c_timer3 < max_timer3_ms)   //10ms
    {
        c_timer3++;
    }

    else
    {
        f_timer3();
        c_timer3 = 1;
    }

    //Timer
    //1ms
    if(c_timer4 < max_timer4)
    {
        c_timer4++;
    }

    else
    {
        f_timer4();
        c_timer4 = 1;
    }
    
    if(c_timer5 < max_timer5)   //1000us
    {
        c_timer5++;
    }

    else
    {
        f_timer5();
        c_timer5 = 1;
    }
    
}//fim do RTOS


//===Fun��es n�o vis�veis ao usu�rio======//
void setup() 
{

    setup_Hardware();   //setup das IO's e das interrup��es
    sei();              //Habilita as interrup��es
    setup_logica();     //defini��o das vari�veis l�gicas(vazio por enquanto)

}//end setup

void setup_logica() /*Fun��o que passa ponteiros para fun��es como par�m*/
{
    max_timer1 = 2,
    max_timer2 = 3,
    max_timer3_ms = 100,
    max_timer_3 = 50, 
    max_timer4 = 10,
    max_timer5 = 10;   
}


void loop()//loop vazio
{

}

void estrategia()
{

    if (!f_parada)  //se f_parada for 0... 
    {
        //sensors_sensores();             //seta o limiar da leitura dos sensores
        sensors_sentido_de_giro();      //Verifica se precisa fazer uma curva e o c�lculo do PID
        //sensors_volta_pra_pista();      //corrige o rob� caso saia da linha
    } 
}


void parada() 
{     
    sensors_le_marcadores();
}


void fim_de_pista()
{
    static char parada = 0;
    
    if(flag)
    {
       parada++;
       flag = 0;
    }
    
    
    if(parada > 1)  //dois marcadores de parada
    {
        f_parada = 1;
        motores_freio();
        parada = 0;
    } 
}

/*fun��es do encoder*/

void count_pulsesD() 
{
    static bool direction_m;
    static bool Encoder_C1Last = 0;

    bool Lstate = (tst_bit(leitura_outros_sensores, encoder_C1D) >> encoder_C1D); //vari�vel de leitura de um dos pinos do encoderD

    if (!Encoder_C1Last && Lstate) {
        //Verifica se Encoder_C1Last � falso e Lstate � verdadeiro
        bool val = tst_bit(leitura_outros_sensores, encoder_C2D >> encoder_C2D); //Vari�vel de leitura do segundo pino do encoderD

        if (!val && direction_m) direction_m = 0; //sentido hor�rio

        else if (val && !direction_m) direction_m = 1; //sentido anti-hor�rio
    }

    Encoder_C1Last = Lstate;

    if (!direction_m) pulse_numberR++; //sentido hor�rio
    else pulse_numberR--;
}

void count_pulsesE()
{
    static bool direction_m;
    static bool Encoder_C1Last = 0;

    bool Lstate = (tst_bit(leitura_sensores, encoder_C1E) >> encoder_C1E); //vari�vel de leitura de um dos pinos do encoderD

    if (!Encoder_C1Last && Lstate) 
    { //Verifica se Encoder_C1Last � falso e Lstate � verdadeiro
        bool val = (tst_bit(leitura_sensores, encoder_C2E) >> encoder_C2E); //Vari�vel de leitura do segundo pino do encoderD

        if (!val && direction_m) direction_m = 0; //sentido hor�rio

        else if (val && !direction_m) direction_m = 1; //sentido anti-hor�rio
    }

    Encoder_C1Last = Lstate;

    if (!direction_m) pulse_numberR++; //sentido hor�rio
    else pulse_numberL--; //sentido anti-hor�rio
}



void millis(void)
{
    //static unsigned int f_read = 0;
    
    millisegundos++;
}

void f_timer1(void)
{
    parada();
    fim_de_pista();         //Verifica se � o fim da pista
}

void f_timer2(void)
{
    estrategia();
}

void f_timer3(void)     //10ms
{   
    static unsigned char c_timer1 = 0;
    
    if(c_timer1 < max_timer_3)  //500ms = 0,5s
    {
        c_timer1++;
    }

    else 
    {   
        //dados_telemetria();
        c_timer1 = 1;
    }
}

void f_timer4(void)
{
    millis();   //fun��o chamada a cada 1ms 

}

void f_timer5(void)
{
    if(!f_stop)
    {
        //dados_coleta();
    }
}