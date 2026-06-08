/* A Bison parser, made by GNU Bison 2.7.  */

/* Bison interface for Yacc-like parsers in C
   
      Copyright (C) 1984, 1989-1990, 2000-2012 Free Software Foundation, Inc.
   
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.
   
   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

#ifndef YY_YY_BISON_TAB_H_INCLUDED
# define YY_YY_BISON_TAB_H_INCLUDED
/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int yydebug;
#endif

/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     PROGRAM = 258,
     DECL = 259,
     ENDDECL = 260,
     BEGIN_TOK = 261,
     END = 262,
     IF = 263,
     ELSE = 264,
     FOR = 265,
     WHILE = 266,
     INTEGER = 267,
     FLOAT = 268,
     CONST = 269,
     SEMI = 270,
     COLON = 271,
     COMMA = 272,
     LPAREN = 273,
     RPAREN = 274,
     LBRACE = 275,
     RBRACE = 276,
     LBRACKET = 277,
     RBRACKET = 278,
     ASSIGN = 279,
     PLUS = 280,
     MINUS = 281,
     MULT = 282,
     DIV = 283,
     AND = 284,
     OR = 285,
     NOT = 286,
     GE = 287,
     LE = 288,
     EQ = 289,
     NEQ = 290,
     GT = 291,
     LT = 292,
     IDF = 293,
     CST_ENT = 294,
     CST_REEL = 295,
     UMINUS = 296
   };
#endif


#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{
/* Line 2058 of yacc.c  */
#line 31 "bison.y"

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


/* Line 2058 of yacc.c  */
#line 116 "bison.tab.h"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif

extern YYSTYPE yylval;

#ifdef YYPARSE_PARAM
#if defined __STDC__ || defined __cplusplus
int yyparse (void *YYPARSE_PARAM);
#else
int yyparse ();
#endif
#else /* ! YYPARSE_PARAM */
#if defined __STDC__ || defined __cplusplus
int yyparse (void);
#else
int yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */

#endif /* !YY_YY_BISON_TAB_H_INCLUDED  */
