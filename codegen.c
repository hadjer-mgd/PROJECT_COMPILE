/*
 * codegen.c  -  Generation de code assembleur 8086
 *
 * Methode : algorithme GetinACC du cours (Chapitre Generation de code Objet)
 * traduit en instructions 8086 reelles.
 *
 *  Machine abstraite du cours  |  8086 reel genere ici
 *  ----------------------------|-------------------------
 *  LOAD  M                     |  MOV  AX, [M]   (ou MOV AX, valeur)
 *  STORE M                     |  MOV  [M], AX
 *  ADD   M                     |  ADD  AX, [M]
 *  SUB   M                     |  SUB  AX, [M]
 *  MULT  M                     |  IMUL WORD PTR [M]
 *  DIV   M                     |  CWD  +  IDIV WORD PTR [M]
 *  CHS                         |  NEG  AX
 *  B     L                     |  JMP  L
 *  BGT/BLT/BGE/BLE/BEQ/BNE L  |  JG/JL/JGE/JLE/JE/JNE L
 *
 * AX joue le role de l'accumulateur ACC.
 * La variable statique  acc  memorise ce que contient AX (connu
 * a la compilation), conformement a la procedure GetinACC du cours.
 *
 * Licence : code original open-source (MIT)
 */

#include "codegen.h"
#include "quad.h"
#include "symboles.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* TAILLE_TABLE est defini dans symboles.h (valeur 101) */

/* ================================================================
 * 0. Etat global de l'accumulateur (AX = ACC du cours)
 * ================================================================ */

static char acc[50] = "";   /* nom de ce qui est dans AX, "" = vide */
static int  save_count = 0; /* compteur de temporaires de sauvegarde */
static FILE *OUT = NULL;    /* fichier de sortie courant */

/* ================================================================
 * 1. Table des temporaires  Tn  ->  adresse symbolique _Tn
 * ================================================================ */

#define MAX_TEMPS 512
typedef struct { char nom[20]; char addr[20]; } TempEntry;
static TempEntry temp_table[MAX_TEMPS];
static int       nb_temps = 0;

static const char *addr_temp(const char *nom)
{
    for (int i = 0; i < nb_temps; i++)
        if (strcmp(temp_table[i].nom, nom) == 0)
            return temp_table[i].addr;
    strncpy(temp_table[nb_temps].nom,  nom, 19);
    temp_table[nb_temps].nom[19] = '\0';
    snprintf(temp_table[nb_temps].addr, 19, "_%s", nom);
    nb_temps++;
    return temp_table[nb_temps - 1].addr;
}

/* ================================================================
 * 2. Utilitaires
 * ================================================================ */

static int est_temporaire(const char *s)
{
    return s && s[0] == 'T' && isdigit((unsigned char)s[1]);
}

static int est_litteral(const char *s)
{
    if (!s || !*s) return 0;
    char *end;
    strtol(s, &end, 10);
    if (*end == '\0') return 1;
    strtod(s, &end);
    return *end == '\0';
}

/*
 * Retourne l'operande memoire 8086 d'un symbole :
 *   - litterale      ->  la valeur telle quelle  (ex: "5")
 *   - temporaire Tn  ->  "_Tn"
 *   - variable/tab   ->  le nom tel quel
 */
static const char *mem_of(const char *s)
{
    if (est_temporaire(s)) return addr_temp(s);
    return s;
}

/* ================================================================
 * 3. Primitives d'emission 8086
 *    (correspondent aux Gen('LOAD',...) du cours)
 * ================================================================ */

/* LOAD M  ->  MOV AX, [M]  ou  MOV AX, valeur */
static void emit_LOAD(const char *m)
{
    if (est_litteral(m))
        fprintf(OUT, "    MOV  AX, %-10s          ; LOAD %s\n", m, m);
    else
        fprintf(OUT, "    MOV  AX, [%-10s]        ; LOAD %s\n",
                mem_of(m), m);
    strncpy(acc, m, 49); acc[49] = '\0';
}

/* STORE M  ->  MOV [M], AX */
static void emit_STORE(const char *m)
{
    fprintf(OUT, "    MOV  [%-10s], AX        ; STORE %s\n", mem_of(m), m);
}

/* ADD M  ->  ADD AX, [M]  ou  ADD AX, valeur */
static void emit_ADD(const char *m)
{
    if (est_litteral(m))
        fprintf(OUT, "    ADD  AX, %-10s          ; ADD %s\n", m, m);
    else
        fprintf(OUT, "    ADD  AX, [%-10s]        ; ADD %s\n",
                mem_of(m), m);
}

/* SUB M  ->  SUB AX, [M]  ou  SUB AX, valeur */
static void emit_SUB(const char *m)
{
    if (est_litteral(m))
        fprintf(OUT, "    SUB  AX, %-10s          ; SUB %s\n", m, m);
    else
        fprintf(OUT, "    SUB  AX, [%-10s]        ; SUB %s\n",
                mem_of(m), m);
}

/* MULT M  ->  IMUL WORD PTR [M] */
static void emit_MULT(const char *m)
{
    if (est_litteral(m)) {
        fprintf(OUT, "    MOV  BX, %-10s          ; MULT %s (imm)\n", m, m);
        fprintf(OUT, "    IMUL BX\n");
    } else {
        fprintf(OUT, "    IMUL WORD PTR [%-10s]  ; MULT %s\n",
                mem_of(m), m);
    }
}

/* DIV M  ->  CWD  +  IDIV WORD PTR [M] */
static void emit_DIV(const char *m)
{
    fprintf(OUT, "    CWD                              ; extension signe\n");
    if (est_litteral(m)) {
        fprintf(OUT, "    MOV  BX, %-10s          ; DIV %s (imm)\n", m, m);
        fprintf(OUT, "    IDIV BX\n");
    } else {
        fprintf(OUT, "    IDIV WORD PTR [%-10s]  ; DIV %s\n",
                mem_of(m), m);
    }
}

/* CHS  ->  NEG AX */
static void emit_CHS(void)
{
    fprintf(OUT, "    NEG  AX                          ; CHS\n");
}

/* B L  ->  JMP L */
static void emit_B(const char *label)
{
    fprintf(OUT, "    JMP  %-10s               ; B %s\n", label, label);
}

/* Branchement conditionnel apres CMP AX, [m2] */
static void emit_Bcc(const char *op8086, const char *label)
{
    fprintf(OUT, "    %-4s %-10s               ; saut conditionnel\n",
            op8086, label);
}

/* ================================================================
 * 4. Procedure GetinACC (X, Y)  --  conforme au cours
 *
 *    X = 1er operande (QD(i,2))
 *    Y = 2eme operande (QD(i,3)), ou "" si non-commutatif
 *    commutative : 1 pour + et *,  0 pour - et /
 *
 *    Modifie px et py en cas de permutation.
 * ================================================================ */

static void GetinACC(char *px, char *py, int commutative)
{
    /* cas 1 : ACC vide */
    if (acc[0] == '\0') {
        emit_LOAD(px);
        /* acc mis a jour dans emit_LOAD */
        return;
    }

    /* cas 2 : ACC contient deja Y (2eme operande) et op commutative
               -> permutation X <-> Y, rien a generer */
    if (commutative && strcmp(acc, py) == 0) {
        fprintf(OUT, "    ; permutation operandes (%s <-> %s), ACC=%s\n",
                px, py, acc);
        char tmp[50];
        strncpy(tmp, px,  49); tmp[49] = '\0';
        strncpy(px,  py,  49); px[49]  = '\0';
        strncpy(py,  tmp, 49); py[49]  = '\0';
        return;
    }

    /* cas 3 : ACC contient X deja -> rien a faire */
    if (strcmp(acc, px) == 0) {
        return;
    }

    /* cas 4 : ACC contient autre chose -> STORE Z puis LOAD X */
    char save[20];
    snprintf(save, sizeof(save), "_SAVE%d", save_count++);
    emit_STORE(save);        /* STORE Z  (sauvegarde l'ACC actuel) */
    emit_LOAD(px);           /* LOAD X                             */
}

/* ================================================================
 * 5. Generation par operateur arithmetique
 * ================================================================ */

static void gen_add(const char *a1, const char *a2, const char *res)
{
    char X[50], Y[50];
    strncpy(X, a1, 49); X[49]='\0';
    strncpy(Y, a2, 49); Y[49]='\0';

    GetinACC(X, Y, 1);   /* commutatif */
    emit_ADD(Y);
    strncpy(acc, res, 49); acc[49]='\0';
    emit_STORE(res);
}

static void gen_sub(const char *a1, const char *a2, const char *res)
{
    char X[50], vide[2]="";
    strncpy(X, a1, 49); X[49]='\0';

    GetinACC(X, vide, 0);  /* non commutatif : forcer X=a1 */
    emit_SUB(a2);
    strncpy(acc, res, 49); acc[49]='\0';
    emit_STORE(res);
}

static void gen_mult(const char *a1, const char *a2, const char *res)
{
    char X[50], Y[50];
    strncpy(X, a1, 49); X[49]='\0';
    strncpy(Y, a2, 49); Y[49]='\0';

    GetinACC(X, Y, 1);   /* commutatif */
    emit_MULT(Y);
    strncpy(acc, res, 49); acc[49]='\0';
    emit_STORE(res);
}

static void gen_div(const char *a1, const char *a2, const char *res)
{
    char X[50], vide[2]="";
    strncpy(X, a1, 49); X[49]='\0';

    GetinACC(X, vide, 0);  /* non commutatif */
    emit_DIV(a2);
    strncpy(acc, res, 49); acc[49]='\0';
    emit_STORE(res);
}

static void gen_uminus(const char *a1, const char *res)
{
    char X[50], vide[2]="";
    strncpy(X, a1, 49); X[49]='\0';

    GetinACC(X, vide, 0);
    emit_CHS();
    strncpy(acc, res, 49); acc[49]='\0';
    emit_STORE(res);
}

/* ================================================================
 * 6. Affectation simple  ( =, src, "", dest )
 * ================================================================ */

static void gen_affect(const char *src, const char *dest)
{
    if (strcmp(acc, src) != 0)
        emit_LOAD(src);
    emit_STORE(dest);
    strncpy(acc, dest, 49); acc[49]='\0';
}

/* ================================================================
 * 7. Tableaux
 *    =[]   tab[idx]  ""   res     (lecture)
 *    []=   val       ""   tab[idx](ecriture)
 * ================================================================ */

static void gen_read_tab(const char *tabidx, const char *res)
{
    char base[50], idx[50];
    if (sscanf(tabidx, "%49[^[][%49[^]]", base, idx) != 2) {
        fprintf(OUT, "    ; ERREUR format tableau : %s\n", tabidx); return;
    }
    /* Calcul adresse : BX = index * 2 (WORD) */
    fprintf(OUT, "    ; lecture %s[%s]\n", base, idx);
    if (est_litteral(idx))
        fprintf(OUT, "    MOV  BX, %s\n", idx);
    else
        fprintf(OUT, "    MOV  BX, [%s]\n", mem_of(idx));
    fprintf(OUT, "    SHL  BX, 1\n");
    fprintf(OUT, "    MOV  AX, %s[BX]           ; AX = %s[%s]\n", base, base, idx);
    emit_STORE(res);
    strncpy(acc, res, 49); acc[49]='\0';
}

static void gen_write_tab(const char *val, const char *tabidx)
{
    char base[50], idx[50];
    if (sscanf(tabidx, "%49[^[][%49[^]]", base, idx) != 2) {
        fprintf(OUT, "    ; ERREUR format tableau : %s\n", tabidx); return;
    }
    fprintf(OUT, "    ; ecriture %s[%s]\n", base, idx);
    /* Sauvegarder la valeur si elle n'est pas dans AX */
    if (strcmp(acc, val) != 0)
        emit_LOAD(val);
    fprintf(OUT, "    PUSH AX\n");
    if (est_litteral(idx))
        fprintf(OUT, "    MOV  BX, %s\n", idx);
    else
        fprintf(OUT, "    MOV  BX, [%s]\n", mem_of(idx));
    fprintf(OUT, "    SHL  BX, 1\n");
    fprintf(OUT, "    POP  AX\n");
    fprintf(OUT, "    MOV  %s[BX], AX           ; %s[%s] = %s\n",
            base, base, idx, val);
    acc[0] = '\0';
}

/* ================================================================
 * 8. Branchements conditionnels
 *
 *  Quads : BG/BL/BGE/BLE/BEQ/BNEQ   cible   gauche   droite
 *  Strategie :
 *    LOAD  gauche
 *    CMP   AX, [droite]    (SUB sans stocker = CMP en 8086)
 *    Jcc   Lcible
 * ================================================================ */

static void gen_branch_cond(const char *op,
                             const char *cible,
                             const char *gauche,
                             const char *droite)
{
    /* Charger gauche dans AX (GetinACC simplifie : juste LOAD) */
    emit_LOAD(gauche);

    /* CMP AX, droite  (equivalent SUB sans ecrire le resultat) */
    if (est_litteral(droite))
        fprintf(OUT, "    CMP  AX, %-10s          ; comparaison\n", droite);
    else
        fprintf(OUT, "    CMP  AX, [%-10s]        ; comparaison\n",
                mem_of(droite));

    acc[0] = '\0'; /* AX modifie par CMP */

    /* Instruction de saut 8086 */
    char label[30];
    snprintf(label, sizeof(label), "L%s", cible);

    const char *jcc = "JMP";
    if      (strcmp(op,"BG")   == 0) jcc = "JG";
    else if (strcmp(op,"BL")   == 0) jcc = "JL";
    else if (strcmp(op,"BGE")  == 0) jcc = "JGE";
    else if (strcmp(op,"BLE")  == 0) jcc = "JLE";
    else if (strcmp(op,"BEQ")  == 0) jcc = "JE";
    else if (strcmp(op,"BNEQ") == 0) jcc = "JNE";

    emit_Bcc(jcc, label);
}

/* ================================================================
 * 9. Traitement d'un quadruplet
 * ================================================================ */

static void traiter_quad(int i)
{
    const char *op  = quads[i].op;
    const char *a1  = quads[i].arg1;
    const char *a2  = quads[i].arg2;
    const char *res = quads[i].res;

    /* Commentaire quad source */
    fprintf(OUT, "; (%3d)  %-6s  %-12s  %-12s  %s\n", i, op, a1, a2, res);

    if (strcmp(op, "NOP") == 0) return;

    if (strcmp(op, "=") == 0 && a2[0] == '\0') {
        gen_affect(a1, res); return;
    }
    if (strcmp(op, "=[]") == 0) { gen_read_tab(a1, res);  return; }
    if (strcmp(op, "[]=") == 0) { gen_write_tab(a1, res); return; }

    if (strcmp(op, "+")      == 0) { gen_add(a1, a2, res);    return; }
    if (strcmp(op, "-")      == 0) { gen_sub(a1, a2, res);    return; }
    if (strcmp(op, "*")      == 0) { gen_mult(a1, a2, res);   return; }
    if (strcmp(op, "/")      == 0) { gen_div(a1, a2, res);    return; }
    if (strcmp(op, "UMINUS") == 0) { gen_uminus(a1, res);     return; }

    /* Branchement inconditionnel */
    if (strcmp(op, "BR") == 0) {
        char label[30];
        snprintf(label, sizeof(label), "L%s", a1);
        emit_B(label);
        acc[0] = '\0';
        return;
    }

    /* Branchements conditionnels */
    if (strcmp(op,"BG")  == 0 || strcmp(op,"BL")   == 0 ||
        strcmp(op,"BGE") == 0 || strcmp(op,"BLE")  == 0 ||
        strcmp(op,"BEQ") == 0 || strcmp(op,"BNEQ") == 0) {
        gen_branch_cond(op, a1, a2, res);
        return;
    }

    fprintf(OUT, "    ; !!! quad non traduit : (%s,%s,%s,%s)\n",op,a1,a2,res);
}

/* ================================================================
 * 10. Segment DATA
 * ================================================================ */

static void emit_data(void)
{
    fprintf(OUT, "DATA    SEGMENT\n\n");
    fprintf(OUT, "    ; --- Variables utilisateur ---\n");
    /* Parcours de la table de hachage (TAILLE_TABLE buckets).
     * Chaque bucket est une liste chainee de Symbole*.          */
    for (int i = 0; i < TAILLE_TABLE; i++) {
        for (Symbole *s = table[i]; s != NULL; s = s->suivant) {
            if (s->categorie == VAR) {
                fprintf(OUT, "    %-12s DW  0\n", s->nom);
            } else if (s->categorie == TABLEAU) {
                fprintf(OUT, "    %-12s DW  %d DUP(0)   ; tableau[%d]\n",
                        s->nom, s->taille, s->taille);
            } else if (s->categorie == CONSTANTE) {
                if (s->type == TYPE_INTEGER)
                    fprintf(OUT, "    %-12s DW  %d\n", s->nom, s->val_entiere);
                else
                    fprintf(OUT, "    %-12s DW  %d\n",
                            s->nom, (int)s->val_reelle);
            }
        }
    }
    fprintf(OUT, "\n    ; --- Temporaires ---\n");
    for (int i = 0; i < nb_temps; i++)
        fprintf(OUT, "    %-12s DW  0\n", temp_table[i].addr);

    fprintf(OUT, "\n    ; --- Sauvegardes ACC ---\n");
    for (int k = 0; k < save_count; k++) {
        char sav[20];
        snprintf(sav, sizeof(sav), "_SAVE%d", k);
        fprintf(OUT, "    %-12s DW  0\n", sav);
    }
    fprintf(OUT, "\nDATA    ENDS\n\n");
}

/* ================================================================
 * 11. Point d'entree : generer_code(FILE *out)
 * ================================================================ */

void generer_code(FILE *out)
{
    OUT        = out;
    acc[0]     = '\0';
    nb_temps   = 0;
    save_count = 0;

    /* Passe 0 : enregistrer tous les temporaires pour le segment DATA */
    for (int i = 0; i < qc; i++) {
        const char *f[3] = { quads[i].arg1, quads[i].arg2, quads[i].res };
        for (int j = 0; j < 3; j++)
            if (est_temporaire(f[j])) addr_temp(f[j]);
    }

    /* En-tete */
    fprintf(OUT,
        "; ============================================================\n"
        "; Code assembleur 8086 genere automatiquement\n"
        "; Methode : GetinACC du cours (machine a accumulateur)\n"
        "; AX = accumulateur (ACC),  BX = registre auxiliaire\n"
        "; ============================================================\n\n"
        "    .MODEL SMALL\n"
        "    .STACK 256\n\n");

    /* Generation du segment CODE dans un buffer temporaire
       (pour connaitre le nombre de _SAVEn avant d'ecrire DATA) */
    FILE *code_buf = tmpfile();
    if (!code_buf) { perror("tmpfile"); return; }

    FILE *final = OUT;
    OUT = code_buf;
    acc[0] = '\0';
    save_count = 0;

    fprintf(OUT, "CODE    SEGMENT\n");
    fprintf(OUT, "        ASSUME  CS:CODE, DS:DATA\n\n");
    fprintf(OUT, "DEBUT:\n");
    fprintf(OUT, "    MOV  AX, DATA\n");
    fprintf(OUT, "    MOV  DS, AX\n\n");

    for (int i = 0; i < qc; i++) {
        fprintf(OUT, "L%d:\n", i);
        traiter_quad(i);
        fprintf(OUT, "\n");
    }
    fprintf(OUT, "L%d:\n", qc);
    fprintf(OUT, "    ; --- fin ---\n");
    fprintf(OUT, "    MOV  AH, 4Ch\n");
    fprintf(OUT, "    MOV  AL, 0\n");
    fprintf(OUT, "    INT  21h\n\n");
    fprintf(OUT, "CODE    ENDS\n");
    fprintf(OUT, "        END  DEBUT\n");

    /* Ecriture finale : DATA puis CODE */
    OUT = final;
    emit_data();
    rewind(code_buf);
    int c;
    while ((c = fgetc(code_buf)) != EOF) fputc(c, OUT);
    fclose(code_buf);
}
