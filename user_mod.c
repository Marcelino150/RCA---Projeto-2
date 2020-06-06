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
#define PASTA "Armazenamento_de"
#define h_addr h_addr_list[0]

typedef struct contato Contato;
typedef struct grupo Grupo;
typedef struct infoArq InfoArq;
typedef struct mensagem Mensagem;
typedef struct msgnvizualiada MsgNVizualiada;
typedef struct infoRede InfoRede;
typedef struct parametro Parametro;
typedef struct minhaInfo MinhaInfo;
typedef struct contatoAgenda ContatoAgenda;

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

struct contatoAgenda{
    char nome[32];
    char numero[20];
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
    char numRemetente[20];
};

struct mensagem{
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

int criarSocketServidor(char *ip, char *porta); // Cria socket para se comunicar com o servidor
void *esperaUsuarios(); // Aguarda conexõesde outros usuários
void *threadReceptora(void *parametros); // Recebe mensagens/arquivos de usuários
void fechaConexoes(); // Fecha todos os sockets
void ficarOffline(); // Muda o status no servidor para "offline"
void encerraCliente(); // Fecha todas as conexões e fica offline no servidor

int adicionaContato(Contato **listaDeContatos); // Menu de adição de contato
int procuraDuplicata(Contato *listaDeContatos, char numero[]); // Verifica se o número a ser cadastrado já existe na lista de contatos
int adicionaNaLista(Contato **listaDeContatos, char nome[], char numero[]); // Adição de contato na lista-ligada
int exibeContatos(Contato *listaDeContatos, int tipoExibicao); // Exibe todos os contatos contidos em "listaDeContatos"
void acessaConversa(char nomeConversa[], Contato *participantes, int tipoConversa); // Entra no chat de conversa
Contato *listaContatos(Contato *listaDeContatos); // Exibe todos os contatos contidos em "listaDeContatos" e aguarda o usuário escolher um
Contato *buscaContato(Contato *contato, int indice); // Busca um contato pelo indice na lista-ligada
Contato *buscaContatoPorNumero(Contato *listaDeContatos, char numero[]); // Busca contato pelo numero do contato

void adicionaNaoVizualizada(char msg[], MsgNVizualiada **listaMsgs); // Quando o usuário não estiver no chat referente a mensagem recebida, armazena a mensagem em uma lista-ligada
void exibeNaoVizualizadas(MsgNVizualiada *listaMsgs); // Exibe as mensagens não lidas armazenadas
void limpaNaoVizualizadas(MsgNVizualiada **listaMsgs); // Libera a memória alocada para mensagens não visualizadas
int contaNaoVizualizadas(MsgNVizualiada *listaMsgs); // Conta a quantiade de mensagens não visualizadas

void criaGrupo(Grupo **grupos); // Cria um novo grupo
void acessaGrupo(Grupo *grupo, Contato *listaDeContatos); // Acessa um grupo criado
int exibeGrupos(Grupo *grupos); // Exibe todos os grupos criados
int adicionaAoGrupo(Contato **participantes, Contato *listaDeContatos); // Exibe todos os contatos contidos em "listaDeContatos", aguarda que o usuário escolha um, e adiciona o escolhido ao grupo
Grupo *listaGrupos(Grupo *grupos); // Exibe todos os grupos criados e aguarda o usuário escolher um
Grupo *buscaGrupo(Grupo *grupo, int indice); // Busca um grupo pelo indice na lista-ligada

void gravaArquivo(char nome[], char numero[]); // Adiciona os dados de um contato no fim do arquivo "Agenda"
void leArquivo(); // Lê o arquivo "Agenda" adicionando os contatos lidos a lista ligada do programa

struct sockaddr_in server;
pthread_mutex_t locker;
MinhaInfo info;
Contato *contatos = NULL;

char nomeServidor[20];
char portaServidor[20];
char chatAtual[20] = "";
int namelen;
int *socketEmissor;
int nParticipantes = 1;

int main(int argc, char **argv){

    int op = 0;
    Grupo *grupos = NULL;
    Contato *retContato;
    Grupo *retGrupo;
    int td;
    pthread_t thread;
    int opServidor;
    int s;

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

    leArquivo();
    socketEmissor = NULL;

    strcpy(nomeServidor, argv[1]);
    strcpy(portaServidor, argv[2]);

    s = criarSocketServidor(nomeServidor, portaServidor);
    if(connect(s, (struct sockaddr *)&server, sizeof(server)) < 0){
        perror("connect()");
        exit(0);
    };

    td = pthread_create(&thread, NULL, &esperaUsuarios, NULL);
    pthread_mutex_lock(&locker);

    opServidor = 0;
    if (send(s, &opServidor, sizeof(opServidor), 0) <= 0){
        perror("send()");
        exit(0);
    }

    if (send(s, &info, sizeof(info), 0) <= 0){
        perror("send()");
        exit(0);
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
        __fpurge(stdin);
        scanf("%d", &op);

        switch(op){

            case 1:
                system("clear");
                if(adicionaContato(&contatos)){
                    printf("\nContato adicionado com sucesso! Pressione Enter para continuar...");
                }
                else{
                    printf("\nJa existe um contato com este numero! Pressione Enter para continuar...");
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

    encerraCliente();
    exit(1);
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
        perror("recv()");
    }
    close(s);

    return rede;
}

int adicionaNaLista(Contato **listaDeContatos, char nome[], char numero[]){

    if(procuraDuplicata(*listaDeContatos, numero)){
        while(*listaDeContatos != NULL && strcmp((*listaDeContatos)->nome, nome) < 0){
            listaDeContatos = &(*listaDeContatos)->prox;
        }
    }
    else{
        return 0;
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

    if(adicionaNaLista(listaDeContatos, nome, numero)){
        gravaArquivo(nome, numero);
        return 1;
    }

    return 0;
}

int procuraDuplicata(Contato *listaDeContatos, char numero[]){
    
    while(listaDeContatos != NULL){
        if(strcmp(listaDeContatos->numero, numero) == 0){
            return 0;
        }

        listaDeContatos = listaDeContatos->prox;
    }
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

    char caminho[100], paraExibir[256];
    int bRecebidos = 0, op;
    FILE *arquivo;
    InfoArq infoArquivo;
    Mensagem recebido;
    void *bloco;
    Contato *remetente;
    struct sockaddr_in client = ((Parametro*)parametros)-> cliente;
    int socketReceptor = ((Parametro*)parametros)-> socketReceptor; 
    int tam;   

    do{
        bzero(&op, sizeof(op));
        tam = recv(socketReceptor, &op, sizeof(op), 0);
        
        if(op == 1){
            bzero(&recebido, sizeof(recebido));
            tam = recv(socketReceptor, &recebido, sizeof(recebido), 0);

            fflush(stdout);
            remetente = buscaContatoPorNumero(contatos, recebido.numRemetente);
            sprintf(paraExibir,"[%s] %s",remetente->nome,recebido.msg);
            if(strcmp(chatAtual, recebido.numRemetente) == 0 && remetente != NULL){
                print(paraExibir);
            }
            else if(remetente != NULL){
                adicionaNaoVizualizada(paraExibir, &(remetente->listaMsgs));
            }
        }
        else if(op == 2){
            
            sprintf(caminho,"%s_%s/", PASTA, info.meuNumero);
            recv(socketReceptor, &infoArquivo, sizeof(infoArquivo), 0);

            mkdir(caminho, S_IRWXU | S_IRWXG | S_IRWXO);
            strcat(caminho, infoArquivo.nomeArq);
            arquivo = fopen(caminho,"wb");
            bloco = malloc(TAM_BLC);

            do{
                bRecebidos = recv(socketReceptor, bloco, TAM_BLC, 0);
                infoArquivo.tamanho -= bRecebidos;

            }while(fwrite(bloco, 1, bRecebidos, arquivo) >= 0 && infoArquivo.tamanho > 0);

            fclose(arquivo);
            free(bloco);

            fflush(stdout);
            remetente = buscaContatoPorNumero(contatos, infoArquivo.numRemetente);
            sprintf(paraExibir,"Voce recebeu o arquivo %s de %s.", infoArquivo.nomeArq, remetente->nome);
            if(strcmp(chatAtual, infoArquivo.numRemetente) == 0 && remetente != NULL){
                print(paraExibir);
            }
            else if(remetente != NULL){
                adicionaNaoVizualizada(paraExibir, &(remetente->listaMsgs));
            }
        }

    }while(op != 0);

    close(socketReceptor);
    pthread_exit(NULL);
}

int conectarUsuario(InfoRede redeUsuario){

    unsigned short port;             
    struct hostent *hostnm;    
    struct sockaddr_in meuserver;
    int socketE = 0;

    hostnm = gethostbyname(redeUsuario.ip);
    if (hostnm == (struct hostent *) 0){
        perror("gethostbyname");
        exit(0);
    }

    meuserver.sin_family      = AF_INET;
    meuserver.sin_port        = (unsigned short) htons(redeUsuario.porta);
    meuserver.sin_addr.s_addr = *((unsigned long *)hostnm->h_addr);

    if ((socketE = socket(PF_INET, SOCK_STREAM, 0)) < 0){
        perror("Socket()");
        exit(0);
    }

    if (connect(socketE, (struct sockaddr *)&meuserver, sizeof(meuserver)) < 0){
        perror("Connect()");
        exit(0);
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
    char caminho[100];
    char meuTexto[100];
    Mensagem aenviar;
    FILE *arquivo;
    InfoArq infoArquivo;
    int op;
    InfoRede infoRecebida;
    int tam, flag = 0;
    int envioValido = 0;
    int usuarioOnline = 0;
    char comando[128];
    void *bloco;
    int bLidos;

    if(tipoConversa == 2){
        nParticipantes = contaParticipantes(participantes);
    }

    socketEmissor = (int*)malloc(nParticipantes*sizeof(int));

    for(int i = 0; i < nParticipantes; i++){
        infoRecebida = buscaEndereco(participantes->numero);

        if(infoRecebida.porta != 0){
            socketEmissor[i] = conectarUsuario(infoRecebida);
            usuarioOnline = 1;
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
    printf("Comandos: /sair - /enviar <nome do arquivo> - /abrir <nome do arquivo>\n\n\n");

    if(tipoConversa == 1){
        strcpy(chatAtual, participantes->numero);
    }
    else{
        strcpy(chatAtual, "");
    }

    while(usuarioOnline == 1){

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
        
        sprintf(caminho,"%s_%s/", PASTA, info.meuNumero);

        if(strcmp(chamada, "/enviar") == 0){

            nomeArquivo = strtok(NULL, "");

            if(!nomeArquivo){
                strcpy(meuTexto, "Nome do arquivo nao especificado.");
                print(meuTexto);
            }
            else{
                strcat(caminho, nomeArquivo);
                if((arquivo = fopen(caminho,"rb")) <= 0){
                    sprintf(meuTexto,"O arquivo [%s] nao existe.", nomeArquivo);
                    print(meuTexto);
                }
                else{
                    fseek (arquivo, 0 , SEEK_END);
                    tam = ftell(arquivo);
                    rewind (arquivo);

                    op = 2;

                    for(int i = 0; i < nParticipantes; i++){
                        if(socketEmissor[i] != 0){
                            send(socketEmissor[i], &op, sizeof(op), 0);
                        };
                    }

                    strcpy(infoArquivo.nomeArq, nomeArquivo);
                    strcpy(infoArquivo.numRemetente, info.meuNumero);
                    infoArquivo.tamanho = tam;

                    for(int i = 0; i < nParticipantes; i++){
                        if (socketEmissor[i] != 0){
                            send(socketEmissor[i], &infoArquivo, sizeof(infoArquivo), 0);
                        }
                    }

                    bloco = malloc(TAM_BLC);

                    while(infoArquivo.tamanho > 0 && (bLidos = fread(bloco, 1, TAM_BLC, arquivo)) >= 0){
                        for(int i = 0; i < nParticipantes; i++){
                            if (socketEmissor[i] != 0){
                                (send(socketEmissor[i], bloco, bLidos, 0) <= 0);
                            }
                        }
                        infoArquivo.tamanho -= bLidos;  
                    }
                    
                    fclose(arquivo);
                    free(bloco);

                    envioValido = 1;

                    sprintf(meuTexto,"Arquivo [%s] enviado.", nomeArquivo);
                    print(meuTexto);
                }
            }
        }
        else if(strcmp(chamada, "/abrir") == 0){

            nomeArquivo = strtok(NULL, "");

            if(!nomeArquivo){
                strcpy(meuTexto, "Nome do arquivo nao especificado.");
            }
            else{
                strcat(caminho, nomeArquivo);
                if((arquivo = fopen(caminho,"rb")) <= 0){
                    sprintf(meuTexto,"O arquivo [%s] nao existe.", nomeArquivo);
                    print(meuTexto);
                }
                else{
                    sprintf(comando, "xdg-open %s 2>/dev/null", caminho);
                    system(comando);
                    sprintf(meuTexto,"Abrindo arquivo [%s]...", nomeArquivo);
                    print(meuTexto);
                    fclose(arquivo);
                }
            }
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
                if(socketEmissor[i] != 0){
                    bLidos = send(socketEmissor[i], &op, sizeof(op), 0);
                };
            }

            bzero(&aenviar,sizeof(aenviar));
            strcpy(aenviar.msg, minhaMensagem);
            strcpy(aenviar.numRemetente, info.meuNumero);

            envioValido = 1;
        }

        if(envioValido){
            for(int i = 0; i < nParticipantes; i++){
                if(socketEmissor[i] != 0){
                    bLidos = send(socketEmissor[i], &aenviar, sizeof(aenviar), 0);
                }
            }
            envioValido = 0;
        }
    }

    if(usuarioOnline == 0){
        printf("O(s) usuario(s) dessa conversa esta(o) offline.\n");
        printf("\nPressione Enter para continuar...");
        __fpurge(stdin);
        getchar();
    }
    else{
        fechaConexoes();
    }

    nParticipantes = 1;
}

void acessaGrupo(Grupo *grupo, Contato *listaDeContatos){

    int op;

    system("clear");

    do{
        printf("-- %s --\n\n", grupo->nome);
        printf("[1] Adicionar contato ao grupo\n");
        printf("[2] Acessar conversa\n");
        printf("[3] Participantes\n");
        printf("[4] Voltar\n");
        printf("\n>> ");
        __fpurge(stdin);
        scanf("%d", &op);

        switch(op){
        
            case 1:
                system("clear");
                if(adicionaAoGrupo(&(grupo->participantes) , listaDeContatos)){
                    printf("\nParticipante adicionado com sucesso! Pressione Enter para continuar...");       
                }
                else{
                    printf("\nEste contato já está no grupo! Pressione Enter para continuar...");
                }
                __fpurge(stdin);
                getchar();
                break;

            case 2:
                system("clear");
                acessaConversa(grupo->nome,grupo->participantes, 2);
                break;

            case 3:
                system("clear");
                printf("-- PARTICIPANTES --\n\n");
                exibeContatos(grupo->participantes, 2);
                __fpurge(stdin);
                printf("\nPressione Enter para continuar...");
                getchar();
                break;
        }

        system("clear");
    }while(op != 4);
}

int adicionaAoGrupo(Contato **participantes, Contato *listaDeContatos){

    long op;
    Contato *contatoEscolhido = NULL, *novoParticipante = NULL;

    if((contatoEscolhido = listaContatos(listaDeContatos))){
        if(procuraDuplicata(*participantes, contatoEscolhido->numero)){
            while(*participantes != NULL && contatoEscolhido != NULL && strcmp((*participantes)->nome, contatoEscolhido->nome) < 0){
                participantes = &(*participantes)->prox;
            }
        }
        else{
            return 0;
        }
    }

    if(contatoEscolhido != NULL){
        novoParticipante = (Contato*)malloc(sizeof(Contato));
        strcpy(novoParticipante->nome, contatoEscolhido->nome);
        strcpy(novoParticipante->numero, contatoEscolhido->numero);
        novoParticipante->prox = *participantes;

        *participantes = novoParticipante;
        return 1;
    }

    return -1;
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

void gravaArquivo(char nome[], char numero[]){

    char caminho[100];
    FILE *arquivo;
    ContatoAgenda contato;

    strcpy(contato.nome, nome);
    strcpy(contato.numero, numero);

    sprintf(caminho,"%s_%s/", PASTA, info.meuNumero);
    mkdir(caminho, S_IRWXU | S_IRWXG | S_IRWXO);
    strcat(caminho, "Agenda");
    arquivo = fopen(caminho,"ab");

    if(arquivo){
        fwrite(&contato, 1, sizeof(ContatoAgenda), arquivo);
        fclose(arquivo);
    }
}

void leArquivo(){

    char caminho[100];
    FILE *arquivo;
    ContatoAgenda contato;
    int bLidos = 0;
    int tamArquivo = 0;;

    sprintf(caminho,"%s_%s/", PASTA, info.meuNumero);
    strcat(caminho, "Agenda");
    arquivo = fopen(caminho,"rb");

    if(arquivo){

        fseek (arquivo , 0 , SEEK_END);
        tamArquivo = ftell(arquivo);
        rewind (arquivo);

        while(tamArquivo > 0 && (bLidos = fread(&contato, 1, sizeof(ContatoAgenda), arquivo)) >= 0){
            adicionaNaLista(&contatos, contato.nome, contato.numero);
            tamArquivo -= bLidos;
        }

        fclose(arquivo);
    }
}

void fechaConexoes(){

    if(socketEmissor != NULL){
        for(int i = 0; i < nParticipantes; i++){
            if(socketEmissor[i] != 0){
                close(socketEmissor[i]);
            }
        }

        free(socketEmissor);
        socketEmissor = NULL;
    }
}

void encerraCliente(){
    system("clear");
    fechaConexoes();
    ficarOffline();
    exit(0);
}

void ficarOffline(){

    int s, opServidor = 2;

    s = criarSocketServidor(nomeServidor, portaServidor);
    if(connect(s, (struct sockaddr *)&server, sizeof(server)) < 0){
        perror("connect()");
    };

    if (send(s, &opServidor, sizeof(opServidor), 0) <= 0){
        perror("send()");
    }

    if (send(s, info.meuNumero, sizeof(info.meuNumero), 0) <= 0){
        perror("send()");
    }

    close(s);
}