%{
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "symboles.h"
#include "quad.h"
#include "bison.tab.h"

int nb_ligne = 1;
int nb_colonne = 1;

%}

%option noyywrap

lettre    [a-zA-Z]
chiffre   [0-9]
IDF       {lettre}({lettre}|{chiffre}|_)*
entier    {chiffre}+
reel      {chiffre}+"."{chiffre}+
signe     [+-]

%%


"//"[^\n]* { nb_colonne += yyleng; }

"/*"([^*]|\*+[^*/])*\*+"/" {
    for(int i=0;i<yyleng;i++){
        if(yytext[i]=='\n'){ nb_ligne++; nb_colonne=1; }
        else nb_colonne++;
    }
}


[ \t]+ { nb_colonne += yyleng; }
\n     { nb_ligne++; nb_colonne = 1; }


"PROGRAM"  { nb_colonne+=yyleng; return PROGRAM; }
"DECL"     { nb_colonne+=yyleng; return DECL; }
"ENDDECL"  { nb_colonne+=yyleng; return ENDDECL; }
"BEGIN"    { nb_colonne+=yyleng; return BEGIN_TOK; }
"END"      { nb_colonne+=yyleng; return END; }

"IF"       { nb_colonne+=yyleng; return IF; }
"ELSE"     { nb_colonne+=yyleng; return ELSE; }
"FOR"      { nb_colonne+=yyleng; return FOR; }
"WHILE"    { nb_colonne+=yyleng; return WHILE; }

"INTEGER"  { nb_colonne+=yyleng; return INTEGER; }
"FLOAT"    { nb_colonne+=yyleng; return FLOAT; }
"CONST"    { nb_colonne+=yyleng; return CONST; }


{IDF} {
    nb_colonne += yyleng;

    if(yyleng > 8){
        fprintf(stderr,
        "Erreur Lexicale ligne %d col %d : IDF trop long '%s'\n",
        nb_ligne, nb_colonne, yytext);
    }

    strncpy(yylval.idf, yytext, 8);
    yylval.idf[8] = '\0';

    return IDF;
}


{reel} {
    nb_colonne += yyleng;
    yylval.reel = atof(yytext);
    return CST_REEL;
}

{entier} {
    int col = nb_colonne;
    nb_colonne += yyleng;

    int v = atoi(yytext);

    if(v < -32768 || v > 32767)
        fprintf(stderr,
        "Erreur Lexicale ligne %d col %d : overflow\n",
        nb_ligne, col);

    yylval.entier = v;
    return CST_ENT;
}


"("({signe}{reel})")" {
    int col = nb_colonne;
    nb_colonne += yyleng;


    char buf[32];
    strncpy(buf, yytext + 1, yyleng - 2);
    buf[yyleng - 2] = '\0';


    yylval.reel = atof(buf);
    return CST_REEL;
}


"("({signe}{entier})")" {
    int col = nb_colonne;
    nb_colonne += yyleng;


    char buf[32];
    strncpy(buf, yytext + 1, yyleng - 2);
    buf[yyleng - 2] = '\0';


    int v = atoi(buf);


    if (v < -32768 || v > 32767)
        fprintf(stderr,
            "Erreur Lexicale : ligne %d , colonne %d , '%s' : valeur hors intervalle\n",
            nb_ligne, col, yytext);


    yylval.entier = v;
    return CST_ENT;
}

{signe}{entier} {
    fprintf(stderr,
        "Erreur Lexicale : ligne %d , colonne %d , '%s' : entier signe doit etre entre parentheses\n",
        nb_ligne, nb_colonne, yytext);
}

{signe}{reel} {
    fprintf(stderr,
        "Erreur Lexicale : ligne %d , colonne %d , '%s' : reel signe doit etre entre parentheses\n",
        nb_ligne, nb_colonne, yytext);
}


"+"  { nb_colonne++; return PLUS; }
"-"  { nb_colonne++; return MINUS; }
"*"  { nb_colonne++; return MULT; }
"/"  { nb_colonne++; return DIV; }

">=" { nb_colonne+=2; return GE; }
"<=" { nb_colonne+=2; return LE; }
"==" { nb_colonne+=2; return EQ; }
"!=" { nb_colonne+=2; return NEQ; }

">"  { nb_colonne++; return GT; }
"<"  { nb_colonne++; return LT; }

"&&" { nb_colonne+=2; return AND; }
"||" { nb_colonne+=2; return OR; }
"!"  { nb_colonne++; return NOT; }

"="  { nb_colonne++; return ASSIGN; }


";" { nb_colonne++; return SEMI; }
":" { nb_colonne++; return COLON; }
"," { nb_colonne++; return COMMA; }

"(" { nb_colonne++; return LPAREN; }
")" { nb_colonne++; return RPAREN; }

"{" { nb_colonne++; return LBRACE; }
"}" { nb_colonne++; return RBRACE; }

"[" { nb_colonne++; return LBRACKET; }
"]" { nb_colonne++; return RBRACKET; }


. {
    fprintf(stderr,
    "Erreur Lexicale ligne %d col %d : '%s'\n",
    nb_ligne, nb_colonne, yytext);

    nb_colonne++;
}

%%
