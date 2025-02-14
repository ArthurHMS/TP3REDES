#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <time.h>

#define PORT 8080
#define BUFFER_SIZE 1024  // Pacotes de 1024 bytes
#define SEQ_SIZE 10       // Número de sequência ocupa 10 bytes
#define LOSS_PROBABILITY 10  // 10% de perda simulada de ACKs

int main() {
    int sockfd;
    struct sockaddr_in server_addr, client_addr;
    char buffer[BUFFER_SIZE + SEQ_SIZE + 1]; // Buffer maior para número de sequência
    socklen_t addr_len = sizeof(client_addr);
    int last_seq_num = -1;

    // Estatísticas
    int total_received = 0, duplicate_packets = 0, lost_acks = 0;

    srand(time(NULL));  // Inicializa gerador aleatório

    // Abre o arquivo em modo binário ("wb")
    FILE *file = fopen("recebido.txt", "wb");
    if (file == NULL) {
        perror("Erro ao abrir arquivo");
        exit(EXIT_FAILURE);
    }

    // Criando socket UDP
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Erro ao criar socket");
        exit(EXIT_FAILURE);
    }

    // Configuração do servidor
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // Ligando o socket à porta
    if (bind(sockfd, (const struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Erro ao fazer bind");
        exit(EXIT_FAILURE);
    }

    while (1) {
        // Recebe pacote do cliente
        int recv_len = recvfrom(sockfd, buffer, BUFFER_SIZE + SEQ_SIZE, 0,
                                (struct sockaddr *)&client_addr, &addr_len);
        if (recv_len < 0) continue;

        buffer[recv_len] = '\0';

        // Extrai número de sequência
        int seq_num;
        sscanf(buffer, "%d", &seq_num);
        char *data = buffer + SEQ_SIZE;  // Pula os primeiros SEQ_SIZE caracteres

        // Simular perda de pacotes recebidos no servidor (10% de chance)
        if (rand() % 100 < LOSS_PROBABILITY) {
            continue;  // Ignora o pacote, fazendo o cliente retransmiti-lo
        }

        // Verifica se o pacote contém "FIM"
        if (strstr(buffer, "FIM") != NULL) {
            printf("\n[Servidor] Recebido sinal de término. Finalizando...\n");
            break;  // Sai do loop sem escrever "FIM"
        }

        // Se o pacote não é duplicado, escreve no arquivo
        if (seq_num > last_seq_num) {
            fwrite(data, 1, recv_len - SEQ_SIZE, file);
            fflush(file);  
            last_seq_num = seq_num;
            total_received++;
        } else {
            duplicate_packets++;
        }

        // Simulação de perda de ACK (10% de chance)
        if (rand() % 100 < LOSS_PROBABILITY) {
            lost_acks++;
            continue;  // Pula o envio do ACK
        }

        // Envia ACK
        char ack_msg[SEQ_SIZE + 4];
        sprintf(ack_msg, "%d ACK", seq_num);
        sendto(sockfd, ack_msg, strlen(ack_msg), 0, 
               (struct sockaddr *)&client_addr, addr_len);
    }

    // Relatório final
    printf("\n=== Relatório Servidor ===\n");
    printf("Pacotes recebidos corretamente: %d\n", total_received);
    printf("Pacotes duplicados descartados: %d\n", duplicate_packets);
    printf("ACKs perdidos: %d\n", lost_acks);
    printf("===========================\n");

    fclose(file);
    close(sockfd);
    return 0;
}
