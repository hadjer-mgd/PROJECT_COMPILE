%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "symboles.h"
#include "quad.h"
#include "optim.h"

typedef struct Node Node;

extern char *yytext;
extern int nb_ligne;
extern int nb_colonne;

Type type_courant = TYPE_INCONNU;
int  nb_erreurs   = 0;

#define FOR_MAX 20
static int  for_br_apres_idx[FOR_MAX];
static int  for_debut_test[FOR_MAX];
static char for_var_nom[FOR_MAX][9];
static char for_fin_place[FOR_MAX][50];
static char for_pas_place[FOR_MAX][50];
static int  for_niveau = 0;

void yyerror(const char *msg);
int  yylex(void);
extern FILE *yyin;
%}

%union {
    int   entier;
    float reel;
    char  idf[9];

    struct {
        char  place[50];
        Type  type;
        int   isConst;
        float val;
        int   hasDiv;
        int   quad;
        Node *truelist;
        Node *falselist;
    } expr_attr;
}

%token PROGRAM DECL ENDDECL BEGIN_TOK END
%token IF ELSE FOR WHILE
%token INTEGER FLOAT CONST
%token SEMI COLON COMMA
%token LPAREN RPAREN
%token LBRACE RBRACE
%token LBRACKET RBRACKET
%token ASSIGN
%token PLUS MINUS MULT DIV
%token AND OR NOT
%token GE LE EQ NEQ GT LT

%token <idf>    IDF
%token <entier> CST_ENT
%token <reel>   CST_REEL

%type <expr_attr> expr
%type <expr_attr> M_then M_else M_while M_while2 M_for_corps2

%left  OR
%left  AND
%right NOT
%left  LT GT LE GE EQ NEQ
%left  PLUS MINUS
%left  MULT DIV
%right UMINUS

%%

programme
    : PROGRAM IDF DECL declarations ENDDECL BEGIN_TOK instructions END
    {
printf("\nCompilation terminee.\n");
        printf("Nombre d'erreurs : %d\n", nb_erreurs);
        afficher_table();

        if (nb_erreurs == 0) {
            printf("\n===== QUADS AVANT OPTIMISATION =====\n");
            afficher_quads();
            optimiser();
            printf("\n===== QUADS APRES OPTIMISATION =====\n");
            afficher_quads();
        }
    }
    | PROGRAM error
    {
        fprintf(stderr, "Erreur Syntaxique : structure programme invalide\n");
        nb_erreurs++;
        yyerrok;
    }
;

declarations
    : declarations declaration
    |
;

declaration
    : decl_variable
    | decl_tableau
    | decl_constante
    | error SEMI
    {
        fprintf(stderr, "Erreur Syntaxique ligne %d : declaration invalide\n", nb_ligne);
        nb_erreurs++;
        yyerrok;
    }
;

type
    : INTEGER { type_courant = TYPE_INTEGER; }
    | FLOAT   { type_courant = TYPE_FLOAT;   }
;

decl_variable
    : type COLON liste_idf SEMI
    | type COLON liste_idf
    {
        fprintf(stderr, "Erreur Syntaxique ligne %d : point-virgule manquant\n", nb_ligne);
        nb_erreurs++;
        yyerrok;
    }
;

liste_idf
    : liste_idf COMMA IDF
    {
        if (chercher($3)) {
            fprintf(stderr, "Erreur Semantique ligne %d : double declaration %s\n", nb_ligne, $3);
            nb_erreurs++;
        } else {
            inserer($3, VAR, type_courant, nb_ligne);
        }
    }
    | IDF
    {
        if (chercher($1)) {
            fprintf(stderr, "Erreur Semantique ligne %d : double declaration %s\n", nb_ligne, $1);
            nb_erreurs++;
        } else {
            inserer($1, VAR, type_courant, nb_ligne);
        }
    }
;

decl_tableau
    : type COLON IDF LBRACKET CST_ENT RBRACKET SEMI
    {
        if ($5 <= 0) {
            fprintf(stderr, "Erreur Semantique ligne %d : taille tableau invalide\n", nb_ligne);
            nb_erreurs++;
        } else if (chercher($3)) {
            fprintf(stderr, "Erreur Semantique ligne %d : double declaration %s\n", nb_ligne, $3);
            nb_erreurs++;
        } else {
            Symbole *s = inserer($3, TABLEAU, type_courant, nb_ligne);
            if (s) s->taille = $5;
        }
    }
    | type COLON IDF LBRACKET CST_ENT RBRACKET error
    {
        fprintf(stderr, "Erreur Syntaxique ligne %d : point-virgule manquant (tableau)\n", nb_ligne);
        nb_erreurs++;
        yyerrok;
    }
;

decl_constante
    : CONST COLON IDF ASSIGN CST_ENT SEMI
    {
        if (chercher($3)) {
            fprintf(stderr, "Erreur Semantique ligne %d : double declaration %s\n", nb_ligne, $3);
            nb_erreurs++;
        } else {
            Symbole *s = inserer($3, CONSTANTE, TYPE_INTEGER, nb_ligne);
            if (s) s->val_entiere = $5;
        }
    }
    | CONST COLON IDF ASSIGN CST_REEL SEMI
    {
        if (chercher($3)) {
            fprintf(stderr, "Erreur Semantique ligne %d : double declaration %s\n", nb_ligne, $3);
            nb_erreurs++;
        } else {
            Symbole *s = inserer($3, CONSTANTE, TYPE_FLOAT, nb_ligne);
            if (s) s->val_reelle = $5;
        }
    }
    | CONST COLON IDF ASSIGN CST_ENT error
    {
        fprintf(stderr, "Erreur Syntaxique ligne %d : point-virgule manquant (constante)\n", nb_ligne);
        nb_erreurs++;
        yyerrok;
    }
    | CONST COLON IDF ASSIGN CST_REEL error
    {
        fprintf(stderr, "Erreur Syntaxique ligne %d : point-virgule manquant (constante)\n", nb_ligne);
        nb_erreurs++;
        yyerrok;
    }
;

instructions
    : instructions instruction
    |
;

instruction
    : affectation
    | condition
    | boucle_while
    | boucle_for
    | error SEMI
    {
        fprintf(stderr, "Erreur Syntaxique ligne %d : instruction invalide\n", nb_ligne);
        nb_erreurs++;
        yyerrok;
    }
;

affectation
    : IDF ASSIGN expr SEMI
    {
        Symbole *s = chercher($1);
        if (!s) {
            fprintf(stderr, "Erreur Semantique ligne %d : variable non declaree %s\n", nb_ligne, $1);
            nb_erreurs++;
        } else if (s->categorie == CONSTANTE) {
            fprintf(stderr, "Erreur Semantique ligne %d : modification constante %s\n", nb_ligne, $1);
            nb_erreurs++;
        } else {
            if ($3.type != TYPE_INCONNU && s->type != $3.type) {
                fprintf(stderr, "Erreur Semantique ligne %d : incompatibilite de type\n", nb_ligne);
                nb_erreurs++;
            }
            if (s->type == TYPE_INTEGER && $3.hasDiv) {
                fprintf(stderr, "Erreur Semantique ligne %d : division peut produire float\n", nb_ligne);
                nb_erreurs++;
            }
            ajouter_quad("=", $3.place, "", $1);
        }
    }
    | IDF LBRACKET expr RBRACKET ASSIGN expr SEMI
    {
        Symbole *s = chercher($1);
        if (!s) {
            fprintf(stderr, "Erreur Semantique ligne %d : tableau non declare %s\n", nb_ligne, $1);
            nb_erreurs++;
        } else if (s->categorie != TABLEAU) {
            fprintf(stderr, "Erreur Semantique ligne %d : %s n'est pas un tableau\n", nb_ligne, $1);
            nb_erreurs++;
        } else {
            if ($3.type != TYPE_INTEGER) {
                fprintf(stderr, "Erreur Semantique ligne %d : indice non entier\n", nb_ligne);
                nb_erreurs++;
            }
            if ($3.isConst && ($3.val < 0 || $3.val >= s->taille)) {
                fprintf(stderr, "Erreur Semantique ligne %d : indice hors limites\n", nb_ligne);
                nb_erreurs++;
            }
            char t[50];
            sprintf(t, "%s[%s]", $1, $3.place);
            ajouter_quad("[]=", $6.place, "", t);
        }
    }
    | IDF ASSIGN expr error
    {
        fprintf(stderr, "Erreur Syntaxique ligne %d : point-virgule manquant (affectation)\n", nb_ligne);
        nb_erreurs++;
        yyerrok;
    }
    | IDF LBRACKET expr RBRACKET ASSIGN expr error
    {
        fprintf(stderr, "Erreur Syntaxique ligne %d : point-virgule manquant (affectation tableau)\n", nb_ligne);
        nb_erreurs++;
        yyerrok;
    }
;

M_then
    : { $$.quad = qc; }
;

M_else
    : {
        ajouter_quad("BR", "?", "", "");
        $$.quad = qc;
    }
;

condition
    : IF LPAREN expr RPAREN M_then LBRACE instructions RBRACE
    {
        if ($3.type != TYPE_INTEGER) {
            fprintf(stderr, "Erreur Semantique ligne %d : condition non booleenne\n", nb_ligne);
            nb_erreurs++;
        }
        backpatch($3.truelist,  $5.quad);
        backpatch($3.falselist, qc);
    }
    | IF LPAREN expr RPAREN M_then LBRACE instructions RBRACE M_else ELSE LBRACE instructions RBRACE
    {
        if ($3.type != TYPE_INTEGER) {
            fprintf(stderr, "Erreur Semantique ligne %d : condition non booleenne\n", nb_ligne);
            nb_erreurs++;
        }
        backpatch($3.truelist,           $5.quad);
        backpatch($3.falselist,          $9.quad);
        backpatch(makelist($9.quad - 1), qc);
    }
;

M_while
    : { $$.quad = qc; }
;

M_while2
    : { $$.quad = qc; }
;

boucle_while
    : WHILE M_while LPAREN expr M_while2 RPAREN LBRACE instructions RBRACE
    {
        if ($4.type != TYPE_INTEGER) {
            fprintf(stderr, "Erreur Semantique ligne %d : condition WHILE non booleenne\n", nb_ligne);
            nb_erreurs++;
        }
        backpatch($4.truelist, $5.quad);
        ajouter_quad("BR", "?", "", "");
        sprintf(quads[qc - 1].arg1, "%d", $2.quad);
        backpatch($4.falselist, qc);
    }
;

M_for_corps2
    : {
        int n = for_niveau - 1;
        for_debut_test[n] = qc;
        ajouter_quad("BLE", "?", for_var_nom[n], for_fin_place[n]);
        int ble_idx = qc - 1;
        ajouter_quad("BR", "?", "", "");
        for_br_apres_idx[n] = qc - 1;
        $$.quad = qc;                              
        sprintf(quads[ble_idx].arg1, "%d", qc);   
    }
;

boucle_for
    : FOR LPAREN IDF COLON expr COLON expr COLON expr RPAREN
    {
        int n = for_niveau++;
        strncpy(for_var_nom[n],   $3,        8);  for_var_nom[n][8]    = '\0';
        strncpy(for_pas_place[n], $7.place, 49);  for_pas_place[n][49] = '\0';
        strncpy(for_fin_place[n], $9.place, 49);  for_fin_place[n][49] = '\0';
        ajouter_quad("=", $5.place, "", $3);

    }
    M_for_corps2 LBRACE instructions RBRACE
    {
        int n = for_niveau - 1;
        Symbole *s = chercher($3);
        if (!s) {
            fprintf(stderr, "Erreur Semantique ligne %d : variable FOR non declaree\n", nb_ligne);
            nb_erreurs++;
        } else if (s->type != TYPE_INTEGER) {
            fprintf(stderr, "Erreur Semantique ligne %d : variable FOR doit etre INTEGER\n", nb_ligne);
            nb_erreurs++;
        }
        ajouter_quad("+", for_var_nom[n], for_pas_place[n], for_var_nom[n]);
        ajouter_quad("BR", "?", "", "");
        sprintf(quads[qc - 1].arg1, "%d", for_debut_test[n]);
        sprintf(quads[for_br_apres_idx[n]].arg1, "%d", qc);
        for_niveau--;
    }
;

expr
    : expr PLUS expr
    {
        $$.quad = qc;
        char *t = generer_temp();
        ajouter_quad("+", $1.place, $3.place, t);
        strcpy($$.place, t);
        $$.type    = ($1.type == TYPE_FLOAT || $3.type == TYPE_FLOAT) ? TYPE_FLOAT : TYPE_INTEGER;
        $$.hasDiv  = $1.hasDiv || $3.hasDiv;
        $$.isConst = $1.isConst && $3.isConst;
        if ($$.isConst) $$.val = $1.val + $3.val;
        $$.truelist = NULL; $$.falselist = NULL;
    }
    | expr MINUS expr
    {
        $$.quad = qc;
        char *t = generer_temp();
        ajouter_quad("-", $1.place, $3.place, t);
        strcpy($$.place, t);
        $$.type    = ($1.type == TYPE_FLOAT || $3.type == TYPE_FLOAT) ? TYPE_FLOAT : TYPE_INTEGER;
        $$.hasDiv  = $1.hasDiv || $3.hasDiv;
        $$.isConst = $1.isConst && $3.isConst;
        if ($$.isConst) $$.val = $1.val - $3.val;
        $$.truelist = NULL; $$.falselist = NULL;
    }
    | expr MULT expr
    {
        $$.quad = qc;
        char *t = generer_temp();
        ajouter_quad("*", $1.place, $3.place, t);
        strcpy($$.place, t);
        $$.type    = ($1.type == TYPE_FLOAT || $3.type == TYPE_FLOAT) ? TYPE_FLOAT : TYPE_INTEGER;
        $$.hasDiv  = $1.hasDiv || $3.hasDiv;
        $$.isConst = $1.isConst && $3.isConst;
        if ($$.isConst) $$.val = $1.val * $3.val;
        $$.truelist = NULL; $$.falselist = NULL;
    }
    | expr DIV expr
    {
        if ($3.isConst && $3.val == 0) {
            fprintf(stderr, "Erreur Semantique ligne %d : division par zero\n", nb_ligne);
            nb_erreurs++;
        }
        $$.quad = qc;
        char *t = generer_temp();
        ajouter_quad("/", $1.place, $3.place, t);
        strcpy($$.place, t);
        $$.type    = TYPE_FLOAT;
        $$.hasDiv  = 1;
        $$.isConst = $1.isConst && $3.isConst;
        if ($$.isConst && $3.val != 0) $$.val = $1.val / $3.val;
        $$.truelist = NULL; $$.falselist = NULL;
    }
    | expr GT expr
    {
        if ($1.type != TYPE_INCONNU && $3.type != TYPE_INCONNU && $1.type != $3.type) {
            fprintf(stderr, "Erreur Semantique ligne %d : incompatibilite comparaison\n", nb_ligne);
            nb_erreurs++;
        }
        $$.quad = qc;
        int q_true  = qc;
        ajouter_quad("BG",  "?", $1.place, $3.place);
        int q_false = qc;
        ajouter_quad("BR",  "?", "", "");
        $$.truelist  = makelist(q_true);
        $$.falselist = makelist(q_false);
        $$.type = TYPE_INTEGER; $$.hasDiv = 0; $$.isConst = 0;
    }
    | expr LT expr
    {
        if ($1.type != TYPE_INCONNU && $3.type != TYPE_INCONNU && $1.type != $3.type) {
            fprintf(stderr, "Erreur Semantique ligne %d : incompatibilite comparaison\n", nb_ligne);
            nb_erreurs++;
        }
        $$.quad = qc;
        int q_true  = qc;
        ajouter_quad("BL",  "?", $1.place, $3.place);
        int q_false = qc;
        ajouter_quad("BR",  "?", "", "");
        $$.truelist  = makelist(q_true);
        $$.falselist = makelist(q_false);
        $$.type = TYPE_INTEGER; $$.hasDiv = 0; $$.isConst = 0;
    }
    | expr GE expr
    {
        if ($1.type != TYPE_INCONNU && $3.type != TYPE_INCONNU && $1.type != $3.type) {
            fprintf(stderr, "Erreur Semantique ligne %d : incompatibilite comparaison\n", nb_ligne);
            nb_erreurs++;
        }
        $$.quad = qc;
        int q_true  = qc;
        ajouter_quad("BGE", "?", $1.place, $3.place);
        int q_false = qc;
        ajouter_quad("BR",  "?", "", "");
        $$.truelist  = makelist(q_true);
        $$.falselist = makelist(q_false);
        $$.type = TYPE_INTEGER; $$.hasDiv = 0; $$.isConst = 0;
    }
    | expr LE expr
    {
        if ($1.type != TYPE_INCONNU && $3.type != TYPE_INCONNU && $1.type != $3.type) {
            fprintf(stderr, "Erreur Semantique ligne %d : incompatibilite comparaison\n", nb_ligne);
            nb_erreurs++;
        }
        $$.quad = qc;
        int q_true  = qc;
        ajouter_quad("BLE", "?", $1.place, $3.place);
        int q_false = qc;
        ajouter_quad("BR",  "?", "", "");
        $$.truelist  = makelist(q_true);
        $$.falselist = makelist(q_false);
        $$.type = TYPE_INTEGER; $$.hasDiv = 0; $$.isConst = 0;
    }
    | expr EQ expr
    {
        if ($1.type != TYPE_INCONNU && $3.type != TYPE_INCONNU && $1.type != $3.type) {
            fprintf(stderr, "Erreur Semantique ligne %d : incompatibilite comparaison\n", nb_ligne);
            nb_erreurs++;
        }
        $$.quad = qc;
        int q_true  = qc;
        ajouter_quad("BEQ", "?", $1.place, $3.place);
        int q_false = qc;
        ajouter_quad("BR",  "?", "", "");
        $$.truelist  = makelist(q_true);
        $$.falselist = makelist(q_false);
        $$.type = TYPE_INTEGER; $$.hasDiv = 0; $$.isConst = 0;
    }
    | expr NEQ expr
    {
        if ($1.type != TYPE_INCONNU && $3.type != TYPE_INCONNU && $1.type != $3.type) {
            fprintf(stderr, "Erreur Semantique ligne %d : incompatibilite comparaison\n", nb_ligne);
            nb_erreurs++;
        }
        $$.quad = qc;
        int q_true  = qc;
        ajouter_quad("BNEQ", "?", $1.place, $3.place);
        int q_false = qc;
        ajouter_quad("BR",   "?", "", "");
        $$.truelist  = makelist(q_true);
        $$.falselist = makelist(q_false);
        $$.type = TYPE_INTEGER; $$.hasDiv = 0; $$.isConst = 0;
    }
    | expr AND expr
    {
        $$.quad = qc;
        backpatch($1.truelist, $3.quad);
        $$.truelist  = $3.truelist;
        $$.falselist = merge($1.falselist, $3.falselist);
        $$.type = TYPE_INTEGER; $$.hasDiv = 0; $$.isConst = 0;
        strcpy($$.place, "");
    }
    | expr OR expr
    {
        $$.quad = qc;
        backpatch($1.falselist, $3.quad);
        $$.truelist  = merge($1.truelist, $3.truelist);
        $$.falselist = $3.falselist;
        $$.type = TYPE_INTEGER; $$.hasDiv = 0; $$.isConst = 0;
        strcpy($$.place, "");
    }
    | NOT expr
    {
        $$.quad      = qc;
        $$.truelist  = $2.falselist;
        $$.falselist = $2.truelist;
        $$.type = TYPE_INTEGER; $$.hasDiv = 0; $$.isConst = 0;
        strcpy($$.place, $2.place);
    }
    | MINUS expr %prec UMINUS
    {
        $$.quad = qc;
        char *t = generer_temp();
        ajouter_quad("UMINUS", $2.place, "", t);
        strcpy($$.place, t);
        $$.type = $2.type; $$.hasDiv = $2.hasDiv; $$.isConst = 0;
        $$.truelist = NULL; $$.falselist = NULL;
    }
    | LPAREN expr RPAREN { $$ = $2; }
    | CST_ENT
    {
        $$.quad = qc; $$.type = TYPE_INTEGER;
        $$.isConst = 1; $$.val = $1; $$.hasDiv = 0;
        $$.truelist = NULL; $$.falselist = NULL;
        sprintf($$.place, "%d", $1);
    }
    | CST_REEL
    {
        $$.quad = qc; $$.type = TYPE_FLOAT;
        $$.isConst = 1; $$.val = $1; $$.hasDiv = 0;
        $$.truelist = NULL; $$.falselist = NULL;
        sprintf($$.place, "%f", $1);
    }
    | IDF
    {
        $$.quad = qc;
        Symbole *s = chercher($1);
        if (!s) {
            fprintf(stderr, "Erreur Semantique ligne %d : variable non declaree %s\n", nb_ligne, $1);
            nb_erreurs++;
            $$.type = TYPE_INCONNU;
        } else {
            $$.type = s->type;
        }
        $$.isConst = 0; $$.hasDiv = 0;
        $$.truelist = NULL; $$.falselist = NULL;
        strcpy($$.place, $1);
    }
    | IDF LBRACKET expr RBRACKET
    {
        $$.quad = qc;
        Symbole *s = chercher($1);
        if (!s) {
            fprintf(stderr, "Erreur Semantique ligne %d : tableau non declare %s\n", nb_ligne, $1);
            nb_erreurs++;
            $$.type = TYPE_INCONNU;
        } else {
            if ($3.type != TYPE_INTEGER) {
                fprintf(stderr, "Erreur Semantique ligne %d : indice non entier\n", nb_ligne);
                nb_erreurs++;
            }
            if ($3.isConst && ($3.val < 0 || $3.val >= s->taille)) {
                fprintf(stderr, "Erreur Semantique ligne %d : indice hors limites\n", nb_ligne);
                nb_erreurs++;
            }
            char *t = generer_temp();
            char tab[50];
            sprintf(tab, "%s[%s]", $1, $3.place);
            ajouter_quad("=[]", tab, "", t);
            strcpy($$.place, t);
            $$.type = s->type;
        }
        $$.isConst = 0; $$.hasDiv = 0;
        $$.truelist = NULL; $$.falselist = NULL;
    }
;

%%

void yyerror(const char *msg)
{
    fprintf(stderr, "Erreur Syntaxique ligne %d col %d : %s\n", nb_ligne, nb_colonne, yytext);
    nb_erreurs++;
}

int main(int argc, char *argv[])
{
    memset(table, 0, sizeof(table));
    memset(quads, 0, sizeof(quads));
    if (argc > 1) {
        FILE *f = fopen(argv[1], "r");
        if (!f) { perror(argv[1]); return 1; }
        yyin = f;
    } else {
        printf("Donner un fichier source\n");
        return 1;
    }
    return yyparse();
}
