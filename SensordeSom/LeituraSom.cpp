/**
 * @file LeituraSom.cpp
 * @brief Leitura de ADC na placa DK32MP usando sysfs
 *
 * Este arquivo cont�m a classe LeituraSom, que permite ler valores brutos
 * de um ADC e convert�-los em tens�o em Volts.
 *
 * Exemplo de uso:
 * @code
 * LeituraSom adc("/sys/bus/iio/devices/iio:device0/in_voltage13_raw");
 * if(adc.ler()) {
 *     std::cout << adc.getLeitura() << " | " << adc.getTensao() << " V\n";
 * }
 * @endcode
 *
 * @author
 * D�let, Manfredini e Viegas
 * @date 2025-09-02
 */

#include <iostream>
#include <fstream>
#include <string>
#include <unistd.h> // usleep()

 /**
  * @class LeituraSom
  * @brief Classe para leitura de valores do ADC via sysfs
  *
  * Esta classe encapsula a leitura de um ADC representado como arquivo no Linux.
  * Permite ler o valor bruto e convert�-lo em tens�o. Tamb�m � compat�vel com
  * Doxygen para gerar diagramas de classe e gr�ficos de chamadas.
  *
  * @note Esta classe acessa diretamente arquivos em /sys/bus/iio/devices/
  */
class LeituraSom {
private:
    std::string adc_path; ///< Caminho do arquivo do ADC (sysfs)
    float VREF;           ///< Tens�o de refer�ncia (em Volts)
    int RESOLUCAO;        ///< Resolu��o m�xima do ADC (valor m�ximo)
    int leitura;          ///< �ltimo valor lido do ADC

public:
    /**
     * @brief Construtor da classe LeituraSom
     * @param path Caminho do arquivo do ADC no sysfs
     * @param vref Tens�o de refer�ncia (padr�o = 3.3 V)
     * @param resolucao Resolu��o m�xima do ADC (padr�o = 65535)
     */
    LeituraSom(const std::string& path, float vref = 3.3, int resolucao = 65535)
        : adc_path(path), VREF(vref), RESOLUCAO(resolucao), leitura(0) {
    }

    /**
     * @brief L� o valor bruto do ADC a partir do arquivo
     * @return true se a leitura foi realizada com sucesso, false caso contr�rio
     *
     * Abre o arquivo do ADC, l� o valor inteiro e armazena na vari�vel leitura.
     * Fecha o arquivo ap�s a leitura.
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
     * @brief Converte a �ltima leitura para tens�o (em Volts)
     * @return Valor em Volts correspondente � leitura
     *
     * A tens�o � calculada usando a f�rmula:
     * @f$ V = \frac{leitura \cdot VREF}{RESOLUCAO} @f$
     */
    float getTensao() const {
        return leitura * VREF / RESOLUCAO;
    }

    /**
     * @brief Retorna a �ltima leitura bruta do ADC
     * @return Valor inteiro correspondente � leitura
     */
    int getLeitura() const {
        return leitura;
    }
};

/**
 * @brief Fun��o principal
 *
 * Cria um objeto LeituraSom associado ao canal A4 (in_voltage13_raw)
 * e exibe continuamente a leitura bruta e a tens�o correspondente.
 *
 * @return C�digo de sa�da do programa (0 = sucesso)
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