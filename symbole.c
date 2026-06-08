#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "symboles.h"

Symbole *table[TAILLE_TABLE];

unsigned int hacher(const char *nom) {
    unsigned int h = 5381;
    while (*nom) h = ((h << 5) + h) ^ (unsigned char)*nom++;
    return h % TAILLE_TABLE;
}

Symbole *chercher(const char *nom) {
    unsigned int h = hacher(nom);
    for (Symbole *s = table[h]; s; s = s->suivant)
        if (strcmp(s->nom, nom) == 0) return s;
    return NULL;
}

Symbole *inserer(const char *nom, Categorie cat, Type type, int ligne) {
    if (chercher(nom)) {
        return NULL;
    }

    Symbole *s = (Symbole *)malloc(sizeof(Symbole));
    if (!s) {
        perror("Malloc failed");
        exit(1);
    }

    strncpy(s->nom, nom, 8);
    s->nom[8] = '\0';

    s->categorie   = cat;
    s->type        = type;
    s->val_entiere = 0;
    s->val_reelle  = 0.0f;
    s->taille      = 0;
    s->ligne_decl  = ligne;

    unsigned int h = hacher(nom);
    s->suivant = table[h];
    table[h]   = s;

    return s;
}

void afficher_table() {
    printf("\n=========================================================\n");
    printf("                  TABLE DES SYMBOLES\n");
    printf("=========================================================\n");
    printf("%-10s %-12s %-10s %-8s %-10s\n", "Nom", "Categorie", "Type", "Ligne", "Details");
    printf("---------------------------------------------------------\n");
    for (int i = 0; i < TAILLE_TABLE; i++) {
        for (Symbole *s = table[i]; s; s = s->suivant) {
            const char *cat = (s->categorie == VAR) ? "VAR" :
                               (s->categorie == CONSTANTE ? "CONST" : "TABLEAU");
            const char *type = (s->type == TYPE_INTEGER) ? "INTEGER" :
                                (s->type == TYPE_FLOAT ? "FLOAT" : "?");
            printf("%-10s %-12s %-10s %-8d", s->nom, cat, type, s->ligne_decl);
            if (s->categorie == TABLEAU) printf(" Taille=%d", s->taille);
            else if (s->categorie == CONSTANTE) {
                if (s->type == TYPE_INTEGER) printf(" Val=%d", s->val_entiere);
                else printf(" Val=%.2f", s->val_reelle);
            }
            printf("\n");
        }
    }
    printf("=========================================================\n");
}
