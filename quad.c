#include "quad.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Quad quads[1000];
int  qc         = 0;
int  temp_count = 0;

char *generer_temp()
{
    char *temp = malloc(20);
    sprintf(temp, "T%d", temp_count++);
    return temp;
}

void ajouter_quad(const char *op, const char *a1, const char *a2, const char *res)
{
    strncpy(quads[qc].op,   op,  sizeof(quads[qc].op)   - 1);
    strncpy(quads[qc].arg1, a1,  sizeof(quads[qc].arg1) - 1);
    strncpy(quads[qc].arg2, a2,  sizeof(quads[qc].arg2) - 1);
    strncpy(quads[qc].res,  res, sizeof(quads[qc].res)  - 1);
    qc++;
}

Node *makelist(int i)
{
    Node *n  = malloc(sizeof(Node));
    n->index = i;
    n->next  = NULL;
    return n;
}

Node *merge(Node *l1, Node *l2)
{
    if (!l1) return l2;
    if (!l2) return l1;
    Node *tmp = l1;
    while (tmp->next) tmp = tmp->next;
    tmp->next = l2;
    return l1;
}

void backpatch(Node *list, int addr)
{
    char buf[20];
    sprintf(buf, "%d", addr);
    for (Node *p = list; p; p = p->next)
        strncpy(quads[p->index].arg1, buf, sizeof(quads[p->index].arg1) - 1);
}

void afficher_quads()
{
    printf("\n===== QUADRUPLETS =====\n");
    for (int i = 0; i < qc; i++)
        printf("%3d : ( %-6s , %-12s , %-12s , %s )\n",
               i,
               quads[i].op,
               quads[i].arg1,
               quads[i].arg2,
               quads[i].res);
    
    printf("%3d : ( %-6s , %-12s , %-12s , %s )\n",
           qc, "FIN", "", "", "");
}
