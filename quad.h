#ifndef QUAD_H
#define QUAD_H

typedef struct {
    char op[20];
    char arg1[50];
    char arg2[50];
    char res[50];
} Quad;

typedef struct Node {
    int index;
    struct Node *next;
} Node;

extern Quad quads[1000];
extern int qc;

void   ajouter_quad(const char *op, const char *a1, const char *a2, const char *res);
Node  *makelist(int i);
Node  *merge(Node *l1, Node *l2);
void   backpatch(Node *list, int addr);
char  *generer_temp();
void   afficher_quads();

#endif
