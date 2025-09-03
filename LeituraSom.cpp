/**
 * @file LeituraSom.cpp
 * @brief Exemplo de leitura de ADC na placa DK32MP usando sysfs.
 *
 * Este arquivo cont�m a defini��o da classe LeituraSom, que permite
 * ler valores brutos de um ADC via sysfs e convert�-los em tens�o.
 *
 * ### Exemplo de uso:
 * @code
 * LeituraSom adc("/sys/bus/iio/devices/iio:device0/in_voltage13_raw");
 * if(adc.ler()) {
 *     std::cout << adc.getLeitura() << " | "
 *               << adc.getTensao() << " V\n";
 * }
 * @endcode
 *
 * @authors
 * D�let, Manfredini e Viegas
 * @date 2025-09-02
 */

#include <iostream>
#include <fstream>
#include <string>
#include <unistd.h> // usleep()

 /**
  * @class LeituraSom
  * @brief Classe para leitura de valores do ADC via sysfs.
  *
  * Esta classe encapsula a leitura de um ADC exposto como arquivo no Linux,
  * permitindo obter o valor bruto e convert�-lo para tens�o.
  *
  * Tamb�m pode ser usada em documenta��o Doxygen com suporte a Graphviz,
  * para gera��o de diagramas de classes e chamadas.
  *
  * @note Requer acesso a arquivos do diret�rio `/sys/bus/iio/devices/`.
  */
class LeituraSom {
private:
    std::string adc_path; ///< Caminho do arquivo do ADC no sysfs
    float VREF;           ///< Tens�o de refer�ncia do ADC (Volts)
    int RESOLUCAO;        ///< Resolu��o m�xima do ADC (ex.: 65535)
    int leitura;          ///< �ltimo valor bruto lido do ADC

public:
    /**
     * @brief Construtor da classe LeituraSom.
     * @param path Caminho do arquivo do ADC no sysfs
     * @param vref Tens�o de refer�ncia (padr�o = 3.3 V)
     * @param resolucao Resolu��o m�xima do ADC (padr�o = 65535)
     */
    LeituraSom(const std::string& path, float vref = 3.3, int resolucao = 65535)
        : adc_path(path), VREF(vref), RESOLUCAO(resolucao), leitura(0) {
    }

    /**
     * @brief Realiza a leitura do valor bruto do ADC.
     * @return true se a leitura foi realizada com sucesso,
     *         false caso contr�rio.
     *
     * Abre o arquivo sysfs correspondente ao ADC, l� o valor inteiro
     * e armazena em @ref leitura.
     */
    bool ler() {
        std::ifstream adc_file(adc_path);
        if (!adc_file.is_open()) {
            std::cerr << "Erro: n�o consegui abrir " << adc_path << std::endl;
            return false;
        }
        adc_file >> leitura;
        adc_file.close();
        return true;
    }

    /**
     * @brief Converte a �ltima leitura para tens�o (em Volts).
     * @return Valor em Volts correspondente � leitura.
     *
     * A convers�o � feita usando a f�rmula:
     * @f$ V = \frac{leitura \cdot VREF}{RESOLUCAO} @f$
     */
    float getTensao() const {
        return leitura * VREF / RESOLUCAO;
    }

    /**
     * @brief Retorna o �ltimo valor bruto lido do ADC.
     * @return Valor inteiro da leitura.
     */
    int getLeitura() const {
        return leitura;
    }
};

/**
 * @brief Fun��o principal.
 *
 * Cria um objeto @ref LeituraSom associado ao canal A4 (in_voltage13_raw),
 * realiza leituras cont�nuas e imprime na tela o valor bruto e a
 * tens�o correspondente.
 *
 * @return C�digo de sa�da do programa (0 = sucesso).
 */
int main() {
    LeituraSom adcA4("/sys/bus/iio/devices/iio:device0/in_voltage13_raw");

    while (true) {
        if (adcA4.ler()) {
            std::cout << "Leitura ADC: " << adcA4.getLeitura()
                << " | Tensao (V): " << adcA4.getTensao() << std::endl;
        }
        usleep(100000); // pausa de 100 ms
    }

    return 0;
}