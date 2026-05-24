#ifndef LEXER_H
#define LEXER_H

// Tipos de Tokens
typedef enum {
    TOKEN_KEYWORD,
    TOKEN_FLOAT,
    TOKEN_INT,
    TOKEN_ID,
    TOKEN_OP,
    TOKEN_STRING,
    TOKEN_ERROR,
    TOKEN_COMMENT,
    TOKEN_DELIM,
    TOKEN_EOF
} TokenType;

// Estrutura de um Token individual
typedef struct {
    TokenType tipo;
    char lexema[256];
    char nome_tipo[20];
    int linha;
    int coluna;
} Token;

// Lista Dinâmica de Tokens
typedef struct {
    Token *tokens;
    int count;
    int capacity;
} TokenList;

// Funções públicas do Lexer
TokenList analisarLexico(const char *input);
void imprimirTokens(TokenList *lista);
void liberarTokens(TokenList *lista);

#endif