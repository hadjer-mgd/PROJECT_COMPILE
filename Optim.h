#ifndef OPTIM_H
#define OPTIM_H

#include "quad.h"

void optimiser();
void propagation_constantes();
void simplification_constantes();
void elimination_code_mort();
void elimination_affectations_inutiles();
void reduction_force();
void elimination_copie_inutile();
void elimination_sous_expressions_communes();
void simplification_branches_constantes();
void suppression_sauts_inutiles();
void optimisation_boucles();

#endif
