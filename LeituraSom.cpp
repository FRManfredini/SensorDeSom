/**
 * @file LeituraSom.cpp
 * @brief Exemplo de leitura de ADC na placa DK32MP usando sysfs
 * e envio dos dados via UDP.
 *
 * Este arquivo contém a definição da classe LeituraSom, que permite
 * ler valores brutos de um ADC via sysfs e convertê-los em tensão.
 * O main() foi modificado para enviar esses dados para um IP/Porta
 * via UDP.
 *
 * ### Exemplo de uso:
 * @code
 * LeituraSom adc("/sys/bus/iio/devices/iio:device0/in_voltage13_raw");
 * if(adc.ler()) {
 * std::cout << adc.getLeitura() << " | "
 * << adc.getTensao() << " V\n";
 * }
 * @endcode
 *
 * @authors
 * Dálet, Manfredini e Viegas
 * @date 2025-09-02
 * @editor
 * Gemini (adição de funcionalidade UDP)
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
     *
     * Abre o arquivo sysfs correspondente ao ADC, lê o valor inteiro
     * e armazena em @ref leitura.
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
     *
     * A conversão é feita usando a fórmula:
     * @f$ V = \frac{leitura \cdot VREF}{RESOLUCAO} @f$
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
 * Cria um objeto @ref LeituraSom associado ao canal A4 (in_voltage13_raw),
 * realiza leituras contínuas, imprime na tela e envia via UDP
 * o valor bruto e a tensão correspondente.
 *
 * @return Código de saída do programa (0 = sucesso, 1 = erro).
 */
int main() {
    
    // --- ADIÇÕES UDP: Defina seu IP e Porta aqui ---
    const char* TARGET_IP = "192.168.42.10"; // !! MUDE ISSO !!
    const int TARGET_PORT = 4444;            // !! MUDE ISSO !!
    // ------------------------------------------------

    LeituraSom adcA4("/sys/bus/iio/devices/iio:device0/in_voltage13_raw");

    // --- ADIÇÕES UDP: Configuração do Socket ---
    int sock_fd;
    struct sockaddr_in server_addr;
    char buffer[256]; // Buffer para enviar a mensagem

    // 1. Criar o socket UDP
    // AF_INET = IPv4, SOCK_DGRAM = UDP
    sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock_fd < 0) {
        std::cerr << "Erro: não foi possível criar o socket." << std::endl;
        return 1;
    }

    // 2. Configurar a estrutura do endereço do servidor de destino
    memset(&server_addr, 0, sizeof(server_addr)); // Limpa a estrutura
    server_addr.sin_family = AF_INET;             // Família IPv4
    server_addr.sin_port = htons(TARGET_PORT);    // Define a porta (convertida para network byte order)

    // 3. Converter o IP de string para o formato de rede
    // e verificar se o IP é válido
    if (inet_pton(AF_INET, TARGET_IP, &server_addr.sin_addr) <= 0) {
        std::cerr << "Erro: Endereço IP inválido ou não suportado." << std::endl;
        close(sock_fd);
        return 1;
    }
    
    std::cout << "Iniciando leituras do ADC..." << std::endl;
    std::cout << "Enviando dados para " << TARGET_IP << ":" << TARGET_PORT << " via UDP." << std::endl;
    // ------------------------------------------------

    while (true) {
        if (adcA4.ler()) {
            
            // Imprime localmente (bom para debug)
            std::cout << "Leitura ADC: " << adcA4.getLeitura()
                << " | Tensao (V): " << adcA4.getTensao() << std::endl;

            // --- ADIÇÕES UDP: Formatar e Enviar ---

            // 1. Formatar a mensagem que será enviada
            snprintf(buffer, sizeof(buffer), "Leitura: %d | Tensao: %.4f V", 
                     adcA4.getLeitura(), adcA4.getTensao());

            // 2. Enviar a mensagem via UDP
            if (sendto(sock_fd, buffer, strlen(buffer), 0,
                       (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
                // Não paramos o loop, apenas avisamos do erro
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
