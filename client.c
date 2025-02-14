#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <time.h>

#define SERVER_IP "127.0.0.1"
#define PORT 8080
#define BUFFER_SIZE 1024
#define SEQ_SIZE 10
#define TIMEOUT_SEC 2
#define LOSS_PROBABILITY 10  // 10% de chance de perda

int main() {
    int sockfd;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE + SEQ_SIZE + 1];  
    socklen_t addr_len = sizeof(server_addr);
    int seq_num = 0;
    
    // Estatísticas
    int total_sent = 0, retransmitted = 0;

    srand(time(NULL));  // Inicializa gerador aleatório

    // Criar socket UDP
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Erro ao criar socket");
        exit(EXIT_FAILURE);
    }

    // Configuração do servidor
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);

    // Abrir arquivo para leitura
    FILE *file = fopen("arquivo.txt", "r");
    if (file == NULL) {
        perror("Erro ao abrir arquivo");
        exit(EXIT_FAILURE);
    }

    // Configurar timeout para retransmissão
    struct timeval timeout;
    timeout.tv_sec = TIMEOUT_SEC;
    timeout.tv_usec = 0;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    while (1) {
        // Lê dados do arquivo
        char data[BUFFER_SIZE];
        int bytes_read = fread(data, 1, BUFFER_SIZE, file);
        if (bytes_read <= 0) break;  // Fim do arquivo

        // Criar pacote com número de sequência
        snprintf(buffer, SEQ_SIZE + 1, "%09d", seq_num);
        memcpy(buffer + SEQ_SIZE, data, bytes_read);

        int ack_received = 0;
        char ack_buffer[SEQ_SIZE + 4];

        // Enviar e esperar ACK
        while (!ack_received) {
            // Simulação de perda de pacote (10% de chance de não enviar)
            if (rand() % 100 >= LOSS_PROBABILITY) {
                sendto(sockfd, buffer, SEQ_SIZE + bytes_read, 0, 
                       (struct sockaddr *)&server_addr, addr_len);
                total_sent++;
            }

            // Tenta receber ACK
            int recv_len = recvfrom(sockfd, ack_buffer, sizeof(ack_buffer), 0, 
                                    (struct sockaddr *)&server_addr, &addr_len);

            if (recv_len > 0) {
                ack_received = 1;  // ACK recebido com sucesso
            } else {
                retransmitted++;
            }
        }

        seq_num++;  // Próximo número de sequência
    }

    // Enviar pacote final indicando término
    sprintf(buffer, "%09dFIM", seq_num);
    sendto(sockfd, buffer, strlen(buffer), 0, 
           (struct sockaddr *)&server_addr, addr_len);

    // Relatório final
    printf("\n=== Relatório Cliente ===\n");
    printf("Pacotes enviados: %d\n", total_sent);
    printf("Pacotes retransmitidos: %d\n", retransmitted);
    printf("=========================\n");

    fclose(file);
    close(sockfd);
    return 0;
}
