#include <stdio.h>
#include <stdlib.h>
#include "lexer.h"
#include "parser.h"

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Uso: %s <nome_do_arquivo.txt>\n", argv[0]);
        return 1;
    }

    FILE *f = fopen(argv[1], "r");
    if (!f) return 1;

    fseek(f, 0, SEEK_END);
    long tam = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *buf = malloc(tam + 1);
    if (buf) {
        size_t n = fread(buf, 1, tam, f);
        buf[n] = '\0';
        
        // 1. Léxico
        TokenList tokens = analisarLexico(buf);
        imprimirTokens(&tokens);
        
        // 2. Sintático
        printf("\nAnalisador sintático\n");
        ASTNode *arvore_sintatica = NULL;
        int sucesso = analisarSintaxe(&tokens, &arvore_sintatica);

        // 3. Resultado
        if (sucesso) {
            printf("SUCESSO!\n");
            printf("\n--- ÁRVORE SINTÁTICA ABSTRATA (AST) ---\n");
            imprimirAST(arvore_sintatica, 0);
        } else {
            printf("ERRO!\n");
            imprimirAST(arvore_sintatica, 0);
        }

        // Limpeza
        liberarAST(arvore_sintatica);
        liberarTokens(&tokens);
        free(buf);
    }

    fclose(f);
    return 0;
}