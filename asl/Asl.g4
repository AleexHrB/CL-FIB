//////////////////////////////////////////////////////////////////////
//
//    Asl - Another simple language (grammar)
//
//    Copyright (C) 2020-2030  Universitat Politecnica de Catalunya
//
//    This library is free software; you can redistribute it and/or
//    modify it under the terms of the GNU General Public License
//    as published by the Free Software Foundation; either version 3
//    of the License, or (at your option) any later version.
//
//    This library is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//    Affero General Public License for more details.
//
//    You should have received a copy of the GNU Affero General Public
//    License along with this library; if not, write to the Free Software
//    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
//
//    contact: José Miguel Rivero (rivero@cs.upc.edu)
//             Computer Science Department
//             Universitat Politecnica de Catalunya
//             despatx Omega.110 - Campus Nord UPC
//             08034 Barcelona.  SPAIN
//
//////////////////////////////////////////////////////////////////////

grammar Asl;

//////////////////////////////////////////////////
/// Parser Rules
//////////////////////////////////////////////////

// A program is a list of functions
program : function+ EOF
        ;

// A function has a name, a list of parameters and a list of statements

function
        : FUNC ID '(' parameters ? ')' (':' type)? declarations statements ENDFUNC
        ;

parameters : (ID ':' type (',' ID ':' type)*)
           ;

declarations
        : (variable_decl)*
        ;

variable_decl
        : VAR ID (',' ID)* ':' type
        ;

type:  basic_type
      | ARRAY '[' INTVAL ']' 'of' basic_type
      ;

basic_type : INT
        | BOOL
        | FLOAT
        | CHAR
        ;

statements
        : (statement)*
        ;

// The different types of instructions
statement
        : left_expr ASSIGN expr ';'           # assignStmt
        | RETURN expr? ';'                    # returnStmt
        | WHILE expr DO statements ENDWHILE # whileStmt
          // if-then-else statement (else is optional)
        | IF expr THEN statements (ELSE statements)? ENDIF       # ifStmt
          // A function/procedure call has a list of arguments in parenthesis (possibly empty)
        | ident '(' (expr (',' expr)*)? ')' ';'                   # procCall
          // Read a variable
        | READ left_expr ';'                  # readStmt
          // Write an expression
        | WRITE expr ';'                      # writeExpr
          // Write a string
        | WRITE STRING ';'                    # writeString
        ;

// Grammar for left expressions (l-values in C++)
left_expr
        : ident                 #leftExprIdent
        | ident '[' expr ']'    #arrayAccessLExpr
        ;

// parametersCall
//         : expr (',' expr)* 
//         ;

// Grammar for expressions with boolean, relational and aritmetic operators
expr    : LPAR expr RPAR                           # parenthesis
        | ident '[' expr ']'                       # arrayAccessExpr
        | op=(MINUS|PLUS) expr                     # unaryArithmetic
        | op=NOT expr                              # unaryLogical
        | expr op=(MUL|DIV|MOD) expr               # arithmetic
        | expr op=(PLUS|MINUS) expr                # arithmetic
        | expr op=(EQUAL|NEQ|GT|LT|GE|LE) expr     # relational
        | expr op=AND expr                         # logical 
        | expr op=OR expr                          # logical
        | INTVAL                                   # value
        | FLOATVAL                                 # value
        | BOOLVAL                                  # value
        | CHARVAL                                  # value
        | ident '(' (expr (',' expr)*) ? ')'       # funcExpr
        | ident                                    # exprIdent
        ;

// Identifiers
ident   : ID
        ;

//////////////////////////////////////////////////
/// Lexer Rules
//////////////////////////////////////////////////

ASSIGN    : '=' ;
EQUAL     : '==' ;
NEQ       : '!=' ;
GE        : '>=' ;
LE        : '<=' ;
GT        : '>' ;
LT        : '<' ;
PLUS      : '+' ;
MINUS     : '-' ;
MUL       : '*';
MOD       : '%';
DIV       : '/';
LPAR      : '(' ;  
RPAR      : ')' ;  
VAR       : 'var';
INT       : 'int';
FLOAT     : 'float';
CHAR      : 'char';
BOOL      : 'bool';
AND       : 'and';
NOT       : 'not' ;  
OR        : 'or';
RETURN    : 'return';
WHILE     : 'while';
ENDWHILE  : 'endwhile';
IF        : 'if' ;
THEN      : 'then' ;
ELSE      : 'else' ;
ENDIF     : 'endif' ;
FUNC      : 'func' ;
ENDFUNC   : 'endfunc' ;
READ      : 'read' ;
WRITE     : 'write' ;
ARRAY     : 'array' ;
DO        : 'do' ;
INTVAL    : ('0'..'9')+ ;
FLOATVAL  : INTVAL '.' INTVAL ;
BOOLVAL   : ('true' | 'false') ;  
CHARVAL   :  '\'' ( ESC_SEQ | ~('\\'|'\'') ) '\'' ;
ID        : ('a'..'z'|'A'..'Z') ('a'..'z'|'A'..'Z'|'_'|'0'..'9')* ;


// Strings (in quotes) with escape sequences
STRING    : '"' ( ESC_SEQ | ~('\\'|'"') )* '"' ;

fragment
ESC_SEQ   : '\\' ('b'|'t'|'n'|'f'|'r'|'"'|'\''|'\\') ;

// Comments (inline C++-style)
COMMENT   : '//' ~('\n'|'\r')* '\r'? '\n' -> skip ;

// White spaces
WS        : (' '|'\t'|'\r'|'\n')+ -> skip ;
// Alternative description
// WS        : [ \t\r\n]+ -> skip ;
