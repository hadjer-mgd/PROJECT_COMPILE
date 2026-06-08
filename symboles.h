#ifndef SYMBOLES_H
#define SYMBOLES_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define TAILLE_TABLE 101

typedef enum { TYPE_INTEGER, TYPE_FLOAT, TYPE_INCONNU } Type;
typedef enum { VAR, CONSTANTE, TABLEAU } Categorie;

typedef struct Symbole {
    char           nom[9];         
    Categorie      categorie;      
    Type           type;            
    int            val_entiere;     
    float          val_reelle;      
    int            taille;          
    int            ligne_decl;      
    struct Symbole *suivant;        
} Symbole;

extern int nb_ligne;
extern int nb_colonne;
extern Symbole *table[TAILLE_TABLE];

unsigned int hacher(const char *nom);
Symbole *chercher(const char *nom);
Symbole *inserer(const char *nom, Categorie cat, Type type, int ligne);
void afficher_table();

#endif
