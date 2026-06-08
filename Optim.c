#include "optim.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int est_entier(const char *s, int *val)
{
    if (!s || s[0] == '\0') return 0;
    char *end;
    long v = strtol(s, &end, 10);
    if (*end == '\0') { *val = (int)v; return 1; }
    return 0;
}

static int est_reel(const char *s, double *val)
{
    if (!s || s[0] == '\0') return 0;
    char *end;
    double v = strtod(s, &end);
    if (*end == '\0') { *val = v; return 1; }
    return 0;
}

static int est_nop(int i)
{
    return strcmp(quads[i].op, "NOP") == 0;
}

static void marquer_nop(int i)
{
    strcpy(quads[i].op,   "NOP");
    strcpy(quads[i].arg1, "");
    strcpy(quads[i].arg2, "");
    strcpy(quads[i].res,  "");
}

static int est_litterale(const char *s)
{
    int vi; double vd;
    return est_entier(s, &vi) || est_reel(s, &vd);
}

static int modifie_dans(const char *dest, int debut, int fin)
{
    if (!dest || dest[0] == '\0') return 0;
    for (int k = debut; k < fin; k++) {
        if (est_nop(k)) continue;
        if (strcmp(quads[k].res, dest) == 0) return 1;
    }
    return 0;
}

static int est_variable_boucle(const char *dest, int init_idx)
{
    for (int br = 0; br < qc; br++) {
        if (est_nop(br)) continue;
        if (quads[br].op[0] != 'B') continue;
        int tgt;
        if (sscanf(quads[br].arg1, "%d", &tgt) != 1) continue;
        if (tgt >= br) continue; 
        if (init_idx < tgt || init_idx > br) continue;
        if (modifie_dans(dest, tgt, br + 1)) return 1;
    }
    return 0;
}


static int usage_dans_boucle_de(const char *dest, int use_idx)
{
    for (int br = 0; br < qc; br++) {
        if (est_nop(br)) continue;
        if (quads[br].op[0] != 'B') continue;
        int tgt;
        if (sscanf(quads[br].arg1, "%d", &tgt) != 1) continue;
        if (tgt >= br) continue;
        if (use_idx < tgt || use_idx > br) continue;
        if (modifie_dans(dest, tgt, br + 1)) return 1;
    }
    return 0;
}

void elimination_copie_inutile()
{
    int modif = 1;
    while (modif) {
        modif = 0;
        for (int i = 0; i < qc - 1; i++) {
            if (est_nop(i)) continue;

            const char *ti = quads[i].res;
            if (ti[0] != 'T' || ti[1] < '0' || ti[1] > '9') continue;
            if (quads[i].op[0] == 'B') continue;

            int j = i + 1;
            while (j < qc && est_nop(j)) j++;
            if (j >= qc) continue;

            if (strcmp(quads[j].op,   "=") != 0) continue;
            if (quads[j].arg2[0]           != '\0') continue;
            if (strcmp(quads[j].arg1, ti)  != 0) continue;

            const char *var = quads[j].res;

            int utilise = 0;
            for (int k = 0; k < qc; k++) {
                if (k == i || k == j || est_nop(k)) continue;
                if (quads[k].op[0] != 'B' &&
                    strcmp(quads[k].arg1, ti) == 0) { utilise = 1; break; }
                if (strcmp(quads[k].arg2, ti) == 0) { utilise = 1; break; }
                if (quads[k].op[0] == 'B' &&
                    strcmp(quads[k].op, "BR") != 0 &&
                    strcmp(quads[k].res, ti) == 0) { utilise = 1; break; }
            }

            if (!utilise) {
                strncpy(quads[i].res, var, sizeof(quads[i].res) - 1);
                quads[i].res[sizeof(quads[i].res) - 1] = '\0';
                marquer_nop(j);
                modif = 1;
            }
        }
    }
}

void simplification_constantes()
{
    for (int i = 0; i < qc; i++) {
        if (est_nop(i)) continue;
        char *op = quads[i].op;
        char *a1 = quads[i].arg1;
        char *a2 = quads[i].arg2;

        if (strcmp(op, "UMINUS") == 0 && a2[0] == '\0') {
            double v1; int vi1;
            int a1_int = est_entier(a1, &vi1);
            if (est_reel(a1, &v1)) {
                strcpy(quads[i].op,   "=");
                strcpy(quads[i].arg2, "");
                if (a1_int) sprintf(quads[i].arg1, "%d", -vi1);
                else        sprintf(quads[i].arg1, "%g", -v1);
            }
            continue;
        }

        double v1, v2;
        int vi1, vi2;
        int a1_int  = est_entier(a1, &vi1);
        int a2_int  = est_entier(a2, &vi2);
        int a1_reel = est_reel(a1, &v1);
        int a2_reel = est_reel(a2, &v2);
        if (!a1_reel || !a2_reel) continue;

        double resultat = 0;
        int    valide   = 1;
        int    res_int  = a1_int && a2_int;

        if      (strcmp(op, "+") == 0) resultat = v1 + v2;
        else if (strcmp(op, "-") == 0) resultat = v1 - v2;
        else if (strcmp(op, "*") == 0) resultat = v1 * v2;
        else if (strcmp(op, "/") == 0) {
            if (v2 == 0) { valide = 0; }
            else { resultat = v1 / v2; res_int = 0; }
        }
        else continue;

        if (!valide) continue;
        strcpy(quads[i].op,   "=");
        strcpy(quads[i].arg2, "");
        if (res_int) sprintf(quads[i].arg1, "%d", (int)resultat);
        else         sprintf(quads[i].arg1, "%g", resultat);
    }
}

void propagation_constantes()
{
    int modif = 1;
    while (modif) {
        modif = 0;
        for (int i = 0; i < qc; i++) {
            if (est_nop(i)) continue;
            if (strcmp(quads[i].op, "=") != 0) continue;
            if (quads[i].arg2[0] != '\0')       continue;
            if (quads[i].res[0]  == '\0')        continue;
            if (!est_litterale(quads[i].arg1))   continue;

            const char *cst  = quads[i].arg1;
            const char *dest = quads[i].res;

            if (est_variable_boucle(dest, i)) continue;

            int limite = qc;
            for (int k = i + 1; k < qc; k++) {
                if (est_nop(k)) continue;
                if (strcmp(quads[k].res, dest) == 0) { limite = k; break; }
            }

            for (int j = i + 1; j < limite; j++) {
                if (est_nop(j)) continue;

                int is_branch = (quads[j].op[0] == 'B');
                int is_cond   = is_branch &&
                                strcmp(quads[j].op, "BR") != 0;

                if (usage_dans_boucle_de(dest, j)) continue;

                int changed = 0;

                if (!is_branch &&
                    strcmp(quads[j].arg1, dest) == 0) {
                    strncpy(quads[j].arg1, cst, sizeof(quads[j].arg1)-1);
                    changed = 1;
                }

                if (!is_cond &&
                    strcmp(quads[j].arg2, dest) == 0) {
                    strncpy(quads[j].arg2, cst, sizeof(quads[j].arg2)-1);
                    changed = 1;
                }

                if (changed) modif = 1;
            }
        }
    }
}

void reduction_force()
{
    for (int i = 0; i < qc; i++) {
        if (est_nop(i)) continue;
        char *op = quads[i].op;
        char *a1 = quads[i].arg1;
        char *a2 = quads[i].arg2;
        int vi;

        if (strcmp(op, "*") == 0) {
            if ((est_entier(a2, &vi) && vi == 0) ||
                (est_entier(a1, &vi) && vi == 0)) {
                strcpy(quads[i].op,   "=");
                strcpy(quads[i].arg1, "0");
                strcpy(quads[i].arg2, "");
            }
            else if (est_entier(a2, &vi) && vi == 1) {
                strcpy(quads[i].op,   "=");
                strcpy(quads[i].arg2, "");
            }
            else if (est_entier(a1, &vi) && vi == 1) {
                strcpy(quads[i].op,   "=");
                strncpy(quads[i].arg1, a2, sizeof(quads[i].arg1)-1);
                strcpy(quads[i].arg2, "");
            }
            else if (est_entier(a2, &vi) && vi == 2) {
                strcpy(quads[i].op, "+");
                strncpy(quads[i].arg2, a1, sizeof(quads[i].arg2)-1);
            }
            else if (est_entier(a1, &vi) && vi == 2) {
                strcpy(quads[i].op, "+");
                strncpy(quads[i].arg1, a2, sizeof(quads[i].arg1)-1);
                strncpy(quads[i].arg2, a2, sizeof(quads[i].arg2)-1);
            }
        }
        else if (strcmp(op, "+") == 0) {
            if (est_entier(a2, &vi) && vi == 0) {
                strcpy(quads[i].op,   "=");
                strcpy(quads[i].arg2, "");
            }
            else if (est_entier(a1, &vi) && vi == 0) {
                strcpy(quads[i].op,   "=");
                strncpy(quads[i].arg1, a2, sizeof(quads[i].arg1)-1);
                strcpy(quads[i].arg2, "");
            }
        }
        else if (strcmp(op, "-") == 0) {
            if (est_entier(a2, &vi) && vi == 0) {
                strcpy(quads[i].op,   "=");
                strcpy(quads[i].arg2, "");
            }
            else if (strcmp(a1, a2) == 0 && a1[0] != '\0' &&
                     !est_litterale(a1)) {
                strcpy(quads[i].op,   "=");
                strcpy(quads[i].arg1, "0");
                strcpy(quads[i].arg2, "");
            }
        }
        else if (strcmp(op, "/") == 0) {
            if (est_entier(a2, &vi) && vi == 1) {
                strcpy(quads[i].op,   "=");
                strcpy(quads[i].arg2, "");
            }
        }
    }
}

void elimination_affectations_inutiles()
{
    for (int i = 0; i < qc; i++) {
        if (est_nop(i)) continue;

        const char *op  = quads[i].op;
        const char *a1  = quads[i].arg1;
        const char *a2  = quads[i].arg2;
        const char *res = quads[i].res;
        int vi;

        if (strcmp(op, "=") == 0 && a2[0] == '\0' &&
            strcmp(a1, res) == 0)
        {
            marquer_nop(i);
            continue;
        }

        if (strcmp(op, "+") == 0 &&
            est_entier(a2, &vi) && vi == 0 &&
            strcmp(a1, res) == 0)
        {
            marquer_nop(i);
            continue;
        }

        if (strcmp(op, "+") == 0 &&
            est_entier(a1, &vi) && vi == 0 &&
            strcmp(a2, res) == 0)
        {
            marquer_nop(i);
            continue;
        }

        if (strcmp(op, "-") == 0 &&
            est_entier(a2, &vi) && vi == 0 &&
            strcmp(a1, res) == 0)
        {
            marquer_nop(i);
            continue;
        }

        if (strcmp(op, "*") == 0 &&
            est_entier(a2, &vi) && vi == 1 &&
            strcmp(a1, res) == 0)
        {
            marquer_nop(i);
            continue;
        }

        if (strcmp(op, "/") == 0 &&
            est_entier(a2, &vi) && vi == 1 &&
            strcmp(a1, res) == 0)
        {
            marquer_nop(i);
            continue;
        }
    }
}

void elimination_code_mort()
{
    int modif = 1;
    while (modif) {
        modif = 0;
        for (int i = 0; i < qc; i++) {
            if (est_nop(i)) continue;
            const char *res = quads[i].res;
            if (res[0] != 'T' || res[1] < '0' || res[1] > '9') continue;
            if (quads[i].op[0] == 'B') continue;

            int utilise = 0;
            for (int j = i + 1; j < qc; j++) {
                if (est_nop(j)) continue;

                int is_cond = (quads[j].op[0] == 'B') &&
                              strcmp(quads[j].op, "BR") != 0;

                if (!( quads[j].op[0] == 'B') &&
                    strcmp(quads[j].arg1, res) == 0) {
                    utilise = 1; break;
                }
                if (strcmp(quads[j].arg2, res) == 0) {
                    utilise = 1; break;
                }

                if (is_cond && strcmp(quads[j].res, res) == 0) {
                    utilise = 1; break;
                }
                if (!is_cond && strcmp(quads[j].res, res) == 0) break;
            }
            if (!utilise) { marquer_nop(i); modif = 1; }
        }
    }
}

void elimination_sous_expressions_communes()
{
    for (int i = 0; i < qc; i++) {
        if (est_nop(i)) continue;
        if (quads[i].op[0] == 'B') continue;
        char *op = quads[i].op;
        if (strcmp(op,"+") && strcmp(op,"-") &&
            strcmp(op,"*") && strcmp(op,"/")) continue;

        for (int j = i + 1; j < qc; j++) {
            if (est_nop(j)) continue;
            if (quads[j].op[0] == 'B') continue;
            if (strcmp(quads[j].op,   op)            == 0 &&
                strcmp(quads[i].arg1, quads[j].arg1) == 0 &&
                strcmp(quads[i].arg2, quads[j].arg2) == 0) {
                strcpy(quads[j].op,   "=");
                strncpy(quads[j].arg1, quads[i].res, sizeof(quads[j].arg1)-1);
                strcpy(quads[j].arg2, "");
            }
            if (strcmp(quads[j].res, quads[i].arg1) == 0 ||
                strcmp(quads[j].res, quads[i].arg2) == 0) break;
        }
    }
}

void simplification_branches_constantes()
{
    for (int i = 0; i < qc; i++) {
        if (est_nop(i)) continue;
        char *op = quads[i].op;
        int is_cond = (strcmp(op,"BG")  == 0 || strcmp(op,"BL")   == 0 ||
                       strcmp(op,"BGE") == 0 || strcmp(op,"BLE")  == 0 ||
                       strcmp(op,"BEQ") == 0 || strcmp(op,"BNEQ") == 0);
        if (!is_cond) continue;

        int v1, v2;
        if (!est_entier(quads[i].arg2, &v1)) continue;
        if (!est_entier(quads[i].res,  &v2)) continue;

        int cond = 0;
        if      (strcmp(op,"BG")   == 0) cond = (v1 >  v2);
        else if (strcmp(op,"BL")   == 0) cond = (v1 <  v2);
        else if (strcmp(op,"BGE")  == 0) cond = (v1 >= v2);
        else if (strcmp(op,"BLE")  == 0) cond = (v1 <= v2);
        else if (strcmp(op,"BEQ")  == 0) cond = (v1 == v2);
        else if (strcmp(op,"BNEQ") == 0) cond = (v1 != v2);

        if (cond) {
            strcpy(quads[i].op,   "BR");
            strcpy(quads[i].arg2, "");
            strcpy(quads[i].res,  "");
        } else {
            marquer_nop(i);
        }
    }
}

void suppression_sauts_inutiles()
{
    for (int i = 0; i < qc; i++) {
        if (est_nop(i)) continue;
        if (strcmp(quads[i].op, "BR") != 0) continue;
        int target;
        if (sscanf(quads[i].arg1, "%d", &target) == 1 &&
            target == i + 1)
            marquer_nop(i);
    }
}

static int est_invariant(const char *s, int debut, int fin)
{
    int vi; double vd;
    if (est_entier(s, &vi) || est_reel(s, &vd)) return 1;
    return !modifie_dans(s, debut, fin);
}

void optimisation_boucles()
{
    for (int i = 0; i < qc; i++) {
        if (est_nop(i)) continue;
        if (strcmp(quads[i].op, "BR") != 0) continue;
        int target;
        if (sscanf(quads[i].arg1, "%d", &target) != 1) continue;
        if (target >= i) continue;  

        int debut = target;
        int fin   = i;

        for (int j = debut; j < fin; j++) {
            if (est_nop(j)) continue;
            char *op  = quads[j].op;
            char *a1  = quads[j].arg1;
            char *a2  = quads[j].arg2;
            char *res = quads[j].res;

            if (strcmp(op,"+") && strcmp(op,"-") &&
                strcmp(op,"*") && strcmp(op,"/")) continue;
            if (!est_invariant(a1, debut, fin)) continue;
            if (!est_invariant(a2, debut, fin)) continue;
            if (modifie_dans(res, j + 1, fin))  continue;

            Quad tmp = quads[j];
            for (int k = j; k > debut; k--)
                quads[k] = quads[k - 1];
            quads[debut] = tmp;

            for (int k = 0; k < qc; k++) {
                if (est_nop(k)) continue;
                if (quads[k].op[0] != 'B') continue;
                int t;
                if (sscanf(quads[k].arg1, "%d", &t) != 1) continue;
                if (t >= debut && t <= fin)
                    sprintf(quads[k].arg1, "%d", t + 1);
            }

            debut++; fin++; i++;
            j = debut - 1;
        }
    }
}

static void elimination_code_inaccessible()
{
    int cible[1000];
    memset(cible, 0, sizeof(cible));
    for (int i = 0; i < qc; i++) {
        if (est_nop(i)) continue;
        if (quads[i].op[0] == 'B') {
            int t;
            if (sscanf(quads[i].arg1, "%d", &t) == 1 &&
                t >= 0 && t < qc)
                cible[t] = 1;
        }
    }
    int inaccessible = 0;
    for (int i = 0; i < qc; i++) {
        if (est_nop(i)) continue;
        if (cible[i]) inaccessible = 0;
        if (inaccessible) { marquer_nop(i); continue; }
        if (strcmp(quads[i].op, "BR") == 0) {
            int t;
            if (sscanf(quads[i].arg1, "%d", &t) == 1 && t > i)
                inaccessible = 1;
        }
    }
}

static void elimination_affectations_redondantes()
{
    for (int i = 0; i < qc; i++) {
        if (est_nop(i)) continue;
        if (strcmp(quads[i].op, "=") != 0) continue;
        if (quads[i].arg2[0] != '\0') continue;
        const char *dest = quads[i].res;
        const char *val  = quads[i].arg1;
        for (int j = i + 1; j < qc; j++) {
            if (est_nop(j)) continue;
            if (strcmp(quads[j].op,   "=")  == 0 &&
                quads[j].arg2[0]            == '\0' &&
                strcmp(quads[j].res,  dest) == 0 &&
                strcmp(quads[j].arg1, val)  == 0) {
                marquer_nop(j); continue;
            }
            if (strcmp(quads[j].arg1, dest) == 0 ||
                strcmp(quads[j].arg2, dest) == 0 ||
                strcmp(quads[j].res,  dest) == 0) break;
        }
    }
}

static void compacter_quads()
{
    int new_addr[1001];
    int pos = 0;

    for (int i = 0; i < qc; i++)
        new_addr[i] = est_nop(i) ? -1 : pos++;
    new_addr[qc] = pos;

    for (int i = 0; i < qc; i++) {
        if (est_nop(i)) continue;
        if (quads[i].op[0] != 'B') continue;
        int target;
        if (sscanf(quads[i].arg1, "%d", &target) != 1) continue;
        if (target < 0 || target > qc) continue;
        while (target < qc && new_addr[target] == -1) target++;
        int nouvelle = (target <= qc) ? new_addr[target] : pos;
        if (nouvelle >= 0)
            sprintf(quads[i].arg1, "%d", nouvelle);
    }

    int nqc = 0;
    for (int i = 0; i < qc; i++)
        if (!est_nop(i)) quads[nqc++] = quads[i];
    qc = nqc;
}

void optimiser()
{
    int changed = 1;
    int iter    = 0;

    while (changed && iter < 10) {
        changed      = 0;
        int qc_avant = qc;

        elimination_copie_inutile();
        simplification_branches_constantes();
        elimination_code_inaccessible();
        propagation_constantes();
        simplification_constantes();
        reduction_force();
        elimination_affectations_inutiles();
        elimination_affectations_redondantes();
        elimination_code_mort();
        compacter_quads();

        suppression_sauts_inutiles();
        optimisation_boucles();

        if (qc != qc_avant) changed = 1;
        iter++;
    }
}
