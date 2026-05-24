#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <regex.h>
#include "lexer.h"

#define QTD_PADROES 12

typedef struct{
    const char *padrao;
    TokenType tipo;
    const char *nome;
} TokenDef;

TokenDef definicoes[] ={
   {"^/\\*([^*]|\\*+[^/*])*\\*+/", TOKEN_COMMENT, "COMENTARIO"},
   {"^\"([^\"\\\\\n\r]|\\\\[^\n\r])*\"", TOKEN_STRING, "STRING"},
   {"^\"([^\"\\\\\n\r]|\\\\[^\n\r])*", TOKEN_ERROR, "ERRO_STR"},
   {"^(if|else|return|while|wesley|for|do|int|float|char|void)", TOKEN_KEYWORD, "P_CHAVE"},
   {"^[0-9]+\\.[0-9.]*\\.[0-9.]*", TOKEN_ERROR, "ERRO_NUM"},
   {"^[0-9]+[a-zA-Z_][a-zA-Z0-9_]*", TOKEN_ERROR, "ERRO_ID"},
   {"^[@#$\\?~`]+[a-zA-Z0-9_]*", TOKEN_ERROR, "ERRO_ID"},
   {"^[0-9]+\\.[0-9]+", TOKEN_FLOAT, "FLOAT"},
   {"^[0-9]+", TOKEN_INT, "INTEIRO"},
   {"^(\\(|\\)|\\{|\\}|\\[|\\]|;|,|\\.)", TOKEN_DELIM, "DELIMITADOR"},
   {"^(==|!=|>=|<=|\\+=|-=|\\*=|/=|\\+\\+|--|&&|[+*/%<>=!&|^~?:-])", TOKEN_OP, "OPERADOR"},
   {"^[a-zA-Z_][a-zA-Z0-9_]*", TOKEN_ID, "ID"}
};

int identificarVazio(char c){
    return(c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v');
}

void adicionarToken(TokenList *lista, TokenType tipo, const char *nome_tipo, const char *lexema, int tam, int linha, int coluna){
    if(lista->count >= lista->capacity){
        lista->capacity *= 2;
        lista->tokens = realloc(lista->tokens, lista->capacity * sizeof(Token));
    }
    Token *t = &lista->tokens[lista->count++];
    t->tipo = tipo;
    snprintf(t->nome_tipo, sizeof(t->nome_tipo), "%s", nome_tipo);
    snprintf(t->lexema, sizeof(t->lexema), "%.*s", tam, lexema);
    t->linha = linha;
    t->coluna = coluna;
}

TokenList analisarLexico(const char *input){
    TokenList lista;
    lista.capacity = 256;
    lista.count = 0;
    lista.tokens = malloc(lista.capacity * sizeof(Token));

    if(!input) return lista;

    regex_t regexes[QTD_PADROES];
    regmatch_t matches[1];

    for(int i = 0; i < QTD_PADROES; i++){
        if(regcomp(&regexes[i], definicoes[i].padrao, REG_EXTENDED) != 0){
            printf("Erro no regex: %s\n", definicoes[i].padrao);
            return lista;
        }
    }

    const char *batedor = input;
    int linha = 1, coluna = 1;

    while(*batedor != '\0'){
        if(identificarVazio(*batedor)){
            if(*batedor == '\n'){ linha++; coluna = 1; } 
            else{ coluna++; }
            batedor++;
            continue;
        }

        const char *ponteiroInicio = batedor;
        int t_lin = linha, t_col = coluna, achou = 0;

        for(int i = 0; i < QTD_PADROES; i++){
            if(regexec(&regexes[i], batedor, 1, matches, 0) == 0 && matches[0].rm_so == 0){
                int tam = matches[0].rm_eo;
                for(int j = 0; j < tam; j++){
                    if(batedor[j] == '\n'){ linha++; coluna = 1; } 
                    else{ coluna++; }
                }
                batedor += tam;
                adicionarToken(&lista, definicoes[i].tipo, definicoes[i].nome, ponteiroInicio, tam, t_lin, t_col);
                achou = 1;
                break;
            }
        }

        if(!achou){
            while(*batedor != '\0'){
                int proximo_eh_valido = 0;
                for(int i = 0; i < QTD_PADROES; i++){
                    if(regexec(&regexes[i], batedor, 1, matches, 0) == 0 && matches[0].rm_so == 0){
                        proximo_eh_valido = 1; break;
                    }
                }
                if(proximo_eh_valido) break;
                batedor++; coluna++;
            }
            adicionarToken(&lista, TOKEN_ERROR, "ERRO LÉXICO", ponteiroInicio, batedor - ponteiroInicio, t_lin, t_col);
        }
    }

    // Adiciona o token de fim de arquivo
    adicionarToken(&lista, TOKEN_EOF, "EOF", "EOF", 3, linha, coluna);

    for(int i = 0; i < QTD_PADROES; i++) regfree(&regexes[i]);
    
    return lista;
}

void imprimirTokens(TokenList *lista){
    printf("%-25s | %-12s | %-6s | %-6s\n", "LEXEMA", "TIPO", "LINHA", "COLUNA");
    printf("--------------------------|--------------|--------|--------\n");
    for(int i = 0; i < lista->count - 1; i++){ // -1 para não imprimir o EOF
        Token *t = &lista->tokens[i];
        printf("%-25s | %-12s | %-6d | %-6d\n", t->lexema, t->nome_tipo, t->linha, t->coluna);
    }
}

void liberarTokens(TokenList *lista){
    free(lista->tokens);
    lista->tokens = NULL;
    lista->count = 0;
    lista->capacity = 0;
}