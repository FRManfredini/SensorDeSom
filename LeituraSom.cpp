/**
 * @file LeituraSom.cpp
 * @brief Exemplo de leitura de ADC na placa DK32MP usando sysfs
 * e envio dos dados via UDP usando protocolo JSON.
 *
 * (O restante da documentação Doxygen permanece igual)
 *
 * @authors
 * Dálet, Manfredini e Viegas
 * @date 2025-09-02
 * @editor
 * Gemini (modificação para protocolo JSON)
 */

#include <iostream>
#include <fstream>
#include <string>
#include <unistd.h> // usleep()

// --- ADIÇÕES UDP ---
#include <sys/socket.h>  // Para socket()
#include <netinet/in.h>  // Para sockaddr_in
#include <arpa/inet.h>   // Para inet_pton() e htons()
#include <cstring>       // Para memset() e strlen()
#include <cstdio>        // Para snprintf()
#include <ctime>         // Para time() - Opcional para timestamp

/**
 * @class LeituraSom
 * @brief Classe para leitura de valores do ADC via sysfs.
 *
 * (O restante da classe LeituraSom permanece idêntico)
 */
class LeituraSom {
private:
    std::string adc_path; ///< Caminho do arquivo do ADC no sysfs
    float VREF;           ///< Tensão de referência do ADC (Volts)
    int RESOLUCAO;        ///< Resolução máxima do ADC (ex.: 65535)
    int leitura;          ///< Último valor bruto lido do ADC

public:
    /**
     * @brief Construtor da classe LeituraSom.
     * @param path Caminho do arquivo do ADC no sysfs
     * @param vref Tensão de referência (padrão = 3.3 V)
     * @param resolucao Resolução máxima do ADC (padrão = 65535)
     */
    LeituraSom(const std::string& path, float vref = 3.3, int resolucao = 65535)
        : adc_path(path), VREF(vref), RESOLUCAO(resolucao), leitura(0) {
    }

    /**
     * @brief Realiza a leitura do valor bruto do ADC.
     * @return true se a leitura foi realizada com sucesso,
     * false caso contrário.
     */
    bool ler() {
        std::ifstream adc_file(adc_path);
        if (!adc_file.is_open()) {
            std::cerr << "Erro: não consegui abrir " << adc_path << std::endl;
            return false;
        }
        adc_file >> leitura;
        adc_file.close();
        return true;
    }

    /**
     * @brief Converte a última leitura para tensão (em Volts).
     * @return Valor em Volts correspondente à leitura.
     */
    float getTensao() const {
        return leitura * VREF / RESOLUCAO;
    }

    /**
     * @brief Retorna o último valor bruto lido do ADC.
     * @return Valor inteiro da leitura.
     */
    int getLeitura() const {
        return leitura;
    }
};

/**
 * @brief Função principal.
 *
 * (Descrição Doxygen do main)
 *
 * @return Código de saída do programa (0 = sucesso, 1 = erro).
 */
int main() {
    
    // --- ADIÇÕES UDP: Defina seu IP e Porta aqui ---
    const char* TARGET_IP = "192.168.42.10"; // !! MUDE ISSO para o IP do seu PC/Servidor !!
    const int TARGET_PORT = 4444;            // !! Use a mesma porta no Servidor !!
    const char* SENSOR_ID = "sensor_grupo_01_A4"; // Identificador do seu sensor
    // ------------------------------------------------

    LeituraSom adcA4("/sys/bus/iio/devices/iio:device0/in_voltage13_raw");

    // --- ADIÇÕES UDP: Configuração do Socket ---
    int sock_fd;
    struct sockaddr_in server_addr;
    char buffer[256]; // Buffer para enviar a mensagem

    // 1. Criar o socket UDP
    sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock_fd < 0) {
        std::cerr << "Erro: não foi possível criar o socket." << std::endl;
        return 1;
    }

    // 2. Configurar a estrutura do endereço do servidor de destino
    memset(&server_addr, 0, sizeof(server_addr)); 
    server_addr.sin_family = AF_INET;             
    server_addr.sin_port = htons(TARGET_PORT);    

    // 3. Converter o IP de string para o formato de rede
    if (inet_pton(AF_INET, TARGET_IP, &server_addr.sin_addr) <= 0) {
        std::cerr << "Erro: Endereço IP inválido ou não suportado." << std::endl;
        close(sock_fd);
        return 1;
    }
    
    std::cout << "Iniciando leituras do ADC..." << std::endl;
    std::cout << "Enviando dados para " << TARGET_IP << ":" << TARGET_PORT << " via UDP." << std::endl;

    while (true) {
        if (adcA4.ler()) {
            
            // (Impressão local para debug)
            std::cout << "Leitura ADC: " << adcA4.getLeitura()
                      << " | Tensao (V): " << adcA4.getTensao() << std::endl;

            // --- ADIÇÕES UDP: Formatar e Enviar ---

            // *** LINHA CORRIGIDA PARA USAR O PROTOCOLO JSON ***
            // 1. Formatar a mensagem como um objeto JSON
            //    Cumprindo os requisitos: id, valor, unidade e timestamp (opcional)
            
            // Opcional: Obter o timestamp atual (se o sistema tiver relógio)
            long timestamp = static_cast<long>(time(NULL)); 

            snprintf(buffer, sizeof(buffer), 
                     "{\"id\": \"%s\", \"valor\": %.4f, \"unidade\": \"V\", \"timestamp\": %ld}", 
                     SENSOR_ID, adcA4.getTensao(), timestamp);
            
            // NOTA: Se você não tiver um relógio confiável (time(NULL) retornar erro),
            // use esta versão sem o timestamp:
            /*
            snprintf(buffer, sizeof(buffer), 
                     "{\"id\": \"%s\", \"valor\": %.4f, \"unidade\": \"V\"}", 
                     SENSOR_ID, adcA4.getTensao());
            */
            // ******************************************************


            // 2. Enviar a mensagem via UDP
            if (sendto(sock_fd, buffer, strlen(buffer), 0,
                       (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
                std::cerr << "Erro: falha ao enviar pacote UDP." << std::endl;  
            }
            // ----------------------------------------
        }
        usleep(100000); // pausa de 100 ms
    }

    // --- ADIÇÕES UDP ---
    close(sock_fd); // Fecha o socket (embora o loop seja infinito)

    return 0;
}
