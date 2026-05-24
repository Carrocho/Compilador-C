#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include "AST.h"

// Retorna 1 se sucesso (sem erros), 0 se houve erros sintáticos
int analisarSintaxe(TokenList *lista, ASTNode **raiz_ast);

#endif