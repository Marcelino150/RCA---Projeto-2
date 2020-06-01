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
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <signal.h>
#include <termios.h>
#include <string.h>
#include <pthread.h>

#define TAM_BLC 65536
#define PASTA "Armazenamento de "
#define h_addr h_addr_list[0]

typedef struct contato Contato;
typedef struct grupo Grupo;
typedef struct infoArq InfoArq;
typedef struct mensagem Mensagem;
typedef struct msgnvizualiada MsgNVizualiada;
typedef struct infoRede InfoRede;
typedef struct parametro Parametro;
typedef struct minhaInfo MinhaInfo;

struct infoRede{
    char ip[32];
    int porta;
};

struct contato{
    char nome[32];
    char numero[20];
    MsgNVizualiada *listaMsgs;
    InfoRede rede;
    Contato *prox;
};

struct grupo{
    char nome[32];
    Contato *participantes;

    Grupo *prox;
};

struct msgnvizualiada{
    char msg[128];

    MsgNVizualiada *prox;
};

struct infoArq{
    int tamanho;
    char nomeArq[128];
};

struct mensagem{
    char chat[20];
    char msg[128];
    char numRemetente[20];
};

struct parametro{
	int socketReceptor;
	struct sockaddr_in cliente;
};

struct minhaInfo{
    char meuNumero[20];
    int porta;
};

int adicionaContato(Contato **listaDeContatos);
void criaGrupo(Grupo **grupos);
Contato *listaContatos(Contato *listaDeContatos);
Grupo *listaGrupos(Grupo *grupos);
void acessaConversa(char nomeConversa[], Contato *participantes, int tipoConversa);
void acessaGrupo(Grupo *grupo, Contato *participantes);
int adicionaAoGrupo(Contato **participantes, Contato *listaDeContatos);
Contato *buscaContato(Contato *contato, int indice);
Grupo *buscaGrupo(Grupo *grupo, int indice);
int exibeContatos(Contato *listaDeContatos, int tipoExibicao);
int exibeGrupos(Grupo *grupos);
Contato *buscaContatoPorNumero(Contato *listaDeContatos, char numero[]);
void *threadReceptora(void *parametros);
int contaNaoVizualizadas(MsgNVizualiada *listaMsgs);
void exibeNaoVizualizadas(MsgNVizualiada *listaMsgs);
void adicionaNaoVizualizada(char msg[], MsgNVizualiada **listaMsgs);
void limpaNaoVizualizadas(MsgNVizualiada **listaMsgs);
int criarSocket(int *porta, int *namelen);
int criarSocketServidor(char *ip, char *porta);
void encerraCliente();
void fechaConexoes();
void *esperaUsuarios();

pthread_mutex_t locker;
char chatAtual[20] = "";
struct sockaddr_in server;
MinhaInfo info;
int namelen;
char nomeServidor[20];
char portaServidor[20];
int *socketEmissor = NULL;
int nParticipantes = 1;
Contato *contatos = NULL;

int main(int argc, char **argv){

    int op = 0;
    Grupo *grupos = NULL;
    Contato *retContato;
    Grupo *retGrupo;
    int td;
    pthread_t thread;
    int opServidor;
    int s;
    int ret;

    if(argc != 3){
        fprintf(stderr, "Use: %s hostname porta\n", argv[0]);
        exit(1);
    }

    signal(SIGINT, encerraCliente);

    pthread_mutex_init(&locker,NULL);
    pthread_mutex_lock(&locker);

    printf("Meu numero: ");
    fgets(info.meuNumero, sizeof(info.meuNumero), stdin);
    info.meuNumero[strlen(info.meuNumero)-1] = '\0';

    strcpy(nomeServidor, argv[1]);
    strcpy(portaServidor, argv[2]);

    s = criarSocketServidor(nomeServidor, portaServidor);
    if(connect(s, (struct sockaddr *)&server, sizeof(server)) < 0){
        perror("connect()");
        return 0;
    };

    td = pthread_create(&thread, NULL, &esperaUsuarios, NULL);
    pthread_mutex_lock(&locker);

    opServidor = 0;
    if (send(s, &opServidor, sizeof(opServidor), 0) <= 0){
        perror("send()");
        return 0;
    }

    if (send(s, &info, sizeof(info), 0) <= 0){
        perror("send()");
        return 0;
    }

    pthread_mutex_unlock(&locker);
    close(s);

    system("clear");

    do{
        printf("-- MENU --\n\n");
        printf("[1] Adicionar contato\n");
        printf("[2] Criar grupo\n");
        printf("[3] Contatos\n");
        printf("[4] Grupos\n");
        printf("[5] Sair\n");
        printf("\n>> ");
        scanf("%d", &op);

        switch(op){
        
            case 1:
                system("clear");
                ret = adicionaContato(&contatos);
                if(ret == 1){
                    printf("\nContato adicionado com sucesso! Pressione Enter para continuar...");
                }
                __fpurge(stdin);
                getchar();
                break;

            case 2:
                system("clear");
                criaGrupo(&grupos);
                __fpurge(stdin);
                printf("\nGrupo criado com sucesso! Pressione Enter para continuar...");
                getchar();
                break;

            case 3:
                system("clear");
                if((retContato = listaContatos(contatos))){         
                    acessaConversa(retContato->nome,retContato, 1);
                };

                break;
            
            case 4:
                system("clear");
                if((retGrupo = listaGrupos(grupos))){
                    acessaGrupo(retGrupo,contatos);
                }
                break;

        }

        system("clear");
    }while(op != 5);
}

InfoRede buscaEndereco(char *numero){

    int s, opServidor = 1;
    InfoRede rede;

    s = criarSocketServidor(nomeServidor, portaServidor);
    if(connect(s, (struct sockaddr *)&server, sizeof(server)) < 0){
        perror("connect()");
    };

    if (send(s, &opServidor, sizeof(opServidor), 0) <= 0){
        perror("send()");
    }

    if(send(s, numero, strlen(numero)+1, 0) <= 0){
        perror("send()");
    };

    if (recv(s, &rede, sizeof(rede), 0) <= 0){
        perror("send()");
    }
    close(s);

    return rede;
}

int adicionaContato(Contato **listaDeContatos){

    char nome[32];
    char numero[20];

    printf("--ADICIONAR CONTATO--\n\n");

    __fpurge(stdin);
    printf("Nome: ");
    fgets(nome, sizeof(nome), stdin);
    nome[strlen(nome)-1] = '\0';

    __fpurge(stdin);
    printf("Numero: ");
    fgets(numero, sizeof(numero), stdin);
    numero[strlen(numero)-1] = '\0';

    while(*listaDeContatos != NULL && strcmp((*listaDeContatos)->nome, nome) < 0){
        listaDeContatos = &(*listaDeContatos)->prox;
    }

    Contato *aux;
    aux = (Contato*)malloc(sizeof(Contato));
    strcpy(aux->nome, nome);
    strcpy(aux->numero, numero);
    aux->listaMsgs = NULL;
    aux->prox = *listaDeContatos;

    *listaDeContatos = aux;

    return 1;
}

void criaGrupo(Grupo **grupos){
    char nome[32];

    printf("-- CRIAR GRUPO --\n\n");

    __fpurge(stdin);
    printf("Nome do grupo: ");
    fgets(nome, sizeof(nome), stdin);
    nome[strlen(nome)-1] = '\0';

    while(*grupos != NULL && strcmp((*grupos)->nome, nome) < 0){
        grupos = &(*grupos)->prox;
    }

    Grupo *aux;
    aux = (Grupo*)malloc(sizeof(Grupo));
    strcpy(aux->nome, nome);
    aux->prox = *grupos;

    *grupos = aux;
}

Contato *listaContatos(Contato *listaDeContatos){

    int quant, op;

    do{
        system("clear");
        printf("-- CONTATOS --\n\n");
        quant = exibeContatos(listaDeContatos, 1);

        printf("\n[0] Voltar\n");
        printf("\n>> ");
        scanf("%d",&op);

        if(op > 0 && op <= quant){
            return buscaContato(listaDeContatos, op);
        }

    }while(op != 0);

    return NULL;
}

Grupo *listaGrupos(Grupo *grupos){

    int quant, op;

    do{
        system("clear");
        printf("-- GRUPOS --\n\n");
        quant = exibeGrupos(grupos);

        printf("\n[0] Voltar\n");
        printf("\n>> ");
        scanf("%d",&op);

        if(op > 0 && op <= quant){
            return buscaGrupo(grupos, op);
        }

    }while(op != 0);
}

void print(char msg[]){

    pthread_mutex_lock(&locker);
    printf("\0337\n\033[2F");
    printf("\033[L%s",msg);
    printf("\0338\033[B");
    fflush(stdout);
    pthread_mutex_unlock(&locker);
}

void *threadReceptora(void *parametros){

    char caminho[128], numero[30], toPrint[256];
    int bRecebidos = 0, op;
    FILE *arquivo;
    InfoArq infoArquivo;
    Mensagem recebido;
    void *bloco;
    Contato *remetente;
    struct sockaddr_in client = ((Parametro*)parametros)-> cliente;
    int socketReceptor = ((Parametro*)parametros)-> socketReceptor;

    strcpy(caminho, PASTA);
    sprintf(numero,"%s/",info.meuNumero);
    strcat(caminho, numero);

    int tam;

    do{
        bzero(&op, sizeof(op));
        if (recv(socketReceptor, &op, sizeof(op), 0) <= 0){
            return 0;
        }

        if(op == 1){
            bzero(&recebido, sizeof(recebido));
            if ((tam = recv(socketReceptor, &recebido, sizeof(recebido), 0)) <= 0){
                return 0;
            }

            fflush(stdout);
            remetente = buscaContatoPorNumero(contatos, recebido.numRemetente);
            if(strcmp(chatAtual, recebido.numRemetente) == 0 && remetente != NULL){
                sprintf(toPrint,"[%s] %s",remetente->nome,recebido.msg);
                print(toPrint);
            }
            else if(remetente != NULL){
                sprintf(toPrint,"[%s] %s",remetente->nome,recebido.msg);
                adicionaNaoVizualizada(toPrint, &(remetente->listaMsgs));
            }
        }
        else if(op == 2){

            if (recv(socketReceptor, &info, sizeof(info), 0) <= 0){
                return 0;
            }

            mkdir(caminho, S_IRWXU | S_IRWXG | S_IRWXO);
            strcat(caminho, infoArquivo.nomeArq);
            arquivo = fopen(caminho,"wb");
            bloco = malloc(TAM_BLC);

            do{
                if ((bRecebidos = recv(socketReceptor, bloco, TAM_BLC, 0)) <= 0){
                    return 0;
                }

                infoArquivo.tamanho -= bRecebidos;

            }while(fwrite(bloco, 1, bRecebidos, arquivo) >= 0 && infoArquivo.tamanho > 0);

            fclose(arquivo);
            free(bloco);          
        }

    }while(op != 0);

    pthread_exit(NULL);
}

int conectarUsuario(InfoRede redeUsuario){

    unsigned short port;             
    struct hostent *hostnm;    
    struct sockaddr_in meuserver;
    int socketE = 0;

    hostnm = gethostbyname(redeUsuario.ip);
    if (hostnm == (struct hostent *) 0){
        fprintf(stderr, "Gethostbyname failed\n");
        exit(2);
    }

    meuserver.sin_family      = AF_INET;
    meuserver.sin_port        = (unsigned short) htons(redeUsuario.porta);
    meuserver.sin_addr.s_addr = *((unsigned long *)hostnm->h_addr);

    if ((socketE = socket(PF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Socket()");
        exit(3);
    }

    if (connect(socketE, (struct sockaddr *)&meuserver, sizeof(meuserver)) < 0)
    {
        perror("Connect()");
        exit(4);
    }

    return socketE;
}

int contaParticipantes(Contato *participantes){
    int cont = 0;

    while(participantes != NULL){
        participantes = participantes->prox;
        cont++;
    }

    return cont;
}

void acessaConversa(char nomeConversa[], Contato *participantes, int tipoConversa){

    char minhaMensagem[100];
    char chamada[100];
    char *nomeArquivo;
    char meuTexto[100];
    Mensagem aenviar;
    FILE *arquivo;
    InfoArq infoArquivo;
    int op;
    InfoRede infoRecebida;
    int tam, flag = 0;

    if(tipoConversa == 2){
        nParticipantes = contaParticipantes(participantes);
    }

    socketEmissor = (int*)malloc(nParticipantes*sizeof(int));

    for(int i = 0; i < nParticipantes; i++){
        fprintf(stderr,"NUMERO: %s\n", participantes->numero);
        infoRecebida = buscaEndereco(participantes->numero);

        if(infoRecebida.porta != 0){
            socketEmissor[i] = conectarUsuario(infoRecebida);
        }
        else{
            socketEmissor[i] = 0;
        }

        if(tipoConversa == 2){
            participantes = participantes->prox;
        }
    }

    system("clear");
    printf("--- Conversa com %s ---\n", nomeConversa);
    printf("/sair (Para sair da conversa)\n\n\n");

    if(tipoConversa == 1){
        strcpy(chatAtual, participantes->numero);
    }

    while(1){

        fprintf(stderr,"> ");
        fflush(stdout);

        if(tipoConversa == 1 && flag == 0){
            exibeNaoVizualizadas(participantes->listaMsgs);
            limpaNaoVizualizadas(&(participantes->listaMsgs));
            flag = 1;
        }

        __fpurge(stdin);
        bzero(minhaMensagem, sizeof(minhaMensagem));
        fgets(minhaMensagem, sizeof(minhaMensagem), stdin);
        minhaMensagem[strlen(minhaMensagem)-1] = '\0';      

        fprintf(stderr,"\033[F\033[K");
        fflush(stdout);

        bzero(chamada, sizeof(chamada));
        strcpy(chamada, minhaMensagem);
        strtok(chamada, " ");
        
        if(strcmp(chamada, "/enviar") == 0){

        }
        else if(strcmp(chamada, "/abrir") == 0){

        }
        else if(strcmp(chamada, "/sair") == 0){
            strcpy(chatAtual, "");
            break;
        }
        else{
            strcpy(meuTexto,"[Voce] ");
            strcat(meuTexto, minhaMensagem);
            print(meuTexto);
            op = 1;

            for(int i = 0; i < nParticipantes; i++){
                if((tam = send(socketEmissor[i], &op, sizeof(op), 0)) <= 0){

                };
            }

            bzero(&aenviar,sizeof(aenviar));
            strcpy(aenviar.chat, chatAtual);
            strcpy(aenviar.msg,minhaMensagem);
            strcpy(aenviar.numRemetente,info.meuNumero);
        }

        for(int i = 0; i < nParticipantes; i++){
            if((tam = send(socketEmissor[i], &aenviar, sizeof(aenviar), 0)) <= 0){
                
            }
        }
    }

    fechaConexoes();
    nParticipantes = 1;
}

void acessaGrupo(Grupo *grupo, Contato *participantes){

    int op;

    do{
        system("clear");
        printf("-- %s --\n\n", grupo->nome);
        printf("[1] Adicionar contato ao grupo\n");
        printf("[2] Acessar conversa\n");
        printf("[3] Participantes\n");
        printf("[4] Voltar\n");
        printf("\n>> ");
        scanf("%d", &op);

        switch(op){
        
            case 1:
                system("clear");
                if(adicionaAoGrupo(&(grupo->participantes) , participantes)){
                    __fpurge(stdin);
                    printf("\nContato adicionado com sucesso! Pressione Enter para continuar...");
                    getchar();
                }
                break;

            case 2:
                acessaConversa(grupo->nome,grupo->participantes, 2);
                break;

            case 3:
                system("clear");
                printf("-- PARTICIPANTES --\n");
                exibeContatos(grupo->participantes, 2);
                __fpurge(stdin);
                printf("\nPressione Enter para continuar...");
                getchar();
                break;
        }

    }while(op != 4);
}

int adicionaAoGrupo(Contato **participantes, Contato *listaDeContatos){

    long op;
    Contato *aux, *novoParticipante = NULL;

    if((aux = listaContatos(listaDeContatos))){
        while(*participantes != NULL && aux != NULL && strcmp((*participantes)->nome, aux->nome) < 0){
            participantes = &(*participantes)->prox;
        }
    }

    if(aux != NULL){
        novoParticipante = (Contato*)malloc(sizeof(Contato));
        strcpy(novoParticipante->nome, aux->nome);
        strcpy(novoParticipante->numero, aux->numero);
        novoParticipante->prox = *participantes;

        *participantes = novoParticipante;
        return 1;
    }

    return 0;
}

Contato *buscaContato(Contato *contato, int indice){

    indice--;

    while(contato != NULL && indice > 0){
        contato = contato->prox;
        indice--;
    }

    return contato;
}

Contato *buscaContatoPorNumero(Contato *contato, char numero[]){

    while(contato != NULL){

        if(strcmp(contato->numero, numero) == 0){
            return contato;
        }
        contato = contato->prox;
    }

    return NULL;
}

Grupo *buscaGrupo(Grupo *grupo, int indice){

    indice--;
    while(grupo != NULL && indice > 0){
        grupo = grupo->prox;
        indice--;
    }

    return grupo;
}

int exibeContatos(Contato *listaDContatos, int tipoExibicao){

    int i = 1, nmensagens = 0;

    while(listaDContatos != NULL){
        printf("[%d] Numero: %s      Nome: %s",i ,listaDContatos->numero, listaDContatos->nome);
        if(tipoExibicao == 1 && (nmensagens = contaNaoVizualizadas(listaDContatos->listaMsgs)) != 0){
            printf(" (%d Novas mensagens)", nmensagens);
        }
        printf("\n");

        listaDContatos = listaDContatos->prox;

        nmensagens = 0;
        i++;
    }

    return i - 1;
}

int exibeGrupos(Grupo *grupos){

    int i = 1;

    while(grupos != NULL){
        printf("[%d] Grupo: %s\n",i ,grupos->nome);
        grupos = grupos->prox;
        i++;
    }

    return i - 1;
}

int contaNaoVizualizadas(MsgNVizualiada *listaMsgs){

    int cont = 0;
    while(listaMsgs != NULL){
        fprintf(stderr,"CONT %d\n",cont);
        listaMsgs = listaMsgs->prox;
        cont++;
    }

    return cont;
}

void exibeNaoVizualizadas(MsgNVizualiada *listaMsgs){

    while(listaMsgs != NULL){
        print(listaMsgs->msg);
        listaMsgs = listaMsgs->prox;
    }
}

void adicionaNaoVizualizada(char msg[], MsgNVizualiada **listaMsgs){

    while(*listaMsgs != NULL){
        listaMsgs = &(*listaMsgs)->prox;
    }

    MsgNVizualiada *aux;
    aux = (MsgNVizualiada*)malloc(sizeof(MsgNVizualiada));
    strcpy(aux->msg, msg);
    aux->prox = NULL;

    *listaMsgs = aux;
}

void limpaNaoVizualizadas(MsgNVizualiada **listaMsgs){

    if(*listaMsgs != NULL){
        limpaNaoVizualizadas(&(*listaMsgs)->prox);
        free(*listaMsgs);
        *listaMsgs = NULL;
    }
}

int criarSocketServidor(char *ip, char *porta){

    unsigned short port;
    struct hostent *hostnm;
    int sock;

    hostnm = gethostbyname(ip);

    if (hostnm == (struct hostent *)0){
        return 0;
    }

    port = (unsigned short)atoi(porta);

    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = *((unsigned long *)hostnm->h_addr);

    if((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0){
        perror("socket()");
        exit(0);
    };

    return sock;
}

void *esperaUsuarios(){

    int td;
    pthread_t thread;
    struct sockaddr_in client;
    int porta;
    int namelen;
    int socketReceptor = 0;
    Parametro parametros;
    int sock;
    struct sockaddr_in meuserver; 

    if ((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0){
        perror("socket()");
        return 0;
    }

    meuserver.sin_family = AF_INET;   
    meuserver.sin_port   = 0;      
    meuserver.sin_addr.s_addr = INADDR_ANY;

    if (bind(sock, (struct sockaddr *) &meuserver, sizeof(meuserver)) < 0){
        perror("bind()");
        return 0;
    }

    namelen = sizeof(meuserver);
    if (getsockname(sock, (struct sockaddr *) &meuserver, &namelen) < 0){
        perror("getsocketname()");
        return 0;
    }

    if (listen(sock, 1) != 0){
        perror("listen()");
        return 0;
    }

    info.porta = ntohs(meuserver.sin_port);
    pthread_mutex_unlock(&locker);

    while(1){

        if ((socketReceptor = accept(sock, (struct sockaddr *)&client, (socklen_t *)&namelen)) == -1){
            perror("accept()");
            exit(5);
        }

        parametros.socketReceptor = socketReceptor;
        parametros.cliente = client;

        td = pthread_create(&thread, NULL, &threadReceptora, (void*)&parametros);
        if (td) {
            printf("ERRO: impossivel criar thread.\n");
            exit(0);
        }

        pthread_detach(thread);
    }
}

void fechaConexoes(){
    
    for(int i = 0; i < nParticipantes; i++){
        if(socketEmissor != NULL){
            close(socketEmissor[i]);
            free(socketEmissor);

            socketEmissor + sizeof(int);
        }
    }
}

void encerraCliente(){
    system("clear");
    fechaConexoes();
    exit(0);
}