// Área de inclusão das bibliotecas  

 

//-----------------------------------------------------------------------------------------------------------------------  

#include <stdio.h>  

#include <string.h>  

#include "freertos/FreeRTOS.h"  

#include "freertos/task.h"  

#include "esp_log.h"  

#include "ioplaca.h"   // Controles das Entradas e Saídas digitais e do teclado  

#include "lcdvia595.h" // Controles do Display LCD  

#include "hcf_adc.h"   // Controles do ADC  

#include "MP_hcf.h"   // Controles do Motor  

#include "connect.h"    // Controle do Wifi  

#include <stdio.h>  

#include <time.h>  

#include <sys/time.h>  

#include "esp_sntp.h"  

#include "freertos/FreeRTOS.h"  

#include "freertos/task.h"  

#include "esp_wifi.h" 

#include "esp_netif.h"  

#include "mqtt_client.h"  

//#include "protocol_examples_common.h"  

#include "esp_system.h"  

#include "nvs_flash.h"  

#include "esp_system.h" // Para obter informações sobre o sistema 

#include "esp_sleep.h" // Para funções de baixo consumo de energia 

// Área das macros  

 
//-----------------------------------------------------------------------------------------------------------------------  

# define PWD 1234  

#define W_DEVICE_ID "65774aa82623fd911ab650c1" //Use o DeviceID no Wegnology    

#define W_ACCESS_KEY "76ac5ed2-ed18-4e96-9e02-d2dd572db083" //use a chave de acesso e a senha  

#define W_PASSWORD "f52797619b7205bc2ac8d796d80fd0cb23f988e882cd0b82d575b26939f78c1c"  

#define W_TOPICO_PUBLICAR "wnology/65774aa82623fd911ab650c1/state" //esse número no meio do tópico deve ser mudado pelo ID do seu device Wegnology  

#define W_TOPICO_SUBSCREVER "wnology/65774aa82623fd911ab650c1/command" // aqui também  

#define W_BROKER "mqtt://broker.app.wnology.io:1883"  

#define SSID "iPhone de Ana Clara"  

#define PASSWORD "123456789"  

#define remedios(x) ((entradas >>x)&1)  

// Área de declaração de variáveis e protótipos de funções  

 

//-----------------------------------------------------------------------------------------------------------------------  

int alarme = 15;  // MUDAR PARA HORARIO ESCOLHIDO 

int total_remedios_retirados = 7;  

static const char *TAG = "Placa";  

static uint8_t entradas, saidas = 0; //variáveis de controle de entradas e saídas  

int controle = 0;  

int senha = 0;  

int tempo = 50;  

int coluna = 0;  

uint32_t adcvalor = 0;  

char exibir [40];  

char mensa [40];  

const char *strLED = "LED\":";  

const char *subtopico_temp = "{\"data\": {\"Temperatura\": ";  

char * Inform;  

bool ledstatus;  
 
esp_mqtt_client_handle_t cliente; 
 
int temperatura = 0;  

time_t now;  

struct tm timeinfo;  

int hora = 0;  

int hoje = 0;  

int minuto = 0;  

int pinosRemedios [7] = {0,0,0,0,0,0,0,};  

char dia_str[40];  

char tecla;  

static int dia_da_semana = 0; // 0=domingo, 1=segunda, ..., 6=sábado  


// Funções e ramos auxiliares para o cofre  


//-----------------------------------------------------------------------------------------------------------------------  

 
// Funções e ramos auxiliares para o IoT  

 

//-----------------------------------------------------------------------------------------------------------------------  

void time_sync_notification_cb(struct timeval *tv)  
{  
    printf("Hora sincronizada!\n");  
}  

// Função para configurar o NTP e sincronizar o tempo 

 void initialize_rtc()  

 { 
   //O RTC já é inicializado automaticamente no ESP32. 
   // Você pode configurar o RTC aqui, se necessário. 
} 

void initialize_sntp(void)  
 {  
 printf("Inicializando o SNTP...\n"); 
 sntp_setoperatingmode(SNTP_OPMODE_POLL);  
 sntp_setservername(0, "pool.ntp.org");  // Servidor NTP  
 sntp_set_time_sync_notification_cb(time_sync_notification_cb);  
 sntp_init();  
}  

//Função para obter a hora e o dia da semana  

 void exibir_dia_da_semana(int weekday) 
 {
    sprintf(dia_str, "Dia: %d", weekday); // Formata a string com o dia da semana
    lcd595_write(1, 0, dia_str); // Mostra o dia da semana no display
}

void obtain_time(void) 
{
   // Tenta obter a hora via NTP 
    if (sntp_get_sync_status() == SNTP_SYNC_STATUS_COMPLETED)
     { 
      time(&now); 
      localtime_r(&now, &timeinfo); 
      timeinfo.tm_hour -= 3; // Ajuste para UTC-3 
       mktime(&timeinfo); 
        // Obtendo o dia da semana 
      int weekday = timeinfo.tm_wday;   
      char buffer[64]; 
      strftime(buffer, sizeof(buffer), "%c", &timeinfo); 
      printf("Hora atual (NTP): %s\n", buffer); 
      printf("Dia da semana: %d\n", weekday);  // Exibe o dia da semana 

    }
     else 
    { 
    // Se a sincronização falhar, usa o RTC 
        time(&now); 
        localtime_r(&now, &timeinfo); // Obter o tempo atual 
        timeinfo.tm_hour -= 3; // Ajuste para UTC-3 

        // Aqui assumimos que o RTC não está ajustado, então exibimos a hora atual 
        printf("Hora atual (RTC): %04d-%02d-%02d %02d:%02d:%02d\n", 
        timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday, 
        timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec); 
        // Obtendo o dia da semana 
        int weekday = timeinfo.tm_wday;   
        printf("Dia da semana: %d\n", weekday);  // Exibe o dia da semana 
        exibir_dia_da_semana(weekday);
    } 

} 

void enviar_lembrete() 

{ 
lcd595_write(1,1, "TOMAR REMEDIO" );  ///Aparecer no display  
}  

 
void enviar_erro ()  
{  
lcd595_write(2,0, "ERRADO" );   ///Aparecer no display   
}  

void enviar_certo()  

{  
    lcd595_write(2,0, "CERTO" );   ///Aparecer no display   
}  

void exibir_total_remedios_gaveta() 
{
    int total_remedios_na_gaveta = 0;
    // Contar remédios na gaveta (0 = na gaveta)
    for (int i = 0; i < 7; i++) 
    {
        if (pinosRemedios[i] == 0)
         {
            total_remedios_na_gaveta++;
        }
    }
    // Exibir no display
    sprintf(dia_str, "Gaveta: %d", total_remedios_na_gaveta);
    lcd595_write(2, 0, dia_str); // Mostra a quantidade de remédios na gaveta
}

  int testar_remedios(int data) 

{    lcd595_write(2, 0, "        ");
     int total_remedios_retirados = 7;  

    if(total_remedios_retirados == hoje && pinosRemedios[hoje] == 1 /*&& hora > alarme*/) //{lcd595_write(2,0,"    "); return 0;} 
    {
      exibir_total_remedios_gaveta();
            return 0;
    }

    else if(total_remedios_retirados == hoje - 1 && pinosRemedios[hoje] == 0 ) 
    {  
       // if(hora > alarme) { enviar_lembrete(); } 
         exibir_total_remedios_gaveta();
        return 0; 
    }
    else return 1; 
}  

void detecta_remedios ()   

{
   time_t now;  
   struct tm timeinfo;  
   time(&now);  
   localtime_r(&now, &timeinfo);  
   timeinfo.tm_hour += -3;  // Ajuste para UTC-3  
   hora = timeinfo.tm_hour;  
   minuto = timeinfo.tm_min;  
   hoje = timeinfo.tm_wday;  

    int total_remedios_retirados = 7;  
    entradas = io_le_escreve (saidas);  

    for (int i = 0; i < 7; i++) // detecta se o remédio esta lá ou não  

    {  
        pinosRemedios[i] = remedios(i);  
        total_remedios_retirados -= remedios(i);  
    }  

    printf("hora %d",hora);  
    if(testar_remedios(hoje)) 
    {
      exibir_total_remedios_gaveta(); 
    } 
}  

void change_day_manual(void)   
{  
    tecla = le_teclado();  // Lê a tecla pressionada  
    if (tecla == '1')   
    {  // Se pressionar a tecla '1', avançar um dia  
        hoje = (hoje + 1) % 7; // Avança o dia  
    } else if (tecla == '2')   

    {  // Se pressionar a tecla '2', voltar um dia  
        hoje = (hoje == 0) ? 6 : hoje - 1; // Retrocede o dia  
    }  

    // Atualiza o display para mostrar o novo dia após ajuste  
    sprintf(dia_str, "Dia: %d", hoje) ;  
    lcd595_write(1, 0, dia_str); // Exibe o dia atualizado  
}  

// Programa Principal  

//-----------------------------------------------------------------------------------------------------------------------  

    //declarar hora e hoje // se a hora for maior que __ e o remedio não for retirado, um lembrente sera enviado  
// if(total_remedios_retirados != hoje) enviar_erro(); //detecta diferença no tanto de remedios e enviar erro  
//  else if (total_remedios_retirados == hoje && pinosRemedios[hoje] == 1) enviar_certo ();  
    //else if (total_remedios_retirados != hoje && pinosRemedios[hoje] != 1) enviar_lembrete ();  
     //led acesso + remedio retirado 
     //led apagado + remedio lá 
        // if ((hora > 10|| (hora == 10 && minuto > 10)) && pinosRemedios[hoje] == 1)  enviar_lembrete();   

void app_main(void)  
{  
    /////////////////////////////////////////////////////////////////////////////////////   Programa principal  

// a seguir, apenas informações de console, aquelas notas verdes no início da execução  

  ESP_LOGI(TAG, "Iniciando...");  
  ESP_LOGI(TAG, "Versão do IDF: %s", esp_get_idf_version());  

    // Inicializações de periféricos (manter assim) 

   ESP_ERROR_CHECK(nvs_flash_init());  

    // inicializar os IOs e teclado da placa  
  ioinit();        
  entradas = io_le_escreve(saidas); // Limpa as saídas e lê o estado das entradas  

    // inicializar o display LCD  
lcd595_init();  

   // lcd595_write(1,0,"COFRE IOT - v1.0");  
  ///  vTaskDelay(1000 / portTICK_PERIOD_MS);    
  //  lcd595_write(1,0,"Inicializando   ");  
 //   lcd595_write(2,0,"ADC             ");  

    // Inicializar o componente de leitura de entrada analógica  
    esp_err_t init_result = hcf_adc_iniciar();  
    if (init_result != ESP_OK)  
        ESP_LOGE("MAIN", "Erro ao inicializar o componente ADC personalizado"); 
  

  lcd595_write(2,0,"ADC / Wifi      ");  
   lcd595_write(1,13,".  ");  
   vTaskDelay(200 / portTICK_PERIOD_MS);  
    // Inicializar a comunicação IoT
wifi_init();      
ESP_ERROR_CHECK(wifi_connect_sta(SSID, PASSWORD, 10000));  

//lcd595_write(2,0,"C / Wifi / MQTT ");  
  //  vTaskDelay(100 / portTICK_PERIOD_MS);  
 //   lcd595_write(2,0,"Wifi / MQTT     ");  
  //  lcd595_write(1,13,".. ");  
   // vTaskDelay(200 / portTICK_PERIOD_MS);  
   // sprintf(mensa,"%s %d }}",subtopico_temp,temperatura);  
    //mqtt_app_start();  
    // inicializa driver de motor de passo com fins de curso nas entradas 6 e 7 da placa  

    lcd595_write(2,0,"i / MQTT / DRV  ");  
    vTaskDelay(100 / portTICK_PERIOD_MS); 
    lcd595_write(2,0,"MQTT / DRV      ");  
    lcd595_write(1,13,"...");  
    vTaskDelay(200 / portTICK_PERIOD_MS); 
    DRV_init(6,7);  

    // fecha a tampa, se estiver aberta  
    hcf_adc_ler(&adcvalor);                  
    adcvalor = adcvalor*360/4095; 
    // if(adcvalor>50) fechar();  

    lcd595_write(2,0,"TT / DRV / APP  ");  
    vTaskDelay(200 / portTICK_PERIOD_MS);  
    lcd595_write(2,0,"DRV / APP       "); 
    vTaskDelay(200 / portTICK_PERIOD_MS);  
    lcd595_write(2,0,"V / APP         "); 
    vTaskDelay(200 / portTICK_PERIOD_MS);  
    lcd595_write(2,0,"APP             ");  

     for(int i = 0; i < 10; i++)  
     {  
        lcd595_write(1,13,"   ");  
        vTaskDelay(200 / portTICK_PERIOD_MS);  
       lcd595_write(1,13,".  ");  
        vTaskDelay(200 / portTICK_PERIOD_MS);  
        lcd595_write(1,13,".. ");  
        vTaskDelay(200 / portTICK_PERIOD_MS);  
        lcd595_write(1,13,"...");  
        vTaskDelay(200 / portTICK_PERIOD_MS);  
    }  

    lcd595_clear();  

    // Inicializa o SNTP para obter a hora da internet  

    initialize_sntp();  
   initialize_rtc();  // Configuração do RTC (ainda que não precise fazer nada aqui) 

    // Espera alguns segundos para sincronização  

    vTaskDelay(2000 / portTICK_PERIOD_MS);  

    // Obtém e imprime a hora e o dia da semana uma vez na inicialização  
    obtain_time();  

    /////////////////////////////////////////////////////////////////////////////////////   Periféricos inicializados  

    /////////////////////////////////////////////////////////////////////////////////////   Início do ramo principal                      

    while (1)                                                                                                                          
    {                                                                                                                                  

        //_______________________________________________________________________________________________________________________________________________________ //  

        //-  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  - -  -  -  -  -  -  -  -  -  -  Escreva seu código aqui!!! //  

   //change_day_manual();  

    vTaskDelay(1000 / portTICK_PERIOD_MS);  // Delay de 1 segundo  
    detecta_remedios();  
     { 
    vTaskDelay(1000 / portTICK_PERIOD_MS);  // Delay de 1 segundo 
   obtain_time();  // Atualiza a hora e imprime novamente  
     }  
        //-  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  - -  -  -  -  -  -  -  -  -  -  Escreva seu só até aqui!!! //  
        //________________________________________________________________________________________________________________________________________________________//  
        vTaskDelay(200 / portTICK_PERIOD_MS);    // delay mínimo obrigatório, se retirar, pode causar reset do ESP  
    }  
 
    // caso erro no programa, desliga o módulo ADC  
    hcf_adc_limpar();  

    /////////////////////////////////////////////////////////////////////////////////////   Fim do ramo principal  


} 


 