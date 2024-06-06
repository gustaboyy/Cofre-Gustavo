//////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                       _              //
//               _    _       _      _        _     _   _   _    _   _   _        _   _  _   _          //
//           |  | |  |_| |\| |_| |\ |_|   |\ |_|   |_| |_| | |  |   |_| |_| |\/| |_| |  |_| | |   /|    //    
//         |_|  |_|  |\  | | | | |/ | |   |/ | |   |   |\  |_|  |_| |\  | | |  | | | |_ | | |_|   _|_   //
//                                                                                       /              //
//////////////////////////////////////////////////////////////////////////////////////////////////////////

/*
*   Programa básico para controle da placa durante a Jornada da Programação 1
*   Permite o controle das entradas e saídas digitais, entradas analógicas, display LCD e teclado. 
*   Cada biblioteca pode ser consultada na respectiva pasta em componentes
*   Existem algumas imagens e outros documentos na pasta Recursos
*   O código principal pode ser escrito a partir da linha 86
*/

// Área de inclusão das bibliotecas
//-----------------------------------------------------------------------------------------------------------------------
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "ioplaca.h"   // Controles das Entradas e Saídas digitais e do teclado
#include "lcdvia595.h" // Controles do Display LCD
#include "hcf_adc.h"   // Controles do ADC
#include "MP_hcf.h"    // Controles do ADC

// Área das macros
//-----------------------------------------------------------------------------------------------------------------------

#define num 6667  // definição da senha
#define LIMITE_TEN 3 // Limite de tentativas
#define BLOQ_SEG 100 // Tempo de bloqueio em segundos 

// Área de declaração de variáveis e protótipos de funções
//-----------------------------------------------------------------------------------------------------------------------

static const char *TAG = "Placa";
static uint8_t entradas, saidas = 0; // variáveis de controle de entradas e saídas
int controle = 0;
int senha = 0;
int tempo = 0;
int coluna = 0;
int tentativas = 0; // Contador de tentativas
uint32_t adcvalor = 0;
char exibir[30]; // Usado para exibição no LCD
uint32_t valorpoten;
int lockout = 0; // Variável para controle de bloqueio

// Funções e ramos auxiliares
//-----------------------------------------------------------------------------------------------------------------------
void exibir_lcd(void)
{
    sprintf(&exibir[0], " %" PRIu32 " ", valorpoten);
    lcd595_write(1, 2, &exibir[0]);
}

void abrir_cofre(void)
{
    int i;
    for (i = 0; i < 1964; i = valorpoten)
    {
        rotacionar_DRV(1, 10, saidas);
        hcf_adc_ler(&valorpoten);
    }
}

void fechar_cofre(void)
{
    int i;
    for (i = valorpoten; i > 400; i = valorpoten)
    {
        rotacionar_DRV(0, 10, saidas);
        hcf_adc_ler(&valorpoten);
    }
}

// Programa principal
//-----------------------------------------------------------------------------------------------------------------------

void app_main(void)
{
    /////////////////////////////////////////////////////////////////////////////////////   Programa principal
    ESP_LOGI(TAG, "Iniciando...");
    ESP_LOGI(TAG, "Versão do IDF: %s", esp_get_idf_version());

    /////////////////////////////////////////////////////////////////////////////////////   Inicializações de periféricos (manter assim)
    
    ioinit();      
    entradas = io_le_escreve(saidas); // Limpa as saídas e lê o estado das entradas

    lcd595_init();
    lcd595_write(1, 1, "   Cofre   "); // Nome 
    lcd595_write(2, 1, "  gustafarmaco  ");
    
    esp_err_t init_result = hcf_adc_iniciar();
    if (init_result != ESP_OK) {
        ESP_LOGE("MAIN", "Erro ao inicializar o componente ADC personalizado");
    }

    vTaskDelay(1000 / portTICK_PERIOD_MS); 
    lcd595_clear();

    /////////////////////////////////////////////////////////////////////////////////////   Periféricos inicializados
    /////////////////////////////////////////////////////////////////////////////////////   Início do ramo principal

    while (1)
    {
        // Bloqueio temporário
        if (lockout > 0) {
            lockout--;
            sprintf(exibir, "Bloqueado: %d s", lockout / 10);
            lcd595_write(1, 0, exibir);
            vTaskDelay(100 / portTICK_PERIOD_MS);
            continue;
        }

        char tecla = le_teclado();
        hcf_adc_ler(&adcvalor);
        adcvalor = adcvalor * 360 / 4095;

        if (tecla >= '0' && tecla <= '9')
        {
            senha = senha * 10 + (tecla - '0');
            controle++;
            
            switch (controle)
            {
                case 1: lcd595_write(2, 0, "[*] [ ] [ ] [ ]");
                    break;
                case 2: lcd595_write(2, 0, "[*] [*] [ ] [ ]");
                    break;
                case 3: lcd595_write(2, 0, "[*] [*] [*] [ ]");
                    break;
                case 4: lcd595_write(2, 0, "[*] [*] [*] [*]");
                    break;
                default: lcd595_write(2, 0, "erro 2");
                    break;
            }
        }
        else if (tecla == 'C')
        {
            senha = 0;
            controle = 0;
        }

        if (controle == 0)
        {
            lcd595_write(1, 1, "Insira a senha:");
        }
        else if (controle < 4)
        {
            //  aguardando mais dígitos
        }
        else if (controle == 4)
        {
            if (senha == num)
            {
                lcd595_clear();
                lcd595_write(1, 1, "Abrindo...");
                abrir_cofre();
                controle = 5;
                lcd595_clear();
                lcd595_write(1, 1, "Aberto");
                tempo = 100; // 10 segundos 
                tentativas = 0; // Reseta tentativas 
            }
            else
            {
                tentativas++;
                if (tentativas >= LIMITE_TEN) {
                    lockout = BLOQ_SEG * 10; // Bloqueia por 10s
                    lcd595_clear();
                    lcd595_write(1, 1, "Bloqueado 1 min");
                } else {
                    lcd595_clear();
                    lcd595_write(1, 1, "bye ladrao");
                }
                senha = 0;
                controle = 0;
            }
        }

        if (controle == 5)
        {
            if (tempo > 0) {
                tempo--;
                sprintf(exibir, "fechando em %d s ", tempo / 10);
                lcd595_write(1, 0, exibir);
            } else {
                lcd595_clear();
                lcd595_write(1, 1, "Fechando...");
                fechar_cofre();
                controle = 0;
                senha = 0;
                lcd595_clear();
                lcd595_write(1, 1, "Cofre Fechado");
            }
        }

        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
    
    hcf_adc_limpar();
}


