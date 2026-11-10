/*
 * test_lista.c - testes unitários da lista encadeada.
 * Compilar e executar (precisa estar na mesma pasta do main.c):
 *   gcc -std=c99 -Wall -Wextra -o test_lista test_lista.c
 *   ./test_lista
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* inclui o programa inteiro, renomeando o main() dele para não conflitar */
#define main citizen_assist_main
#include "main.c"
#undef main

static int total = 0, falhas = 0;

#define VERIFICAR(codigo, descricao, condicao)                                \
    do {                                                                      \
        total++;                                                              \
        if (condicao) {                                                       \
            printf("[OK]    %s %s\n", codigo, descricao);                     \
        } else {                                                              \
            falhas++;                                                         \
            printf("[FALHA] %s %s (linha %d)\n", codigo, descricao, __LINE__);\
        }                                                                     \
    } while (0)

/* ---- auxiliares ---------------------------------------------------- */

static Servico novo(int id, const char *titulo, const char *categoria,
                    const char *local)
{
    Servico s;
    memset(&s, 0, sizeof s);
    s.id = id;
    snprintf(s.titulo, sizeof s.titulo, "%s", titulo);
    snprintf(s.categoria, sizeof s.categoria, "%s", categoria);
    snprintf(s.descricao, sizeof s.descricao, "desc");
    snprintf(s.requisitos, sizeof s.requisitos, "req");
    snprintf(s.local, sizeof s.local, "%s", local);
    return s;
}

/* Escreve os IDs na ordem da lista, ex.: "3,1,2". */
static void ids(const Lista *l, char *saida, size_t tam)
{
    size_t pos = 0;
    const No *p;

    saida[0] = '\0';
    for (p = l->cabeca; p != NULL; p = p->proximo) {
        int w = snprintf(saida + pos, tam - pos, "%s%d", pos ? "," : "", p->dado.id);
        if (w < 0 || (size_t)w >= tam - pos) {
            break;
        }
        pos += (size_t)w;
    }
}

static int ids_iguais(const Lista *l, const char *esperado)
{
    char buf[128];
    ids(l, buf, sizeof buf);
    return strcmp(buf, esperado) == 0;
}

/* Lista de teste: IDs 3 (Saúde), 1 e 2 (Documentação). */
static void montar(Lista *l)
{
    Servico a = novo(3, "Cartão SUS", "Saúde", "UBS");
    Servico b = novo(1, "RG segunda via", "Documentação", "Poupatempo");
    Servico c = novo(2, "Título de eleitor", "Documentação", "Cartório");

    lista_iniciar(l);
    lista_inserir_fim(l, &a);
    lista_inserir_fim(l, &b);
    lista_inserir_fim(l, &c);
}

/* Callback de pesquisa: acumula os IDs encontrados. */
typedef struct { char ids[64]; int n; } Coleta;

static void coletar(const Servico *s, void *contexto)
{
    Coleta *c = (Coleta *)contexto;
    size_t pos = strlen(c->ids);
    snprintf(c->ids + pos, sizeof c->ids - pos, "%s%d", c->n ? "," : "", s->id);
    c->n++;
}

/* ---- casos normais ------------------------------------------------- */

static void testes_normais(void)
{
    Lista l;
    Coleta col;
    Servico *s;
    Servico extra;
    Contagem *v;
    int k;

    montar(&l);
    VERIFICAR("N01", "cadastro de 3 registros preserva ordem e tamanho",
              ids_iguais(&l, "3,1,2") && l.tamanho == 3);

    s = lista_buscar_id(&l, 1);
    VERIFICAR("N02", "consulta por ID existente", s != NULL && strcmp(s->titulo, "RG segunda via") == 0);

    memset(&col, 0, sizeof col);
    lista_pesquisar(&l, CAMPO_TITULO, "ELEITOR", coletar, &col);
    VERIFICAR("N03", "pesquisa por título parcial ignorando maiúsculas",
              col.n == 1 && strcmp(col.ids, "2") == 0);

    memset(&col, 0, sizeof col);
    lista_pesquisar(&l, CAMPO_CATEGORIA, "documentação", coletar, &col);
    VERIFICAR("N04", "pesquisa por categoria com vários resultados",
              col.n == 2 && strcmp(col.ids, "1,2") == 0);

    lista_ordenar(&l, CHAVE_ID, 0);
    VERIFICAR("N05", "Merge Sort por ID crescente", ids_iguais(&l, "1,2,3"));

    lista_ordenar(&l, CHAVE_ID, 1);
    VERIFICAR("N06", "Merge Sort por ID decrescente", ids_iguais(&l, "3,2,1"));
    lista_liberar(&l);

    montar(&l);
    lista_ordenar(&l, CHAVE_TITULO, 0);
    VERIFICAR("N07", "Merge Sort por título (alfabético)", ids_iguais(&l, "3,1,2"));
    lista_liberar(&l);

    montar(&l);
    lista_ordenar(&l, CHAVE_CATEGORIA, 0);
    VERIFICAR("N08", "Merge Sort estável (empate por categoria mantém ordem)",
              ids_iguais(&l, "1,2,3"));

    lista_ordenar(&l, CHAVE_ID, 0);
    extra = novo(9, "Extra", "Cat", "L");
    lista_inserir_fim(&l, &extra);
    VERIFICAR("N09", "cauda correta após ordenar (inserção continua válida)",
              ids_iguais(&l, "1,2,3,9") && l.cauda->dado.id == 9);
    lista_liberar(&l);

    montar(&l);
    VERIFICAR("N10", "atualização preserva ID e posição",
              lista_atualizar(&l, 1, "RG novo", NULL, "", "  ", "Poupatempo Osasco") == LISTA_OK &&
              strcmp(lista_buscar_id(&l, 1)->titulo, "RG novo") == 0 &&
              strcmp(lista_buscar_id(&l, 1)->local, "Poupatempo Osasco") == 0 &&
              strcmp(lista_buscar_id(&l, 1)->categoria, "Documentação") == 0 &&
              ids_iguais(&l, "3,1,2"));
    lista_liberar(&l);

    montar(&l);
    VERIFICAR("N11", "remoção da cabeça",
              lista_remover(&l, 3) == LISTA_OK && ids_iguais(&l, "1,2") && l.tamanho == 2);
    lista_liberar(&l);

    montar(&l);
    VERIFICAR("N12", "remoção do meio religa os vizinhos",
              lista_remover(&l, 1) == LISTA_OK && ids_iguais(&l, "3,2"));
    lista_liberar(&l);

    montar(&l);
    lista_remover(&l, 2);
    extra = novo(7, "Novo", "Cat", "L");
    lista_inserir_fim(&l, &extra);
    VERIFICAR("N13", "remoção da cauda e nova inserção",
              ids_iguais(&l, "3,1,7") && l.cauda->dado.id == 7);
    lista_liberar(&l);

    montar(&l);
    v = lista_contar_por_categoria(&l, &k);
    VERIFICAR("N14", "indicador: quantidade por categoria",
              v != NULL && k == 2 &&
              strcmp(v[0].categoria, "Saúde") == 0 && v[0].quantidade == 1 &&
              strcmp(v[1].categoria, "Documentação") == 0 && v[1].quantidade == 2);
    free(v);
    lista_liberar(&l);
}

/* ---- registros inexistentes ---------------------------------------- */

static void testes_inexistentes(void)
{
    Lista l;
    Coleta col;
    int achados;

    montar(&l);
    VERIFICAR("E01", "consulta por ID inexistente retorna NULL",
              lista_buscar_id(&l, 999) == NULL);

    memset(&col, 0, sizeof col);
    achados = lista_pesquisar(&l, CAMPO_TITULO, "passaporte", coletar, &col);
    VERIFICAR("E02", "pesquisa sem resultado retorna 0", achados == 0 && col.n == 0);

    VERIFICAR("E03", "remoção de ID inexistente não altera a lista",
              lista_remover(&l, 999) == LISTA_NAO_ENCONTRADO && ids_iguais(&l, "3,1,2"));

    VERIFICAR("E04", "atualização de ID inexistente",
              lista_atualizar(&l, 999, "x", NULL, NULL, NULL, NULL) == LISTA_NAO_ENCONTRADO);
    lista_liberar(&l);
}

/* ---- entradas inválidas -------------------------------------------- */

static void testes_invalidos(void)
{
    Lista l;
    Servico s;

    lista_iniciar(&l);
    s = novo(1, "Serviço", "Cat", "L");
    lista_inserir_fim(&l, &s);
    s = novo(1, "Outro", "Cat", "L");
    VERIFICAR("I01", "ID duplicado é rejeitado e a lista não muda",
              lista_inserir_fim(&l, &s) == LISTA_ERRO_DUPLICADO && l.tamanho == 1);
    lista_liberar(&l);

    lista_iniciar(&l);
    s = novo(0, "Serviço", "Cat", "L");
    {
        int r0 = lista_inserir_fim(&l, &s);
        int rn;
        s.id = -5;
        rn = lista_inserir_fim(&l, &s);
        VERIFICAR("I02", "ID zero ou negativo é rejeitado",
                  r0 == LISTA_ERRO_INVALIDO && rn == LISTA_ERRO_INVALIDO && l.tamanho == 0);
    }

    s = novo(1, "   ", "Cat", "L");
    {
        int r1 = lista_inserir_fim(&l, &s);
        int r2;
        s = novo(1, "Serviço", "", "L");
        r2 = lista_inserir_fim(&l, &s);
        VERIFICAR("I03", "título ou categoria vazios são rejeitados",
                  r1 == LISTA_ERRO_INVALIDO && r2 == LISTA_ERRO_INVALIDO && l.tamanho == 0);
    }
    lista_liberar(&l);

    montar(&l);
    VERIFICAR("I04", "critério de ordenação inexistente é rejeitado",
              lista_ordenar(&l, (ChaveOrdenacao)99, 0) == LISTA_ERRO_INVALIDO &&
              ids_iguais(&l, "3,1,2"));

    VERIFICAR("I05", "atualização com campos vazios/NULL mantém o registro",
              lista_atualizar(&l, 1, "   ", "", NULL, NULL, NULL) == LISTA_OK &&
              strcmp(lista_buscar_id(&l, 1)->titulo, "RG segunda via") == 0 &&
              strcmp(lista_buscar_id(&l, 1)->categoria, "Documentação") == 0);

    VERIFICAR("I06", "termo de busca vazio ou campo inválido retorna -1",
              lista_pesquisar(&l, CAMPO_TITULO, "   ", NULL, NULL) == -1 &&
              lista_pesquisar(&l, (CampoBusca)7, "rg", NULL, NULL) == -1);

    VERIFICAR("I07", "ponteiros NULL são tratados sem falha",
              lista_inserir_fim(NULL, &s) == LISTA_ERRO_INVALIDO &&
              lista_inserir_fim(&l, NULL) == LISTA_ERRO_INVALIDO &&
              lista_remover(NULL, 1) == LISTA_ERRO_INVALIDO);
    lista_liberar(&l);

    lista_iniciar(&l);
    s = novo(1, "x", "Cat", "L");
    memset(s.titulo, 'a', sizeof s.titulo - 1);   /* título no limite do buffer */
    s.titulo[sizeof s.titulo - 1] = '\0';
    VERIFICAR("I08", "título no tamanho máximo é aceito sem estourar o buffer",
              lista_inserir_fim(&l, &s) == LISTA_OK &&
              strlen(lista_buscar_id(&l, 1)->titulo) == TAM_TITULO - 1);
    lista_liberar(&l);
}

/* ---- lista vazia --------------------------------------------------- */

static void testes_lista_vazia(void)
{
    Lista l;
    Servico s;
    int k = -1;
    Coleta col;

    lista_iniciar(&l);
    memset(&col, 0, sizeof col);
    VERIFICAR("V01", "buscar, pesquisar, remover, ordenar e contar na lista vazia",
              lista_vazia(&l) &&
              lista_buscar_id(&l, 1) == NULL &&
              lista_pesquisar(&l, CAMPO_TITULO, "x", coletar, &col) == 0 &&
              lista_remover(&l, 1) == LISTA_NAO_ENCONTRADO &&
              lista_ordenar(&l, CHAVE_ID, 0) == LISTA_OK &&
              lista_contar_por_categoria(&l, &k) == NULL && k == 0);

    s = novo(1, "Serviço", "Cat", "L");
    lista_inserir_fim(&l, &s);
    lista_remover(&l, 1);
    s = novo(2, "Outro", "Cat", "L");
    VERIFICAR("V02", "remover o único nó esvazia a lista e permite reinserir",
              l.cabeca == NULL && l.cauda == NULL && l.tamanho == 0 &&
              lista_inserir_fim(&l, &s) == LISTA_OK && ids_iguais(&l, "2"));

    lista_liberar(&l);
    VERIFICAR("V03", "lista_liberar deixa a lista vazia e pode ser chamada de novo",
              l.cabeca == NULL && l.cauda == NULL && l.tamanho == 0);
    lista_liberar(&l);
}

int main(void)
{
    testes_normais();
    testes_inexistentes();
    testes_invalidos();
    testes_lista_vazia();

    printf("\n%d teste(s), %d falha(s).\n", total, falhas);
    return falhas == 0 ? 0 : 1;
}