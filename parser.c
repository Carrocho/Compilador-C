#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.h"

static TokenList *tokens;
static int pos = 0;
static int tem_erro = 0;

// ============================================================================
// TABELA DE SÍMBOLOS E ANÁLISE SEMÂNTICA
// ============================================================================
#define MAX_SIMB 2000

typedef enum{ SIM_VAR, SIM_FUNC } SimCategoria;

typedef struct{
    char nome[256];
    char tipo[30];
    SimCategoria categoria;
    int escopo;
    int inicializada;
    int usada;
    int linha;
    int coluna;
} Simbolo;

static Simbolo tabela_simbolos[MAX_SIMB];
static int totalSimbolos = 0;
static int escopoAtual = 0;

void sairEscopo(){
    for (int i = 0; i < totalSimbolos; i++){
        if (tabela_simbolos[i].escopo == escopoAtual){
            if (!tabela_simbolos[i].usada && tabela_simbolos[i].categoria == SIM_VAR){
                printf("[AVISO SEMÂNTICO] Linha %d, Col: %d -> Variável '%s' declarada, mas nunca usada.\n",
                       tabela_simbolos[i].linha, tabela_simbolos[i].coluna, tabela_simbolos[i].nome);
            }
        }
    }
    
    int novoTotal = 0;
    for (int i = 0; i < totalSimbolos; i++){
        if (tabela_simbolos[i].escopo < escopoAtual){
            tabela_simbolos[novoTotal++] = tabela_simbolos[i];
        }
    }
    totalSimbolos = novoTotal;
    escopoAtual--;
}

int buscarSimboloEscopoAtual(const char* nome){
    for (int i = totalSimbolos - 1; i >= 0; i--){
        if (tabela_simbolos[i].escopo < escopoAtual) break;
        if (strcmp(tabela_simbolos[i].nome, nome) == 0) return i;
    }
    return -1;
}

int buscarSimboloGlobal(const char* nome){
    for (int i = totalSimbolos - 1; i >= 0; i--){
        if (strcmp(tabela_simbolos[i].nome, nome) == 0) return i;
    }
    return -1;
}

void inserirSimbolo(const char* nome, const char* tipo, SimCategoria cat, int linha, int coluna){
    if (buscarSimboloEscopoAtual(nome) != -1){
        printf("[ERRO SEMÂNTICO] Linha %d, Col: %d -> '%s' já foi declarado neste escopo.\n", linha, coluna, nome);
        tem_erro = 1;
        return;
    }
    if (totalSimbolos >= MAX_SIMB) return;

    strcpy(tabela_simbolos[totalSimbolos].nome, nome);
    strcpy(tabela_simbolos[totalSimbolos].tipo, tipo);
    tabela_simbolos[totalSimbolos].categoria = cat;
    tabela_simbolos[totalSimbolos].escopo = escopoAtual;
    tabela_simbolos[totalSimbolos].inicializada = 0;
    tabela_simbolos[totalSimbolos].usada = 0;
    tabela_simbolos[totalSimbolos].linha = linha;
    tabela_simbolos[totalSimbolos].coluna = coluna;
    totalSimbolos++;
}

void verificarUsoSimbolo(const char* nome, int linha, int coluna, int is_atribuicao){
    int idx = buscarSimboloGlobal(nome);
    if (idx != -1){
        if (is_atribuicao){
            tabela_simbolos[idx].inicializada = 1;
        } else{
            tabela_simbolos[idx].usada = 1;
            if (!tabela_simbolos[idx].inicializada && tabela_simbolos[idx].categoria == SIM_VAR){
                printf("[AVISO SEMÂNTICO] Linha %d, Col: %d -> Variável '%s' usada sem ser inicializada.\n", linha, coluna, nome);
            }
        }
    } else{
        printf("[ERRO SEMÂNTICO] Linha %d, Col: %d -> '%s' não foi declarado.\n", linha, coluna, nome);
        tem_erro = 1;
    }
}

// Tabela de Operadores: Define o tipo resultante de uma operação binária
const char* inferirTipo(const char* t1, const char* t2, const char* op, int linha, int coluna){
    if (strcmp(t1, "erro") == 0 || strcmp(t2, "erro") == 0) return "erro";

    if (strcmp(op, "==") == 0 || strcmp(op, "!=") == 0 || strcmp(op, ">=") == 0 || 
        strcmp(op, "<=") == 0 || strcmp(op, ">") == 0 || strcmp(op, "<") == 0){
        return "int"; 
    }

    if (strcmp(t1, "string") == 0 || strcmp(t2, "string") == 0){
        if (linha > 0){ // Se linha > 0, dispara o erro. Se 0, é só uma consulta interna.
            printf("[ERRO SEMÂNTICO] Linha %d, Col: %d -> Operação '%s' inválida com tipo 'string'.\n", linha, coluna, op);
            tem_erro = 1;
        }
        return "erro";
    }

    if (strcmp(t1, "float") == 0 || strcmp(t2, "float") == 0){
        return "float";
    }

    return "int";
}

const char* obterTipoExpressao(ASTNode *no){
    if (!no) return "void";

    if (no->tipo == AST_LITERAL){
        if (strchr(no->valor, '.')) return "float";
        if (no->valor[0] == '"') return "string";
        if (no->valor[0] == '\'') return "char";
        return "int";
    }

    if (no->tipo == AST_IDENTIFICADOR || no->tipo == AST_CHAMADA_FUNCAO){
        int idx = buscarSimboloGlobal(no->valor);
        if (idx != -1) return tabela_simbolos[idx].tipo;
        return "erro";
    }

    if (no->tipo == AST_EXPRESSAO_BINARIA){
        const char* t1 = obterTipoExpressao(no->filhos[0]);
        const char* t2 = obterTipoExpressao(no->filhos[1]);
        return inferirTipo(t1, t2, no->valor, 0, 0); 
    }

    return "void";
}

void verificarCompatibilidadeAtribuicao(const char* tipo_esq, const char* tipo_dir, int linha, int coluna){
    if (strcmp(tipo_esq, "erro") == 0 || strcmp(tipo_dir, "erro") == 0) return;
    if (strcmp(tipo_esq, tipo_dir) == 0) return; // Tipos iguais, tudo certo

    // Promoção implícita segura
    if (strcmp(tipo_esq, "float") == 0 && strcmp(tipo_dir, "int") == 0) return; 
    
    // Possível perda de dados
    if (strcmp(tipo_esq, "int") == 0 && strcmp(tipo_dir, "float") == 0){
        printf("[AVISO SEMÂNTICO] Linha %d, Col: %d -> Atribuição de 'float' para 'int' causa perda de casas decimais.\n", linha, coluna);
        return;
    }

    // char é flexível no C
    if (strcmp(tipo_esq, "char") == 0 && strcmp(tipo_dir, "int") == 0) return;
    if (strcmp(tipo_esq, "int") == 0 && strcmp(tipo_dir, "char") == 0) return;

    if (strcmp(tipo_esq, "char") == 0 && strcmp(tipo_dir, "string") == 0) return;

    // Erro crítico de tipos incompativeis
    if (strcmp(tipo_dir, "string") == 0 && (strcmp(tipo_esq, "int") == 0 || strcmp(tipo_esq, "float") == 0)){
        printf("[ERRO SEMÂNTICO] Linha %d, Col: %d -> Tipos incompatíveis para variável '%s'.\n", linha, coluna, tipo_esq);
        tem_erro = 1;
    }
}

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
    printf("[ERRO SINTÁTICO] Linha %d, Col: %d -> Esperado: %s | Encontrado: '%s'\n", t.linha, t.coluna, esperado, t.lexema);
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
ASTNode* bloco(int criarEscopo);

// fator -> NUMERO | STRING | ID | ID '(' argumentos ')' | '(' expressao ')'
ASTNode* fator(){
    if(match(TOKEN_INT) || match(TOKEN_FLOAT) || match(TOKEN_STRING)){
        return criarNoAST(AST_LITERAL, anterior().lexema);
    }

    //se for um Identificador(variável ou função)
    if(match(TOKEN_ID)){
        char nome[256];
        strcpy(nome, anterior().lexema);
        int linha_id = anterior().linha;
        int coluna_id = anterior().coluna;
        
        //ve se é uma chamada de função
        if(matchLexema(TOKEN_DELIM, "(")){
            verificarUsoSimbolo(nome, linha_id, coluna_id, 0);
            ASTNode *chamada = criarNoAST(AST_CHAMADA_FUNCAO, nome);
            //lê a lista de argumentos, se a função não for vazia
            if(!(atual().tipo == TOKEN_DELIM && strcmp(atual().lexema, ")") == 0)){
                adicionarFilhoAST(chamada, expressao()); // lê o primeiro argumento
                
                //se tiver vírgula, lê os próximos
                while(matchLexema(TOKEN_DELIM, ",")){
                    adicionarFilhoAST(chamada, expressao());
                }
            }
            if(!matchLexema(TOKEN_DELIM, ")")) erroSintatico(")");
            return chamada;
        }
        verificarUsoSimbolo(nome, linha_id, coluna_id, 0);
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
        Token tokenAnterior = anterior();
        ASTNode *novo_no = criarNoAST(AST_EXPRESSAO_BINARIA, tokenAnterior.lexema);
        adicionarFilhoAST(novo_no, no);
        
        ASTNode *dir = fator();
        adicionarFilhoAST(novo_no, dir);
        
        // Verifica Tipos na Tabela de Operadores
        const char* tipo_esq = obterTipoExpressao(no);
        const char* tipo_dir = obterTipoExpressao(dir);
        inferirTipo(tipo_esq, tipo_dir, tokenAnterior.lexema, tokenAnterior.linha, tokenAnterior.coluna);
        
        no = novo_no;
    }
    return no;
}

ASTNode* expressao(){
    ASTNode *no = termo();
    while(atual().tipo == TOKEN_OP){
        Token tokenAnterior = atual();
        avancar();
        ASTNode *novo_no = criarNoAST(AST_EXPRESSAO_BINARIA, tokenAnterior.lexema);
        adicionarFilhoAST(novo_no, no);
        
        ASTNode *dir = termo();
        adicionarFilhoAST(novo_no, dir);
        
        // Verifica Tipos na Tabela de Operadores
        const char* tipo_esq = obterTipoExpressao(no);
        const char* tipo_dir = obterTipoExpressao(dir);
        inferirTipo(tipo_esq, tipo_dir, tokenAnterior.lexema, tokenAnterior.linha, tokenAnterior.coluna);
        
        no = novo_no;
    }
    return no;
}

ASTNode* bloco(int criarEscopo){
    ASTNode *no = criarNoAST(AST_BLOCO, "{}");
    if(!matchLexema(TOKEN_DELIM, "{")) erroSintatico("{");
    if (criarEscopo) escopoAtual++;
    
    while(atual().tipo != TOKEN_EOF && !(atual().tipo == TOKEN_DELIM && strcmp(atual().lexema, "}") == 0)){
        if(atual().tipo == TOKEN_ERROR || atual().tipo == TOKEN_COMMENT){ avancar(); continue; }
        adicionarFilhoAST(no, statement());
    }
    
    if (criarEscopo) sairEscopo();
    if(!matchLexema(TOKEN_DELIM, "}")) erroSintatico("}");
    return no;
}

ASTNode* statement(){
    ASTNode *no = NULL;
    
    if(atual().tipo == TOKEN_KEYWORD &&(strcmp(atual().lexema, "int") == 0 || strcmp(atual().lexema, "float") == 0 || strcmp(atual().lexema, "char") == 0)){
        char tipo[30];
        strcpy(tipo, atual().lexema);
        avancar();
        no = criarNoAST(AST_DECLARACAO, anterior().lexema);
        
        if(match(TOKEN_ID)){
            char nome_var[256];
            strcpy(nome_var, anterior().lexema);
            int linha_var = anterior().linha;
            int coluna_var = anterior().coluna;
            
            inserirSimbolo(nome_var, tipo, SIM_VAR, linha_var, coluna_var);
            adicionarFilhoAST(no, criarNoAST(AST_IDENTIFICADOR, nome_var));
            
            if(matchLexema(TOKEN_OP, "=")){
                ASTNode *expr = expressao();
                adicionarFilhoAST(no, expr);
                verificarUsoSimbolo(nome_var, linha_var, coluna_var, 1);
                
                // Validação de tipos na atribuição
                verificarCompatibilidadeAtribuicao(tipo, obterTipoExpressao(expr), linha_var, coluna_var);
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
        adicionarFilhoAST(no, bloco(1));
    }
    // Repetição(WHILE)
    else if(matchLexema(TOKEN_KEYWORD, "while")){
        no = criarNoAST(AST_WHILE, "while");
        if(!matchLexema(TOKEN_DELIM, "(")) erroSintatico("(");
        adicionarFilhoAST(no, expressao());
        if(!matchLexema(TOKEN_DELIM, ")")) erroSintatico(")");
        adicionarFilhoAST(no, bloco(1));
    }
    // Atribuição
    else if(match(TOKEN_ID)){
        char nome_var[256];
        strcpy(nome_var, anterior().lexema);
        int linha_var = anterior().linha;
        int coluna_var = anterior().coluna;
        
        if(matchLexema(TOKEN_OP, "=")){
            no = criarNoAST(AST_ATRIBUICAO, "=");
            adicionarFilhoAST(no, criarNoAST(AST_IDENTIFICADOR, nome_var));
            
            ASTNode *expr = expressao();
            adicionarFilhoAST(no, expr);
            verificarUsoSimbolo(nome_var, linha_var, coluna_var, 1);
            
            // Validação de tipos na atribuição
            int idx = buscarSimboloGlobal(nome_var);
            if (idx != -1) verificarCompatibilidadeAtribuicao(tabela_simbolos[idx].tipo, obterTipoExpressao(expr), linha_var, coluna_var);
            
        } else{
            pos--; 
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
        char tipo[30];
        strcpy(tipo, atual().lexema);
        avancar(); 
        
        if(match(TOKEN_ID)){
            char nome[256];
            strcpy(nome, anterior().lexema);
            int linha_decl = anterior().linha;
            int coluna_decl = anterior().coluna;
            
            if(matchLexema(TOKEN_DELIM, "(")){
                inserirSimbolo(nome, tipo, SIM_FUNC, linha_decl, coluna_decl);
                escopoAtual++; 
                
                char info_funcao[1024];
                snprintf(info_funcao, sizeof(info_funcao), "%s %s()", tipo, nome);
                ASTNode *no_funcao = criarNoAST(AST_FUNCAO, info_funcao);
                
                while(atual().tipo != TOKEN_EOF && !(atual().tipo == TOKEN_DELIM && strcmp(atual().lexema, ")") == 0)){
                    if(atual().tipo == TOKEN_KEYWORD){
                        char p_tipo[30];
                        strcpy(p_tipo, atual().lexema);
                        avancar();
                        
                        if(match(TOKEN_ID)){
                            char p_nome[256];
                            strcpy(p_nome, anterior().lexema);
                            int p_linha = anterior().linha;
                            int p_coluna = anterior().coluna;
                            
                            inserirSimbolo(p_nome, p_tipo, SIM_VAR, p_linha, p_coluna); 
                            verificarUsoSimbolo(p_nome, p_linha, p_coluna, 1); 
                            
                            char p_info[1024];
                            snprintf(p_info, sizeof(p_info), "Param: %s %s", p_tipo, p_nome);
                            adicionarFilhoAST(no_funcao, criarNoAST(AST_IDENTIFICADOR, p_info));
                        } else{ erroSintatico("Identificador do parâmetro"); }
                    } else{
                        erroSintatico("Tipo do parâmetro(ex: int, float)");
                        avancar(); 
                    }
                    matchLexema(TOKEN_DELIM, ",");
                }
                
                if(!matchLexema(TOKEN_DELIM, ")")) erroSintatico(")");
                
                adicionarFilhoAST(no_funcao, bloco(0)); 
                sairEscopo(); 
                return no_funcao;
            } 
            else{
                inserirSimbolo(nome, tipo, SIM_VAR, linha_decl, coluna_decl);
                ASTNode *no_decl = criarNoAST(AST_DECLARACAO, tipo);
                adicionarFilhoAST(no_decl, criarNoAST(AST_IDENTIFICADOR, nome));
                
                if(matchLexema(TOKEN_OP, "=")){
                    ASTNode *expr = expressao();
                    adicionarFilhoAST(no_decl, expr);
                    verificarUsoSimbolo(nome, linha_decl, coluna_decl, 1);
                    verificarCompatibilidadeAtribuicao(tipo, obterTipoExpressao(expr), linha_decl, coluna_decl);
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
    tokens = lista; pos = 0; tem_erro = 0; totalSimbolos = 0; escopoAtual = 0;
    escopoAtual++; 

    *raiz_ast = criarNoAST(AST_PROGRAMA, "PROGRAMA");

    while(atual().tipo != TOKEN_EOF){
        if(atual().tipo == TOKEN_ERROR || atual().tipo == TOKEN_COMMENT){ avancar(); continue; }
        ASTNode *no_global = declaracaoGlobal();
        if(no_global) adicionarFilhoAST(*raiz_ast, no_global);
    }
    
    sairEscopo(); 
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
    else if(raiz->tipo == AST_BLOCO) printf("(bloco(1))");
    else if(raiz->tipo == AST_IF) printf("(If)");
    else if(raiz->tipo == AST_WHILE) printf("(While)");
    else if(raiz->tipo == AST_CHAMADA_FUNCAO) printf("(Chamada de Funcao)");
    printf("\n");

    for(int i = 0; i < raiz->qtd_filhos; i++) imprimirAST(raiz->filhos[i], nivel + 1);
}

void liberarAST(ASTNode *raiz){
    if(!raiz) return;
    for(int i = 0; i < raiz->qtd_filhos; i++) liberarAST(raiz->filhos[i]);
    free(raiz->filhos);
    free(raiz);
}