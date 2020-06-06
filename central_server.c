#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdio_ext.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <signal.h>

typedef struct usuario Usuario;
typedef struct parametro Parametro;
typedef struct infoRede InfoRede;
typedef struct infoUsuario infoUsuario;

struct infoRede{
    char ip[32];
    int porta;
};

struct infoUsuario{
    char numero[20];
    int porta;
};

struct usuario{
    char numero[20];
    InfoRede rede;

    Usuario *prox;
};

struct parametro{
	int socketControle;
	struct sockaddr_in cliente;
};

Usuario *listaUsuarios = NULL;

void loginDeUsuario(char numero[], char ip[], int porta, Usuario **usuarios);
InfoRede requisitaUsuario(char numero[], Usuario *usuarios);
void saidaDeUsuario(char numero[], Usuario *usuarios);
void *atenderCliente(void *parametros);
void encerraServidor();

void main(int argc, char **argv){

    struct sockaddr_in client; 
    struct sockaddr_in server; 
    int namelen, tp;
    pthread_t thread;
    Parametro parametros;
    int socketControle = 0;
    int socketAux = 0;
    int porta;
    
    if(argc != 2){
        fprintf(stderr, "Use: %s porta\n", argv[0]);
        exit(1);
    }

    porta = atoi(argv[1]);

    signal(SIGINT, encerraServidor);

    if ((socketControle = socket(PF_INET, SOCK_STREAM, 0)) < 0){
        exit(0);
    }

    server.sin_family = AF_INET;   
    server.sin_port   = htons(porta);      
    server.sin_addr.s_addr = INADDR_ANY;

    if (bind(socketControle, (struct sockaddr *)&server, sizeof(server)) < 0){
        perror("bind()");
        exit(0);
    }

    if (listen(socketControle, 1) != 0){
        perror("listen()");
        exit(0);
    }

    printf("Servidor aberto na porta %d.\n", ntohs(server.sin_port));

    while(1){

        if ((socketAux = accept(socketControle, (struct sockaddr *) &client, (socklen_t *) &namelen)) == -1){
            perror("accept()");
            exit(0);
        }
 
        parametros.socketControle = socketAux;
        parametros.cliente = client;

        tp = pthread_create(&thread, NULL, &atenderCliente, (void*)&parametros);

        if (tp) {
            printf("ERRO: impossivel criar thread.\n");
            exit(0);
        }

        printf("Thread atendente criada.\n");	
        pthread_detach(thread);
    };
    
    close(socketControle);
}

void loginDeUsuario(char numero[], char ip[], int porta, Usuario **usuarios){

    int novoUsuario = 1;

    while(*usuarios != NULL){

        if(strcmp((*usuarios)->numero, numero) == 0){
            (*usuarios)->rede.porta = porta;
            strcpy((*usuarios)->rede.ip, ip); 
            novoUsuario = 0;
            break;
        }

        usuarios = &(*usuarios)->prox;
    }

    if(novoUsuario){
        Usuario *aux;
        aux = (Usuario*)malloc(sizeof(Usuario));
        strcpy(aux->numero, numero);
        strcpy(aux->rede.ip, ip);
        aux->rede.porta = porta;
        aux->prox = NULL;

        *usuarios = aux;
    }
}

void saidaDeUsuario(char numero[], Usuario *usuarios){

    while(usuarios != NULL){

        if(strcmp(usuarios->numero, numero) == 0){
            usuarios->rede.porta = 0;
            strcpy(usuarios->rede.ip, "");   
            break;
        }

        usuarios = usuarios->prox;
    }
}

InfoRede requisitaUsuario(char numero[], Usuario *usuarios){

    while(usuarios != NULL){

        if(strcmp(usuarios->numero, numero) == 0){
            return usuarios->rede;
        }

        usuarios = usuarios->prox;
    }
}

void *atenderCliente(void *parametros){

    int socket = ((Parametro*)parametros)-> socketControle;
    int op;
    char numero[20];
    InfoRede redeRet;
    infoUsuario infoU;

    recv(socket, &op, sizeof(op), 0);

    if(op == 0){
        printf("Login de usuario\n");
        if (recv(socket, &infoU, sizeof(infoU), 0) <= 0){
            return 0;
        }
        loginDeUsuario(infoU.numero, inet_ntoa(((Parametro*)parametros)-> cliente.sin_addr), infoU.porta, &listaUsuarios);
    }
    else if(op == 1){
        redeRet.porta = 0;
        printf("Requisicao de usuario\n");
        if (recv(socket, &numero, sizeof(numero), 0) <= 0){
            return 0;
        }
        redeRet = requisitaUsuario(numero, listaUsuarios);

        send(socket, &redeRet, sizeof(redeRet), 0);
    }
    else if(op == 2){
        printf("Saida de usuario\n");
        if (recv(socket, &numero, sizeof(numero), 0) <= 0){
            return 0;
        }
        saidaDeUsuario(numero, listaUsuarios);
    }

    close(socket);
    printf("Thread atendente encerrada.\n");
    pthread_exit(NULL);
}

void encerraServidor(){
    system("clear");
    exit(1);
}