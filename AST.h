#ifndef AST_H
#define AST_H

typedef enum {
    AST_PROGRAMA,
    AST_DECLARACAO,
    AST_ATRIBUICAO,
    AST_IF,
    AST_WHILE,
    AST_BLOCO,
    AST_EXPRESSAO_BINARIA,
    AST_IDENTIFICADOR,
    AST_LITERAL,
    AST_FUNCAO,
    AST_CHAMADA_FUNCAO
} ASTNodeType;

typedef struct ASTNode {
    ASTNodeType tipo;
    char valor[256];
    
    struct ASTNode **filhos;
    int qtd_filhos;
    int capacidade;
} ASTNode;

// Funções de manipulação da Árvore
ASTNode* criarNoAST(ASTNodeType tipo, const char *valor);
void adicionarFilhoAST(ASTNode *pai, ASTNode *filho);
void imprimirAST(ASTNode *raiz, int nivel);
void liberarAST(ASTNode *raiz);

#endif