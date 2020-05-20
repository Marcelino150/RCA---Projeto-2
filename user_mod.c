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

typedef struct contato Contato;
typedef struct grupo Grupo;

struct contato{
    char nome[32];
    int numero;

    Contato *prox;
};

struct grupo{
    char nome[32];
    Contato *participantes;

    Grupo *prox;
};

void adicionaContato(Contato **contatos);
void criaGrupo(Grupo **grupos);
Contato *listaContatos(Contato *contatos);
Grupo *listaGrupos(Grupo *grupos);
void acessaConversa(char nomeConversa[], Contato contatos);
void acessaGrupo(Grupo *grupo, Contato *contatos);
void acessaContato(Contato *contato);
int adicionarAoGrupo(Contato **participantes, Contato *contatos);
Contato *buscaContato(Contato *contato, int indice);
Grupo *buscaGrupo(Grupo *grupo, int indice);
int exibeContatos(Contato *contatos);
int exibeGrupos(Grupo *grupos);

void main(){

    int op = 0;
    Contato *contatos = NULL;
    Grupo *grupos = NULL;

    Contato *retContato;
    Grupo *retGrupo;

    do{
        system("clear");
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
                adicionaContato(&contatos);
                __fpurge(stdin);
                printf("\nContato adicionado com sucesso! Pressione Enter para continuar...");
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
                retContato = listaContatos(contatos);
                break;
            
            case 4:
                system("clear");
                if((retGrupo = listaGrupos(grupos))){
                    acessaGrupo(retGrupo,contatos);
                }
                break;

            case 5:
                system("clear");
        }

    }while(op != 5);
}

void adicionaContato(Contato **contatos){

    char nome[32];
    long numero;

    printf("--ADICIONAR CONTATO--\n\n");

    __fpurge(stdin);
    printf("Nome: ");
    fgets(nome, sizeof(nome), stdin);
    nome[strlen(nome)-1] = '\0';

    printf("Numero: ");
    scanf("%ld", &numero);

    while(*contatos != NULL && strcmp((*contatos)->nome, nome) < 0){
        contatos = &(*contatos)->prox;
    }

    Contato *aux;
    aux = (Contato*)malloc(sizeof(Contato));
    strcpy(aux->nome, nome);
    aux->numero = numero;
    aux->prox = *contatos;

    *contatos = aux;
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

Contato *listaContatos(Contato *contatos){

    int quant, op;

    do{
        system("clear");
        printf("-- CONTATOS --\n\n");
        quant = exibeContatos(contatos);

        printf("\n[0] Voltar");
        printf("\n>> ");
        scanf("%d",&op);

        if(op > 0 && op <= quant){
            return buscaContato(contatos, op);
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

        printf("\n[0] Voltar");
        printf("\n>> ");
        scanf("%d",&op);

        if(op > 0 && op <= quant){
            return buscaGrupo(grupos, op);
        }

    }while(op != 0);
}

void acessaConversa(char nomeConversa[], Contato contatos){

}

void acessaGrupo(Grupo *grupo, Contato *contatos){

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
                if(adicionarAoGrupo(&(grupo->participantes) , contatos)){
                    __fpurge(stdin);
                    printf("\nContato adicionado com sucesso! Pressione Enter para continuar...");
                    getchar();
                }
                break;

            case 2:
                break;

            case 3:
                system("clear");
                printf("-- PARTICIPANTES --\n");
                exibeContatos(grupo->participantes);
                __fpurge(stdin);
                printf("\nPressione Enter para continuar...");
                getchar();
                break;
        }

    }while(op != 4);
}

int adicionarAoGrupo(Contato **participantes, Contato *contatos){

    long op;
    Contato *aux, *novoParticipante = NULL;

    if((aux = listaContatos(contatos))){
        while(*participantes != NULL && aux != NULL && strcmp((*participantes)->nome, aux->nome) < 0){
            participantes = &(*participantes)->prox;
        }
    }

    if(aux != NULL){
        novoParticipante = (Contato*)malloc(sizeof(Contato));
        strcpy(novoParticipante->nome, aux->nome);
        novoParticipante->numero = aux->numero;

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

Grupo *buscaGrupo(Grupo *grupo, int indice){

    indice--;
    while(grupo != NULL && indice > 0){
        grupo = grupo->prox;
        indice--;
    }

    return grupo;
}

int exibeContatos(Contato *contatos){

    int i = 1;

    while(contatos != NULL){
        printf("[%d] Numero: %d      Nome: %s\n",i ,contatos->numero, contatos->nome);
        contatos = contatos->prox;
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


void acessaContatos(Grupo *contato){

}