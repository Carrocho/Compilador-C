# Compilador em C — Analisador Léxico e Sintático

Este projeto implementa as duas primeiras fases de um compilador escrito em **C puro**: a **análise léxica** (scanner) e a **análise sintática** (parser), gerando uma **Árvore Sintática Abstrata (AST)** como saída.

---

## Estrutura do Projeto

```
/
├── main.c          # Ponto de entrada: orquestra léxico, sintático e saída
├── lexer.h         # Interface pública do analisador léxico
├── lexer.c         # Implementação do analisador léxico (tokenizador)
├── parser.h        # Interface pública do analisador sintático
├── parser.c        # Implementação do parser (descida recursiva) + AST
├── AST.h           # Definição dos tipos e funções da AST
├── Makefile        # Script de automação para compilação e execução
├── teste.txt       # Código de exemplo para teste normal
└── testeErros.txt  # Código de exemplo com erros léxicos e sintáticos
```

---

## Funcionalidades

### Fase 1 — Análise Léxica (`lexer.c`)

Lê o arquivo-fonte e o converte em uma lista de tokens usando expressões regulares POSIX (`regex.h`). A análise **não é interrompida por erros**: tokens inválidos são classificados e a leitura continua.

Tokens reconhecidos:

| Tipo         | Exemplos                                      |
|--------------|-----------------------------------------------|
| `P_CHAVE`    | `int`, `float`, `char`, `void`, `if`, `while`, `for`, `do`, `return`, `else` |
| `ID`         | `main`, `contador`, `calcular_soma`           |
| `INTEIRO`    | `0`, `42`, `100`                              |
| `FLOAT`      | `3.14`, `222.2`                               |
| `STRING`     | `"texto"`                                     |
| `OPERADOR`   | `+`, `-`, `*`, `/`, `>=`, `==`, `++`, `&&`    |
| `DELIMITADOR`| `(`, `)`, `{`, `}`, `[`, `]`, `;`, `,`       |
| `COMENTARIO` | `/* ... */`                                   |

Erros léxicos identificados:

| Tipo          | Exemplo             | Descrição                            |
|---------------|---------------------|--------------------------------------|
| `ERRO_STR`    | `"texto sem fechar` | String aberta sem fechamento         |
| `ERRO_NUM`    | `3.14.15`           | Número com múltiplos pontos          |
| `ERRO_ID`     | `1contador`, `@var` | Identificador iniciado com dígito ou símbolo inválido |
| `ERRO LÉXICO` | `$$`, `??`          | Caracteres sem padrão reconhecido    |

### Fase 2 — Análise Sintática (`parser.c`)

Parser de **descida recursiva** que consome a lista de tokens gerada pelo léxico e constrói uma AST. Reconhece os seguintes construtos:

- **Declarações** — variáveis globais e locais (`int x = 10;`) e funções com parâmetros
- **Atribuições** — `x = expressao;`
- **Condições** — `if (expr) { ... }`
- **Repetições** — `while (expr) { ... }`
- **Expressões** — aritméticas (`+`, `-`, `*`, `/`) e de chamada de função (`f(a, b)`)

#### Recuperação de Erros

Quando um erro sintático é encontrado, o parser **não para**: entra em modo de recuperação (`sincronizar`) e descarta tokens até encontrar um ponto seguro (`;`, `}` ou início de palavra-chave), permitindo que erros subsequentes também sejam reportados.

### Árvore Sintática Abstrata (AST)

Definida em `AST.h`, a AST representa a estrutura hierárquica do programa. Tipos de nós:

| Nó                   | Descrição                          |
|----------------------|------------------------------------|
| `AST_PROGRAMA`       | Raiz da árvore                     |
| `AST_FUNCAO`         | Declaração de função               |
| `AST_DECLARACAO`     | Declaração de variável             |
| `AST_ATRIBUICAO`     | Atribuição de valor                |
| `AST_IF`             | Estrutura condicional              |
| `AST_WHILE`          | Estrutura de repetição             |
| `AST_BLOCO`          | Bloco de comandos `{ ... }`        |
| `AST_EXPRESSAO_BINARIA` | Operação com dois operandos     |
| `AST_CHAMADA_FUNCAO` | Chamada de função com argumentos   |
| `AST_IDENTIFICADOR`  | Variável ou nome                   |
| `AST_LITERAL`        | Valor literal (número ou string)   |

---

## Como Executar

Certifique-se de ter o GCC instalado e todos os arquivos na mesma pasta.

### Compilar e executar (tudo de uma vez)

```bash
make
```

Compila o projeto e já executa sobre `teste.txt`.

### Apenas executar (sem recompilar)

```bash
make run
```

### Limpar o executável gerado

```bash
make clean
```

### Executar manualmente com outro arquivo

```bash
./compilador testeErros.txt
```

---

## Saída do Programa

A saída tem duas seções.

### 1. Tabela de Tokens (Léxico)

```
LEXEMA                    | TIPO         | LINHA  | COLUNA
--------------------------|--------------|--------|--------
int                       | P_CHAVE      | 1      | 1
calcular_soma             | ID           | 1      | 5
(                         | DELIMITADOR  | 1      | 19
...
```

### 2. Resultado Sintático + AST

Em caso de **sucesso**:
```
Analisador sintático
SUCESSO!

--- ÁRVORE SINTÁTICA ABSTRATA (AST) ---
- PROGRAMA
  - int void main()(Funcao)
    - {}(Bloco)
      - =(Atribuicao)
        - contador(Identificador)
        - 0(Literal)
      - if(If)
      ...
```

Em caso de **erro**:
```
Analisador sintático
[ERRO SINTÁTICO] Linha 3, Col: 1 -> Esperado: Declaração de função ou variável global | Encontrado: 'x'
ERRO!
```

Os erros sintáticos indicam a **linha**, a **coluna**, o que era **esperado** e o que foi **encontrado**.

---

## Exemplos de Teste

### `teste.txt` — Código válido

Contém variável global, função com parâmetros, declarações locais, atribuições, `if` e `while`.
O compilador deve reportar `SUCESSO!` e exibir a AST completa.

### `testeErros.txt` — Código com erros intencionais

Contém erros léxicos (`@taxa_invalida`, `1contador`, `3.14.15`, string não fechada) e erros sintáticos (atribuição global sem tipo, parâmetro sem tipo, `;` faltando, `if` sem `)`). Útil para verificar a capacidade de recuperação do compilador.

---

*Desenvolvido para estudos de Compiladores e Análise Léxica.*
