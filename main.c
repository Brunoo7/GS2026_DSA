/*
 * Citizen Assist AI - Global Solution 2026.2 (DSA)
 * Recorte: Documentação e Cidadania
 *
 * Programa completo em um único arquivo:
 *   1) estruturas e protótipos (Servico, No, Lista)
 *   2) lista encadeada, busca, Merge Sort e indicador
 *   3) menu interativo (terminal)
 *
 * Compilar:  gcc -std=c99 -Wall -Wextra -o citizen_assist main.c
 * Executar:  ./citizen_assist      (Windows: citizen_assist.exe)
 *
 * Windows: o programa configura o terminal para UTF-8 ao iniciar, para
 * exibir acentos corretamente. Salve este arquivo como UTF-8.
 */
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>   /* SetConsoleOutputCP: acentos corretos no terminal do Windows */
#endif

/* ================================================================== */
/* 1) ESTRUTURAS E PROTÓTIPOS                                          */
/* ================================================================== */

#define TAM_TITULO     100
#define TAM_CATEGORIA   50
#define TAM_DESCRICAO  200
#define TAM_REQUISITOS 200
#define TAM_LOCAL      100

/* Registro de um serviço ao cidadão. */
typedef struct {
    int  id;
    char titulo[TAM_TITULO];
    char categoria[TAM_CATEGORIA];
    char descricao[TAM_DESCRICAO];
    char requisitos[TAM_REQUISITOS];
    char local[TAM_LOCAL];
} Servico;

/* Nó da lista: dado + ponteiro para o próximo nó (alocado com malloc). */
typedef struct No {
    Servico    dado;
    struct No *proximo;
} No;

/* Lista encadeada simples com ponteiros de cabeça e cauda. */
typedef struct {
    No *cabeca;
    No *cauda;
    int tamanho;
} Lista;

typedef enum {
    LISTA_OK = 0,
    LISTA_ERRO_INVALIDO,
    LISTA_ERRO_DUPLICADO,
    LISTA_ERRO_MEMORIA,
    LISTA_NAO_ENCONTRADO
} Resultado;

typedef enum { CAMPO_TITULO = 1, CAMPO_CATEGORIA = 2 } CampoBusca;

typedef enum {
    CHAVE_ID = 1,
    CHAVE_TITULO,
    CHAVE_CATEGORIA,
    CHAVE_LOCAL
} ChaveOrdenacao;

/* Indicador: quantidade de registros por categoria. */
typedef struct {
    char categoria[TAM_CATEGORIA];
    int  quantidade;
} Contagem;

/* Função chamada para cada registro encontrado em uma pesquisa. */
typedef void (*VisitaFn)(const Servico *servico, void *contexto);

void      lista_iniciar(Lista *l);
int       lista_vazia(const Lista *l);
Resultado lista_inserir_fim(Lista *l, const Servico *s);
Servico  *lista_buscar_id(const Lista *l, int id);
int       lista_pesquisar(const Lista *l, CampoBusca campo, const char *termo,
                          VisitaFn visita, void *contexto);
Resultado lista_atualizar(Lista *l, int id, const char *titulo,
                          const char *categoria, const char *descricao,
                          const char *requisitos, const char *local);
Resultado lista_remover(Lista *l, int id);
Resultado lista_ordenar(Lista *l, ChaveOrdenacao chave, int reverso);
Contagem *lista_contar_por_categoria(const Lista *l, int *quantidade_categorias);
void      lista_liberar(Lista *l);

/* ================================================================== */
/* 2) LISTA ENCADEADA, BUSCA, ORDENAÇÃO E INDICADOR                    */
/* ================================================================== */

/* ------------------------------------------------------------------ */
/* Funções auxiliares de texto                                         */
/* ------------------------------------------------------------------ */

/* Remove espaços do início e do fim (in-place). */
static void aparar(char *s)
{
    size_t ini = 0, fim = strlen(s);

    while (s[ini] != '\0' && isspace((unsigned char)s[ini])) {
        ini++;
    }
    while (fim > ini && isspace((unsigned char)s[fim - 1])) {
        fim--;
    }
    memmove(s, s + ini, fim - ini);
    s[fim - ini] = '\0';
}

/* Copia src para dest (no máximo tam-1 caracteres) e apara os espaços. */
static void copiar_aparado(char *dest, size_t tam, const char *src)
{
    snprintf(dest, tam, "%s", src != NULL ? src : "");
    aparar(dest);
}

/* Comparação sem diferenciar maiúsculas (ASCII). */
static int comparar_ci(const char *a, const char *b)
{
    while (*a != '\0' && *b != '\0') {
        int ca = tolower((unsigned char)*a);
        int cb = tolower((unsigned char)*b);
        if (ca != cb) {
            return ca - cb;
        }
        a++;
        b++;
    }
    return tolower((unsigned char)*a) - tolower((unsigned char)*b);
}

/* 1 se 'termo' aparece em 'texto' (sem diferenciar maiúsculas). */
static int contem_ci(const char *texto, const char *termo)
{
    size_t n = strlen(texto), m = strlen(termo), i, j;

    if (m == 0) {
        return 1;
    }
    for (i = 0; i + m <= n; i++) {
        for (j = 0; j < m; j++) {
            if (tolower((unsigned char)texto[i + j]) !=
                tolower((unsigned char)termo[j])) {
                break;
            }
        }
        if (j == m) {
            return 1;
        }
    }
    return 0;
}

/* ------------------------------------------------------------------ */
/* Lista encadeada                                                     */
/* ------------------------------------------------------------------ */

void lista_iniciar(Lista *l)
{
    l->cabeca = NULL;
    l->cauda = NULL;
    l->tamanho = 0;
}

int lista_vazia(const Lista *l)
{
    return l->cabeca == NULL;
}

/*
 * Insere no fim da lista (usa o ponteiro de cauda).
 * Valida os campos e rejeita ID duplicado (percorre a lista: O(n)).
 */
Resultado lista_inserir_fim(Lista *l, const Servico *s)
{
    Servico tmp;
    No *novo;

    if (l == NULL || s == NULL) {
        return LISTA_ERRO_INVALIDO;
    }

    tmp.id = s->id;
    copiar_aparado(tmp.titulo, sizeof tmp.titulo, s->titulo);
    copiar_aparado(tmp.categoria, sizeof tmp.categoria, s->categoria);
    copiar_aparado(tmp.descricao, sizeof tmp.descricao, s->descricao);
    copiar_aparado(tmp.requisitos, sizeof tmp.requisitos, s->requisitos);
    copiar_aparado(tmp.local, sizeof tmp.local, s->local);

    if (tmp.id <= 0 || tmp.titulo[0] == '\0' || tmp.categoria[0] == '\0') {
        return LISTA_ERRO_INVALIDO;
    }
    if (lista_buscar_id(l, tmp.id) != NULL) {
        return LISTA_ERRO_DUPLICADO;
    }

    novo = (No *)malloc(sizeof(No));
    if (novo == NULL) {
        return LISTA_ERRO_MEMORIA;
    }
    novo->dado = tmp;
    novo->proximo = NULL;

    if (l->cabeca == NULL) {
        l->cabeca = novo;
        l->cauda = novo;
    } else {
        l->cauda->proximo = novo;
        l->cauda = novo;
    }
    l->tamanho++;
    return LISTA_OK;
}

/* Busca sequencial por identificador. Retorna NULL se não encontrar. */
Servico *lista_buscar_id(const Lista *l, int id)
{
    No *atual = l->cabeca;

    while (atual != NULL) {
        if (atual->dado.id == id) {
            return &atual->dado;
        }
        atual = atual->proximo;
    }
    return NULL;
}

/*
 * Busca sequencial por trecho do título ou da categoria.
 * Chama visita() para cada registro encontrado.
 * Retorna a quantidade encontrada, ou -1 se os argumentos forem inválidos.
 */
int lista_pesquisar(const Lista *l, CampoBusca campo, const char *termo,
                    VisitaFn visita, void *contexto)
{
    char busca[TAM_TITULO];
    const No *atual;
    int achados = 0;

    if (l == NULL || termo == NULL ||
        (campo != CAMPO_TITULO && campo != CAMPO_CATEGORIA)) {
        return -1;
    }
    copiar_aparado(busca, sizeof busca, termo);
    if (busca[0] == '\0') {
        return -1;
    }

    for (atual = l->cabeca; atual != NULL; atual = atual->proximo) {
        const char *texto = (campo == CAMPO_TITULO) ? atual->dado.titulo
                                                    : atual->dado.categoria;
        if (contem_ci(texto, busca)) {
            achados++;
            if (visita != NULL) {
                visita(&atual->dado, contexto);
            }
        }
    }
    return achados;
}

/* Troca o texto de um campo, a menos que 'novo' seja NULL ou em branco. */
static void atualizar_campo(char *destino, size_t tam, const char *novo)
{
    char buf[TAM_DESCRICAO];

    if (novo == NULL) {
        return;
    }
    copiar_aparado(buf, tam < sizeof buf ? tam : sizeof buf, novo);
    if (buf[0] != '\0') {
        snprintf(destino, tam, "%s", buf);
    }
}

/*
 * Atualiza os campos do registro. Parâmetro NULL ou em branco = manter o
 * valor atual. O ID nunca é alterado.
 */
Resultado lista_atualizar(Lista *l, int id, const char *titulo,
                          const char *categoria, const char *descricao,
                          const char *requisitos, const char *local)
{
    Servico *s;

    if (l == NULL) {
        return LISTA_ERRO_INVALIDO;
    }
    s = lista_buscar_id(l, id);
    if (s == NULL) {
        return LISTA_NAO_ENCONTRADO;
    }

    atualizar_campo(s->titulo, sizeof s->titulo, titulo);
    atualizar_campo(s->categoria, sizeof s->categoria, categoria);
    atualizar_campo(s->descricao, sizeof s->descricao, descricao);
    atualizar_campo(s->requisitos, sizeof s->requisitos, requisitos);
    atualizar_campo(s->local, sizeof s->local, local);
    return LISTA_OK;
}

/*
 * Remove o nó com o ID dado: religa os vizinhos, atualiza cabeça/cauda
 * e libera a memória com free().
 */
Resultado lista_remover(Lista *l, int id)
{
    No *anterior = NULL;
    No *atual;

    if (l == NULL) {
        return LISTA_ERRO_INVALIDO;
    }
    atual = l->cabeca;
    while (atual != NULL && atual->dado.id != id) {
        anterior = atual;
        atual = atual->proximo;
    }
    if (atual == NULL) {
        return LISTA_NAO_ENCONTRADO;
    }

    if (anterior == NULL) {            /* remoção da cabeça */
        l->cabeca = atual->proximo;
    } else {                           /* meio ou cauda */
        anterior->proximo = atual->proximo;
    }
    if (atual == l->cauda) {           /* remoção da cauda */
        l->cauda = anterior;
    }
    free(atual);
    l->tamanho--;
    return LISTA_OK;
}

/* Libera todos os nós e deixa a lista vazia. */
void lista_liberar(Lista *l)
{
    No *atual, *proximo;

    if (l == NULL) {
        return;
    }
    atual = l->cabeca;
    while (atual != NULL) {
        proximo = atual->proximo;
        free(atual);
        atual = proximo;
    }
    lista_iniciar(l);
}

/* ------------------------------------------------------------------ */
/* Ordenação: Merge Sort sobre os nós (religando ponteiros)            */
/* ------------------------------------------------------------------ */

static int comparar(const Servico *a, const Servico *b, ChaveOrdenacao chave)
{
    switch (chave) {
    case CHAVE_ID:        return (a->id > b->id) - (a->id < b->id);
    case CHAVE_TITULO:    return comparar_ci(a->titulo, b->titulo);
    case CHAVE_CATEGORIA: return comparar_ci(a->categoria, b->categoria);
    case CHAVE_LOCAL:     return comparar_ci(a->local, b->local);
    }
    return 0;
}

/* Intercala duas listas já ordenadas. Estável: em empate, pega da esquerda. */
static No *intercalar(No *a, No *b, ChaveOrdenacao chave, int reverso)
{
    No sentinela;
    No *ponta = &sentinela;

    sentinela.proximo = NULL;
    while (a != NULL && b != NULL) {
        int c = comparar(&a->dado, &b->dado, chave);
        int pega_a = reverso ? (c >= 0) : (c <= 0);

        if (pega_a) {
            ponta->proximo = a;
            a = a->proximo;
        } else {
            ponta->proximo = b;
            b = b->proximo;
        }
        ponta = ponta->proximo;
    }
    ponta->proximo = (a != NULL) ? a : b;
    return sentinela.proximo;
}

static No *merge_sort(No *cabeca, ChaveOrdenacao chave, int reverso)
{
    No *lento, *rapido, *segunda, *esq, *dir;

    if (cabeca == NULL || cabeca->proximo == NULL) {
        return cabeca;
    }

    /* divide ao meio: quando 'rapido' chega ao fim, 'lento' está no meio */
    lento = cabeca;
    rapido = cabeca->proximo;
    while (rapido != NULL && rapido->proximo != NULL) {
        lento = lento->proximo;
        rapido = rapido->proximo->proximo;
    }
    segunda = lento->proximo;
    lento->proximo = NULL;

    esq = merge_sort(cabeca, chave, reverso);
    dir = merge_sort(segunda, chave, reverso);
    return intercalar(esq, dir, chave, reverso);
}

Resultado lista_ordenar(Lista *l, ChaveOrdenacao chave, int reverso)
{
    if (l == NULL ||
        (chave != CHAVE_ID && chave != CHAVE_TITULO &&
         chave != CHAVE_CATEGORIA && chave != CHAVE_LOCAL)) {
        return LISTA_ERRO_INVALIDO;
    }

    l->cabeca = merge_sort(l->cabeca, chave, reverso);

    /* reposiciona a cauda */
    l->cauda = l->cabeca;
    while (l->cauda != NULL && l->cauda->proximo != NULL) {
        l->cauda = l->cauda->proximo;
    }
    return LISTA_OK;
}

/* ------------------------------------------------------------------ */
/* Indicador                                                           */
/* ------------------------------------------------------------------ */

/*
 * Devolve um vetor (alocado com calloc) com a quantidade de registros por
 * categoria, na ordem em que cada categoria aparece. O chamador deve
 * liberar o vetor com free(). Retorna NULL se a lista estiver vazia.
 */
Contagem *lista_contar_por_categoria(const Lista *l, int *quantidade_categorias)
{
    Contagem *v;
    const No *p;
    int k = 0, i;

    *quantidade_categorias = 0;
    if (l == NULL || l->tamanho == 0) {
        return NULL;
    }
    v = (Contagem *)calloc((size_t)l->tamanho, sizeof(Contagem));
    if (v == NULL) {
        return NULL;
    }

    for (p = l->cabeca; p != NULL; p = p->proximo) {
        for (i = 0; i < k; i++) {
            if (comparar_ci(v[i].categoria, p->dado.categoria) == 0) {
                break;
            }
        }
        if (i == k) {
            snprintf(v[k].categoria, sizeof v[k].categoria, "%s",
                     p->dado.categoria);
            k++;
        }
        v[i].quantidade++;
    }
    *quantidade_categorias = k;
    return v;
}

/* ================================================================== */
/* 3) MENU INTERATIVO                                                  */
/* ================================================================== */

/* ------------------------------------------------------------------ */
/* Registros de exemplo (opção 8 do menu)                              */
/* ------------------------------------------------------------------ */
static const Servico EXEMPLOS[] = {
    {1, "Primeira via do título de eleitor", "Documentação",
     "Emissão do primeiro título de eleitor.",
     "Documento oficial com foto; comprovante de residência; quitação militar (homens 19 anos)",
     "Cartório Eleitoral / Título Net"},
    {2, "Segunda via de RG", "Documentação",
     "Emissão de nova via da carteira de identidade.",
     "Certidão de nascimento ou casamento; CPF; comprovante de residência",
     "Poupatempo"},
    {3, "Inscrição no CadÚnico", "Assistência Social",
     "Cadastro para programas sociais do governo federal.",
     "CPF ou título de eleitor do responsável familiar; comprovante de residência",
     "CRAS do município"},
    {4, "Cartão Nacional de Saúde (SUS)", "Saúde",
     "Emissão do cartão para atendimento no SUS.",
     "Documento oficial com foto; comprovante de residência",
     "Unidade Básica de Saúde"},
    {5, "Matrícula em curso de qualificação", "Educação",
     "Inscrição em cursos gratuitos de qualificação profissional.",
     "RG; CPF; comprovante de escolaridade",
     "Centro de qualificação municipal"}
};
#define NUM_EXEMPLOS ((int)(sizeof EXEMPLOS / sizeof EXEMPLOS[0]))

/* ------------------------------------------------------------------ */
/* Entrada e saída                                                     */
/* ------------------------------------------------------------------ */

/* Lê uma linha (sem o '\n'). Retorna 0 em fim de arquivo. */
static int ler_linha(const char *prompt, char *buf, size_t tam)
{
    size_t n;

    printf("%s", prompt);
    fflush(stdout);
    if (fgets(buf, (int)tam, stdin) == NULL) {
        buf[0] = '\0';
        return 0;
    }
    n = strlen(buf);
    if (n > 0 && buf[n - 1] == '\n') {
        buf[--n] = '\0';
        if (n > 0 && buf[n - 1] == '\r') {
            buf[n - 1] = '\0';
        }
    } else {
        int c;                          /* linha maior que o buffer: descarta o resto */
        while ((c = getchar()) != '\n' && c != EOF) {
        }
    }
    return 1;
}

/* Lê um inteiro positivo. Retorna 1 se válido; senão avisa e retorna 0. */
static int ler_inteiro(const char *prompt, int *valor)
{
    char linha[32], *fim;
    long n;

    if (!ler_linha(prompt, linha, sizeof linha)) {
        return 0;
    }
    errno = 0;
    n = strtol(linha, &fim, 10);
    if (linha[0] == '\0' || *fim != '\0' || errno == ERANGE ||
        n <= 0 || n > INT_MAX) {
        printf("Entrada inválida: informe um número inteiro positivo.\n");
        return 0;
    }
    *valor = (int)n;
    return 1;
}

static void imprimir_servico(const Servico *s, void *contexto)
{
    (void)contexto;
    printf("[%d] %s\n", s->id, s->titulo);
    printf("    Categoria : %s\n", s->categoria);
    printf("    Descrição : %s\n", s->descricao[0] ? s->descricao : "-");
    printf("    Requisitos: %s\n", s->requisitos[0] ? s->requisitos : "-");
    printf("    Local     : %s\n\n", s->local[0] ? s->local : "-");
}

static void exibir_lista(const Lista *l)
{
    const No *p;

    if (lista_vazia(l)) {
        printf("A lista está vazia. Nenhum registro para exibir.\n");
        return;
    }
    for (p = l->cabeca; p != NULL; p = p->proximo) {
        imprimir_servico(&p->dado, NULL);
    }
    printf("Total: %d registro(s).\n", l->tamanho);
}

/* ------------------------------------------------------------------ */
/* Opções do menu                                                      */
/* ------------------------------------------------------------------ */

static void opcao_cadastrar(Lista *l)
{
    Servico s;
    Resultado r;

    memset(&s, 0, sizeof s);
    if (!ler_inteiro("ID (inteiro positivo): ", &s.id)) {
        return;
    }
    ler_linha("Título: ", s.titulo, sizeof s.titulo);
    ler_linha("Categoria: ", s.categoria, sizeof s.categoria);
    ler_linha("Descrição: ", s.descricao, sizeof s.descricao);
    ler_linha("Requisitos: ", s.requisitos, sizeof s.requisitos);
    ler_linha("Local: ", s.local, sizeof s.local);

    r = lista_inserir_fim(l, &s);
    switch (r) {
    case LISTA_OK:
        printf("Registro cadastrado com sucesso.\n");
        break;
    case LISTA_ERRO_DUPLICADO:
        printf("Erro: Já existe um registro com o ID %d.\n", s.id);
        break;
    case LISTA_ERRO_MEMORIA:
        printf("Erro: memória insuficiente.\n");
        break;
    default:
        printf("Erro: título e categoria são obrigatórios.\n");
        break;
    }
}

static int checar_lista_nao_vazia(const Lista *l, const char *mensagem)
{
    if (lista_vazia(l)) {
        printf("%s\n", mensagem);
        return 0;
    }
    return 1;
}

static void opcao_consultar(Lista *l)
{
    int id;
    Servico *s;

    if (!checar_lista_nao_vazia(l, "A lista está vazia. Cadastre registros primeiro.") ||
        !ler_inteiro("ID a consultar: ", &id)) {
        return;
    }
    s = lista_buscar_id(l, id);
    if (s == NULL) {
        printf("Nenhum registro encontrado com o ID %d.\n", id);
    } else {
        imprimir_servico(s, NULL);
    }
}

static void opcao_pesquisar(Lista *l)
{
    char escolha[8], termo[TAM_TITULO];
    int achados;
    CampoBusca campo;

    if (!checar_lista_nao_vazia(l, "A lista está vazia. Cadastre registros primeiro.")) {
        return;
    }
    ler_linha("Pesquisar por (1) título ou (2) categoria: ", escolha, sizeof escolha);
    if (strcmp(escolha, "1") != 0 && strcmp(escolha, "2") != 0) {
        printf("Opção inválida.\n");
        return;
    }
    campo = (escolha[0] == '1') ? CAMPO_TITULO : CAMPO_CATEGORIA;
    ler_linha("Termo: ", termo, sizeof termo);

    achados = lista_pesquisar(l, campo, termo, imprimir_servico, NULL);
    if (achados < 0) {
        printf("Erro: o termo de busca não pode ser vazio.\n");
    } else if (achados == 0) {
        printf("Nenhum resultado para '%s'.\n", termo);
    } else {
        printf("%d resultado(s).\n", achados);
    }
}

static void opcao_atualizar(Lista *l)
{
    int id;
    Servico *atual;
    char titulo[TAM_TITULO], categoria[TAM_CATEGORIA], descricao[TAM_DESCRICAO],
         requisitos[TAM_REQUISITOS], local[TAM_LOCAL];

    if (!checar_lista_nao_vazia(l, "A lista está vazia. Cadastre registros primeiro.") ||
        !ler_inteiro("ID a atualizar: ", &id)) {
        return;
    }
    atual = lista_buscar_id(l, id);
    if (atual == NULL) {
        printf("Nenhum registro encontrado com o ID %d.\n", id);
        return;
    }
    imprimir_servico(atual, NULL);
    printf("(Enter mantém o valor atual)\n");
    ler_linha("Novo título: ", titulo, sizeof titulo);
    ler_linha("Nova categoria: ", categoria, sizeof categoria);
    ler_linha("Nova descrição: ", descricao, sizeof descricao);
    ler_linha("Novos requisitos: ", requisitos, sizeof requisitos);
    ler_linha("Novo local: ", local, sizeof local);

    lista_atualizar(l, id, titulo, categoria, descricao, requisitos, local);
    printf("Registro atualizado.\n");
}

static void opcao_excluir(Lista *l)
{
    int id;

    if (!checar_lista_nao_vazia(l, "A lista está vazia. Nada para excluir.") ||
        !ler_inteiro("ID a excluir: ", &id)) {
        return;
    }
    if (lista_remover(l, id) == LISTA_OK) {
        printf("Registro excluído.\n");
    } else {
        printf("Nenhum registro encontrado com o ID %d.\n", id);
    }
}

static void opcao_ordenar(Lista *l)
{
    char escolha[8], ordem[8];
    ChaveOrdenacao chave;
    const char *nome;
    int reverso;

    if (!checar_lista_nao_vazia(l, "A lista está vazia. Nada para ordenar.")) {
        return;
    }
    printf("Critério: 1) ID  2) Título  3) Categoria  4) Local\n");
    ler_linha("Escolha: ", escolha, sizeof escolha);
    if (strcmp(escolha, "1") == 0)      { chave = CHAVE_ID;        nome = "id"; }
    else if (strcmp(escolha, "2") == 0) { chave = CHAVE_TITULO;    nome = "título"; }
    else if (strcmp(escolha, "3") == 0) { chave = CHAVE_CATEGORIA; nome = "categoria"; }
    else if (strcmp(escolha, "4") == 0) { chave = CHAVE_LOCAL;     nome = "local"; }
    else {
        printf("Opção inválida.\n");
        return;
    }
    ler_linha("Ordem decrescente? (s/N): ", ordem, sizeof ordem);
    reverso = (ordem[0] == 's' || ordem[0] == 'S');

    lista_ordenar(l, chave, reverso);
    printf("Lista ordenada por %s (%s) com Merge Sort:\n\n", nome,
           reverso ? "decrescente" : "crescente");
    exibir_lista(l);
}

static void opcao_analise(const Lista *l)
{
    Contagem *v;
    int k, i;

    if (!checar_lista_nao_vazia(l, "A lista está vazia. Sem dados para analisar.")) {
        return;
    }
    printf("Total de registros: %d\n", l->tamanho);
    v = lista_contar_por_categoria(l, &k);
    if (v == NULL) {
        printf("Erro: memória insuficiente para a análise.\n");
        return;
    }
    printf("Quantidade por categoria:\n");
    for (i = 0; i < k; i++) {
        printf("  - %s: %d\n", v[i].categoria, v[i].quantidade);
    }
    free(v);
}

static void opcao_exemplos(Lista *l)
{
    int i, inseridos = 0;

    for (i = 0; i < NUM_EXEMPLOS; i++) {
        if (lista_inserir_fim(l, &EXEMPLOS[i]) == LISTA_OK) {
            inseridos++;
        }
    }
    printf("%d exemplo(s) carregado(s).\n", inseridos);
}

static void exibir_menu(void)
{
    printf("\n=== CITIZEN ASSIST AI ===\n");
    printf("1 - Cadastrar serviço\n");
    printf("2 - Consultar por ID\n");
    printf("3 - Pesquisar (título/categoria)\n");
    printf("4 - Atualizar serviço\n");
    printf("5 - Excluir serviço\n");
    printf("6 - Ordenar e listar\n");
    printf("7 - Análise (indicadores)\n");
    printf("8 - Carregar exemplos\n");
    printf("9 - Listar todos\n");
    printf("0 - Encerrar\n");
}

int main(void)
{
    Lista lista;
    char opcao[16];

#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);   /* saída em UTF-8 */
    SetConsoleCP(CP_UTF8);         /* entrada em UTF-8 */
#endif

    lista_iniciar(&lista);

    for (;;) {
        exibir_menu();
        if (!ler_linha("Opção: ", opcao, sizeof opcao)) {
            printf("\nEncerrando.\n");
            break;
        }
        if (strcmp(opcao, "0") == 0) {
            printf("Encerrando o Citizen Assist AI. Até logo!\n");
            break;
        } else if (strcmp(opcao, "1") == 0) { opcao_cadastrar(&lista);
        } else if (strcmp(opcao, "2") == 0) { opcao_consultar(&lista);
        } else if (strcmp(opcao, "3") == 0) { opcao_pesquisar(&lista);
        } else if (strcmp(opcao, "4") == 0) { opcao_atualizar(&lista);
        } else if (strcmp(opcao, "5") == 0) { opcao_excluir(&lista);
        } else if (strcmp(opcao, "6") == 0) { opcao_ordenar(&lista);
        } else if (strcmp(opcao, "7") == 0) { opcao_analise(&lista);
        } else if (strcmp(opcao, "8") == 0) { opcao_exemplos(&lista);
        } else if (strcmp(opcao, "9") == 0) { exibir_lista(&lista);
        } else {
            printf("Opção inválida. Escolha um número de 0 a 9.\n");
        }
    }

    lista_liberar(&lista);   /* libera todos os nós antes de sair */
    return 0;
}