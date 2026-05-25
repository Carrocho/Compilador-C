#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.h"

static TokenList *tokens;
static int pos = 0;
static int tem_erro = 0;

ASTNode* criarNoAST(ASTNodeType tipo, const char *valor){
    ASTNode *no = malloc(sizeof(ASTNode));
    no->tipo = tipo;
    strcpy(no->valor, valor ? valor : "");
    no->capacidade = 2;
    no->qtd_filhos = 0;
    no->filhos = malloc(no->capacidade * sizeof(ASTNode*));
    return no;
}

void adicionarFilhoAST(ASTNode *pai, ASTNode *filho){
    if(!pai || !filho) return;
    if(pai->qtd_filhos >= pai->capacidade){
        pai->capacidade *= 2;
        pai->filhos = realloc(pai->filhos, pai->capacidade * sizeof(ASTNode*));
    }
    pai->filhos[pai->qtd_filhos++] = filho;
}

Token atual(){ return tokens->tokens[pos]; }
Token anterior(){ return tokens->tokens[pos > 0 ? pos - 1 : 0]; }

void avancar(){
    if(atual().tipo != TOKEN_EOF) pos++;
}

int match(TokenType tipo){
    if(atual().tipo == tipo){
        avancar();
        return 1;
    }
    return 0;
}

int matchLexema(TokenType tipo, const char *lexema){
    if(atual().tipo == tipo && strcmp(atual().lexema, lexema) == 0){
        avancar();
        return 1;
    }
    return 0;
}

void erroSintatico(const char *esperado){
    Token t = atual();
    printf("[ERRO SINTÁTICO] Linha %d, Col: %d -> Esperado: %s | Encontrado: '%s'\n", 
           t.linha, t.coluna, esperado, t.lexema);
    tem_erro = 1;
}

void sincronizar(){
    tem_erro = 1;
    
    while(atual().tipo != TOKEN_EOF){
        if(atual().tipo == TOKEN_KEYWORD){
            return;
        }
        if(atual().tipo == TOKEN_DELIM &&(strcmp(atual().lexema, ";") == 0 || strcmp(atual().lexema, "}") == 0)){
            avancar();
            return;
        }
        avancar();
    }
}

ASTNode* expressao();
ASTNode* statement();
ASTNode* bloco();

// fator -> NUMERO | STRING | ID | ID '(' argumentos ')' | '(' expressao ')'
ASTNode* fator(){
    if(match(TOKEN_INT) || match(TOKEN_FLOAT) || match(TOKEN_STRING)){
        return criarNoAST(AST_LITERAL, anterior().lexema);
    }
    //se for um Identificador(variável ou função)
    if(match(TOKEN_ID)){
        char nome[256];
        strcpy(nome, anterior().lexema);
        
        //ve se é uma chamada de função
        if(matchLexema(TOKEN_DELIM, "(")){
            ASTNode *chamada = criarNoAST(AST_CHAMADA_FUNCAO, nome);
            //lê a lista de argumentos, se a função não for vazia
            if(!(atual().tipo == TOKEN_DELIM && strcmp(atual().lexema, ")") == 0)){
                adicionarFilhoAST(chamada, expressao()); // Lê o primeiro argumento
                
                //se tiver vírgula, lê os próximos
                while(matchLexema(TOKEN_DELIM, ",")){
                    adicionarFilhoAST(chamada, expressao());
                }
            }
            if(!matchLexema(TOKEN_DELIM, ")")) erroSintatico(")");
            return chamada;
        }
        //se não tinha '(', então é só uma variável normal
        return criarNoAST(AST_IDENTIFICADOR, nome);
    }
    
    if(matchLexema(TOKEN_DELIM, "(")){
        ASTNode *expr = expressao();
        if(!matchLexema(TOKEN_DELIM, ")")) erroSintatico(")");
        return expr;
    }
    
    erroSintatico("Número, Identificador ou '('");
    return NULL;
}

ASTNode* termo(){
    ASTNode *no = fator();
    while(matchLexema(TOKEN_OP, "*") || matchLexema(TOKEN_OP, "/")){
        ASTNode *novo_no = criarNoAST(AST_EXPRESSAO_BINARIA, anterior().lexema);
        adicionarFilhoAST(novo_no, no);
        adicionarFilhoAST(novo_no, fator());
        no = novo_no;
    }
    return no;
}

ASTNode* expressao(){
    ASTNode *no = termo();
    while(atual().tipo == TOKEN_OP){
        avancar();
        ASTNode *novo_no = criarNoAST(AST_EXPRESSAO_BINARIA, anterior().lexema);
        adicionarFilhoAST(novo_no, no);
        adicionarFilhoAST(novo_no, termo());
        no = novo_no;
    }
    return no;
}

ASTNode* bloco(){
    ASTNode *no = criarNoAST(AST_BLOCO, "{}");
    if(!matchLexema(TOKEN_DELIM, "{")) erroSintatico("{");
    
    while(atual().tipo != TOKEN_EOF && !(atual().tipo == TOKEN_DELIM && strcmp(atual().lexema, "}") == 0)){
        if(atual().tipo == TOKEN_ERROR || atual().tipo == TOKEN_COMMENT){
            avancar();
            continue;
        }
        adicionarFilhoAST(no, statement());
    }
    
    if(!matchLexema(TOKEN_DELIM, "}")) erroSintatico("}");
    return no;
}

ASTNode* statement(){
    ASTNode *no = NULL;
    
    // Declaração Local
    if(atual().tipo == TOKEN_KEYWORD &&(strcmp(atual().lexema, "int") == 0 || strcmp(atual().lexema, "float") == 0 || strcmp(atual().lexema, "char") == 0)){
        avancar();
        no = criarNoAST(AST_DECLARACAO, anterior().lexema);
        
        if(match(TOKEN_ID)){
            adicionarFilhoAST(no, criarNoAST(AST_IDENTIFICADOR, anterior().lexema));
            if(matchLexema(TOKEN_OP, "=")){
                adicionarFilhoAST(no, expressao());
            }
            if(!matchLexema(TOKEN_DELIM, ";")) erroSintatico(";");
        } else{
            erroSintatico("Identificador de variável");
            sincronizar();
        }
    }
    // Condição(IF)
    else if(matchLexema(TOKEN_KEYWORD, "if")){
        no = criarNoAST(AST_IF, "if");
        if(!matchLexema(TOKEN_DELIM, "(")) erroSintatico("(");
        adicionarFilhoAST(no, expressao());
        if(!matchLexema(TOKEN_DELIM, ")")) erroSintatico(")");
        adicionarFilhoAST(no, bloco());
    }
    // Repetição(WHILE)
    else if(matchLexema(TOKEN_KEYWORD, "while")){
        no = criarNoAST(AST_WHILE, "while");
        if(!matchLexema(TOKEN_DELIM, "(")) erroSintatico("(");
        adicionarFilhoAST(no, expressao());
        if(!matchLexema(TOKEN_DELIM, ")")) erroSintatico(")");
        adicionarFilhoAST(no, bloco());
    }
    // Atribuição
    else if(match(TOKEN_ID)){
        char nome_var[256];
        strcpy(nome_var, anterior().lexema);
        
        if(matchLexema(TOKEN_OP, "=")){
            no = criarNoAST(AST_ATRIBUICAO, "=");
            adicionarFilhoAST(no, criarNoAST(AST_IDENTIFICADOR, nome_var));
            adicionarFilhoAST(no, expressao());
        } else{
            pos--; //retrocede se for apenas expressão solta
            no = expressao();
        }
        if(!matchLexema(TOKEN_DELIM, ";")){ erroSintatico(";"); sincronizar(); }
    }
    else{
        erroSintatico("Comando válido(if, while, declaração ou atribuição)");
        sincronizar();
    }
    
    return no;
}

ASTNode* declaracaoGlobal(){
    if(atual().tipo == TOKEN_KEYWORD &&(strcmp(atual().lexema, "int") == 0 || strcmp(atual().lexema, "float") == 0 || strcmp(atual().lexema, "char") == 0 || strcmp(atual().lexema, "void") == 0)){
        char tipo[256];
        strcpy(tipo, atual().lexema);
        avancar(); 
        
        if(match(TOKEN_ID)){
            char nome[256];
            strcpy(nome, anterior().lexema);
            //se o próximo for '(', é uma função
            if(matchLexema(TOKEN_DELIM, "(")){
                char info_funcao[1024];
                snprintf(info_funcao, sizeof(info_funcao), "%s %s()", tipo, nome);
                ASTNode *no_funcao = criarNoAST(AST_FUNCAO, info_funcao);
                
                while(atual().tipo != TOKEN_EOF && !(atual().tipo == TOKEN_DELIM && strcmp(atual().lexema, ")") == 0)){
                    if(atual().tipo == TOKEN_KEYWORD){
                        char p_tipo[256];
                        strcpy(p_tipo, atual().lexema);
                        avancar();
                        if(match(TOKEN_ID)){
                            char p_info[1024];
                            snprintf(p_info, sizeof(p_info), "Param: %s %s", p_tipo, anterior().lexema);
                            adicionarFilhoAST(no_funcao, criarNoAST(AST_IDENTIFICADOR, p_info));
                        } else{
                            erroSintatico("Identificador do parâmetro");
                        }
                    } else{
                        erroSintatico("Tipo do parâmetro(ex: int, float)");
                        avancar(); 
                    }
                    
                    matchLexema(TOKEN_DELIM, ",");
                }
                
                if(!matchLexema(TOKEN_DELIM, ")")) erroSintatico(")");
                //o corpo da função obrigatoriamente é um bloco
                adicionarFilhoAST(no_funcao, bloco());
                return no_funcao;
            } 
            //caso contrário, é uma variável global comum
            else{
                ASTNode *no_decl = criarNoAST(AST_DECLARACAO, tipo);
                adicionarFilhoAST(no_decl, criarNoAST(AST_IDENTIFICADOR, nome));
                if(matchLexema(TOKEN_OP, "=")){
                    adicionarFilhoAST(no_decl, expressao());
                }
                if(!matchLexema(TOKEN_DELIM, ";")) erroSintatico(";");
                return no_decl;
            }
        } else{
            erroSintatico("Identificador esperado");
            sincronizar();
        }
    } else{
        erroSintatico("Declaração de função ou variável global");
        sincronizar();
    }
    return NULL;
}

int analisarSintaxe(TokenList *lista, ASTNode **raiz_ast){
    tokens = lista;
    pos = 0;
    tem_erro = 0;

    *raiz_ast = criarNoAST(AST_PROGRAMA, "PROGRAMA");

    //o laço principal agora opera no nível global externo do arquivo
    while(atual().tipo != TOKEN_EOF){
        if(atual().tipo == TOKEN_ERROR || atual().tipo == TOKEN_COMMENT){
            avancar();
            continue;
        }
        ASTNode *no_global = declaracaoGlobal();
        if(no_global){
            adicionarFilhoAST(*raiz_ast, no_global);
        }
    }
    return !tem_erro;
}

void imprimirAST(ASTNode *raiz, int nivel){
    if(!raiz) return;
    for(int i = 0; i < nivel; i++) printf("  ");
    
    printf("- %s", raiz->valor);
    if(raiz->tipo == AST_DECLARACAO) printf("(Declaracao)");
    else if(raiz->tipo == AST_ATRIBUICAO) printf("(Atribuicao)");
    else if(raiz->tipo == AST_EXPRESSAO_BINARIA) printf("(Operacao Binaria)");
    else if(raiz->tipo == AST_FUNCAO) printf("(Funcao)");
    else if(raiz->tipo == AST_IDENTIFICADOR) printf("(Identificador)");
    else if(raiz->tipo == AST_LITERAL) printf("(Literal)");
    else if(raiz->tipo == AST_BLOCO) printf("(Bloco)");
    else if(raiz->tipo == AST_IF) printf("(If)");
    else if(raiz->tipo == AST_WHILE) printf("(While)");
    else if(raiz->tipo == AST_CHAMADA_FUNCAO) printf("(Chamada de Funcao)");
    printf("\n");

    for(int i = 0; i < raiz->qtd_filhos; i++){
        imprimirAST(raiz->filhos[i], nivel + 1);
    }
}

void liberarAST(ASTNode *raiz){
    if(!raiz) return;
    for(int i = 0; i < raiz->qtd_filhos; i++) liberarAST(raiz->filhos[i]);
    free(raiz->filhos);
    free(raiz);
}