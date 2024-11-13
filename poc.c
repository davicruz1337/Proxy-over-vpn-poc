#include <stdio.h>  // pra poder usar printf e coisas do tipo
#include <stdlib.h> // pra usar funções como malloc, free, etc
#include <unistd.h> // pra usar o close e outras
#include <string.h> // pra usar o memcpy e tal
#include <arpa/inet.h> // pra manipular ip e porta
#include <sys/socket.h> // pra usar socket

#define LISTEN_PORT 8080  // porta que vai escutar as conexões do cliente
#define FORWARD_IP "127.0.0.1"  // IP que vai receber o tráfego (o destino)
#define FORWARD_PORT 9090 // porta do destino onde o tráfego vai ser redirecionado

// função que vai redirecionar o tráfego
void forward_traffic(int client_socket) {
    int forward_socket;  // esse é o socket que a gente vai usar pra falar com o servidor de destino
    struct sockaddr_in forward_addr; // onde a gente vai guardar o endereço do servidor destino
    char buffer[1024];  // buffer pra armazenar o que a gente vai enviar/receber
    int bytes_read;  // vai contar quantos bytes a gente recebeu do cliente

    // criando socket pra enviar os dados ao destino
    forward_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (forward_socket < 0) {
        perror("erro ao criar socket do destino");
        return;
    }

    // configurando o destino (IP e porta)
    forward_addr.sin_family = AF_INET;
    forward_addr.sin_port = htons(FORWARD_PORT);  // usa a porta de destino
    forward_addr.sin_addr.s_addr = inet_addr(FORWARD_IP);  // define o IP de destino

    // tenta se conectar ao servidor de destino
    if (connect(forward_socket, (struct sockaddr *)&forward_addr, sizeof(forward_addr)) < 0) {
        perror("falha na conexão com o servidor de destino");
        close(forward_socket);
        return;
    }

    // começa a receber dados do cliente e enviar pro servidor de destino
    while ((bytes_read = recv(client_socket, buffer, sizeof(buffer), 0)) > 0) {
        send(forward_socket, buffer, bytes_read, 0); // manda os dados pro destino

        // se o servidor de destino responder, a gente manda de volta pro cliente
        bytes_read = recv(forward_socket, buffer, sizeof(buffer), 0);
        if (bytes_read > 0) {
            send(client_socket, buffer, bytes_read, 0); // manda a resposta de volta
        }
    }

    // fecha os sockets depois que terminar
    close(forward_socket);
    close(client_socket);
}

int main() {
    int server_socket, client_socket;  // server_socket vai escutar as conexões, client_socket vai ser a conexão com o cliente
    struct sockaddr_in server_addr, client_addr;  // onde guardamos os endereços
    socklen_t client_len = sizeof(client_addr);  // tamanho do endereço do cliente

    // cria o socket do servidor
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("erro ao criar o socket");
        return -1;
    }

    // configura o servidor pra escutar na porta LISTEN_PORT
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;  // escuta em qualquer IP
    server_addr.sin_port = htons(LISTEN_PORT); // usa a porta que definimos

    // associa o socket ao endereço (vincula o socket à porta e IP)
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("erro ao fazer bind no socket");
        close(server_socket);
        return -1;
    }

    // começa a escutar as conexões que chegam
    if (listen(server_socket, 5) < 0) {
        perror("erro ao escutar");
        close(server_socket);
        return -1;
    }

    printf("escutando na porta %d...\n", LISTEN_PORT);

    // aceitando conexões dos clientes
    while ((client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_len)) >= 0) {
        printf("cliente conectado de %s\n", inet_ntoa(client_addr.sin_addr));
        
        // redireciona o tráfego
        forward_traffic(client_socket);
    }

    if (client_socket < 0) {
        perror("erro ao aceitar conexão");
        close(server_socket);
        return -1;
    }

    // fecha o socket do servidor quando não precisar mais
    close(server_socket);
    return 0;
}
