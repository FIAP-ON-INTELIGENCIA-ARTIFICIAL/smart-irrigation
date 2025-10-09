#include <stdio.h>
#include <stdlib.h>

int main() {
    // Força o terminal a usar UTF-8
    system("chcp 65001");
   
    // Inteiro
    int total_leituras = 10;

    // Strings
    char primeira_leitura[] = "2024-10-06 08:00:00";
    char ultima_leitura[] = "2024-10-06 08:45:00";
    char tendencia[] = "Estável";

    // Valores float
    float ultima_umidade = 66.9;
    float media_total = 66.0;
    float media_ultimas5 = 66.1;
    float umidade_min = 63.2;
    float umidade_max = 68.9;
    float valor_recomendado = 0.0; // Será lido do arquivo

    // ===== LEITURA DO ARQUIVO =====
    char caminho_arquivo[] = "C:\\Users\\jvmp2\\Documents\\FIAP\\FASE2\\Atividade1\\smart-irrigation\\data\\valor_ccpp.txt";
    FILE *arquivo = fopen(caminho_arquivo, "r");
    
    if (arquivo != NULL) {
        // Lê o valor do arquivo
        if (fscanf(arquivo, "%f", &valor_recomendado) == 1) {
            printf("Valor lido do arquivo Python: %.1f\n\n", valor_recomendado);
        } else {
            printf("Erro ao ler o valor do arquivo.\n\n");
            valor_recomendado = 0.0;
        }
        fclose(arquivo);
    } else {
        printf("Erro ao abrir o arquivo: %s\n", caminho_arquivo);
        printf("Usando valor padrão.\n\n");
        valor_recomendado = 66.1; // Valor padrão caso o arquivo não exista
    }

    // Impressão dos resultados
    printf("===== ANÁLISE DE UMIDADE DO SOLO =====\n\n");
    printf("Total de leituras: %d\n", total_leituras);
    printf("Primeira leitura: %s\n", primeira_leitura);
    printf("Última leitura: %s\n\n", ultima_leitura);

    printf("ESTATÍSTICAS:\n");
    printf("Última leitura: %.1f%%\n", ultima_umidade);
    printf("Média total: %.1f%%\n", media_total);
    printf("Média (últimas 5 leituras): %.1f%%\n", media_ultimas5);
    printf("Umidade mínima: %.1f%%\n", umidade_min);
    printf("Umidade máxima: %.1f%%\n", umidade_max);
    printf("Tendência: %s\n\n", tendencia);

    printf("VALOR RECOMENDADO PARA C/C++ (do Python): %.1f\n\n", valor_recomendado);

    return 0;
}