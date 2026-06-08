/*
 * codegen.h  –  Interface du générateur de code objet
 *
 * Machine cible : accumulateur + 8 registres
 * Jeu d'instructions : LOAD STORE ADD SUB MULT DIV CHS B Bxx
 *
 * Licence : code original open-source (MIT)
 */
#ifndef CODEGEN_H
#define CODEGEN_H

#include <stdio.h>

/*
 * generer_code(out)
 *
 * Génère le code objet (machine à accumulateur)
 * à partir du tableau global de quadruplets quads[qc]
 * (après appel à optimiser()).
 *
 * Paramètre :
 *   out  – fichier de sortie ouvert en écriture
 *          (ex. : FILE *f = fopen("prog.obj","w");)
 */
void generer_code(FILE *out);

#endif /* CODEGEN_H */
