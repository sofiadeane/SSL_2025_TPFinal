#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define NUMESTADOS 28
#define NUMCOLS 17
#define TAMLEX 33
#define TAMNOM 21

/******************Declaraciones Globales*************************/
FILE * in;

typedef enum{
    INICIO, FIN, LEER, ESCRIBIR, SI, MIENTRAS, REPETIR, HASTA,
    ID,
    CONSTANTE_INT, CONSTANTE_FLOAT, CONSTANTE_CHAR,
    PARENIZQUIERDO, PARENDERECHO, PUNTOYCOMA, COMA,
    ASIGNACION, SUMA, RESTA, IGUAL, DISTINTO, MENOR, MENORIGUAL, MAYOR, MAYORIGUAL,
    FDT, ERRORLEXICO
} TOKEN;

typedef struct{
    char identificadores[TAMLEX];
    TOKEN t;
} RegTS;

RegTS TS[1000] = {
    {"inicio", INICIO},
    {"fin", FIN},
    {"leer", LEER},
    {"escribir", ESCRIBIR},
    {"si", SI},
    {"mientras", MIENTRAS},
    {"repetir", REPETIR},
    {"hasta", HASTA},
    {"$", 99}
};

typedef enum { T_INT, T_FLOAT, T_CHAR } TIPO_CTE;

typedef struct {
    TOKEN clase;
    char nombre[TAMLEX];
    TIPO_CTE tipoDato;
    union {
        int   valorInt;
        float valorFloat;
        char  valorChar;
    } valor;
} REG_EXPRESION;

char buffer[TAMLEX];
TOKEN tokenActual;
int flagToken = 0;

/**********************Prototipos de Funciones************************/
TOKEN scanner();
int columna(int c);
int estadoFinal(int e);

void Objetivo(void);
void Programa(void);
void ListaSentencias(void);
void Sentencia(void);
void ListaIdentificadores(void);
void Identificador(REG_EXPRESION * presul);
void ListaExpresiones(void);
void Expresion(REG_EXPRESION * presul);
void ExpresionLogica(void);
void Primaria(REG_EXPRESION * presul);
void OperadorAditivo(char * presul);

REG_EXPRESION ProcesarConstante(TIPO_CTE tipoDato);
REG_EXPRESION ProcesarId(void);
char * ProcesarOp(void);

void Leer(REG_EXPRESION in);
void Escribir(REG_EXPRESION out);
REG_EXPRESION GenInfijo(REG_EXPRESION e1, char * op, REG_EXPRESION e2);
void Match(TOKEN t);
TOKEN ProximoToken();

void ErrorLexico(void);
void ErrorSintactico(void);

void Generar(char * co, char * a, char * b, char * c);
char * Extraer(REG_EXPRESION * preg);
int Buscar(char * id, RegTS * TS, TOKEN * t);
void Colocar(char * id, RegTS * TS);
void Chequear(char * s);
void Comenzar(void);
void Terminar(void);
void Asignar(REG_EXPRESION izq, REG_EXPRESION der);

/***************************Programa Principal************************/
int main(int cantArgumentosMain, char * argumentosMain[])
{
    char nomArchi[TAMNOM];
    int l;

    if ( cantArgumentosMain == 1 )
    {
        printf("Debe ingresar el nombre del archivo fuente (en lenguaje Micro) en la linea de comandos\n");
        return -1;
    }
    if ( cantArgumentosMain != 2 )
    {
        printf("Numero incorrecto de argumentos\n");
        return -1;
    }

    strcpy(nomArchi, argumentosMain[1]);
    l = (int)strlen(nomArchi);

    if ( l > TAMNOM ){
        printf("Nombre incorrecto del Archivo Fuente\n");
        return -1;
    }

    if ( l < 2 || nomArchi[l-1] != 'm' || nomArchi[l-2] != '.' ){
        printf("Nombre incorrecto del Archivo Fuente\n");
        return -1;
    }

    if ( (in = fopen(nomArchi, "r") ) == NULL ){
        printf("No se pudo abrir archivo fuente\n");
        return -1;
    }

    Objetivo();

    fclose(in);
    return 0;
}

/**********Procedimientos de Analisis Sintactico (PAS) *****************/
void Objetivo(void){
    Programa();
    Match(FDT);
    Terminar();
}

void Programa(void){
    Comenzar();
    Match(INICIO);
    ListaSentencias();
    Match(FIN);
}

void ListaSentencias(void){
    TOKEN t;
    Sentencia();
    while ( 1 ){
        t = ProximoToken();
        if (t == ID || t == LEER || t == ESCRIBIR || t == SI || t == MIENTRAS || t == REPETIR)
            Sentencia();
        else
            return;
    }
}

void Sentencia(void){
    TOKEN tok = ProximoToken();
    REG_EXPRESION izq, der;

    switch ( tok ){
        case ID :
            Identificador(&izq);
            Match(ASIGNACION);
            Expresion(&der);
            Asignar(izq, der);
            Match(PUNTOYCOMA);
            break;

        case LEER :
            Match(LEER);
            Match(PARENIZQUIERDO);
            ListaIdentificadores();
            Match(PARENDERECHO);
            Match(PUNTOYCOMA);
            break;

        case ESCRIBIR :
            Match(ESCRIBIR);
            Match(PARENIZQUIERDO);
            ListaExpresiones();
            Match(PARENDERECHO);
            Match(PUNTOYCOMA);
            break;

        case SI:
            Match(SI);
            Match(PARENIZQUIERDO);
            ExpresionLogica();
            Match(PARENDERECHO);
            ListaSentencias();
            break;

        case MIENTRAS:
            Match(MIENTRAS);
            Match(PARENIZQUIERDO);
            ExpresionLogica();
            Match(PARENDERECHO);
            ListaSentencias();
            break;

        case REPETIR:
            Match(REPETIR);
            ListaSentencias();
            Match(HASTA);
            Match(PARENIZQUIERDO);
            ExpresionLogica();
            Match(PARENDERECHO);
            Match(PUNTOYCOMA);
            break;

        default : return;
    }
}

void ListaIdentificadores(void){
    TOKEN t;
    REG_EXPRESION reg;

    Identificador(&reg);
    Leer(reg);

    for ( t = ProximoToken(); t == COMA; t = ProximoToken() ){
        Match(COMA);
        Identificador(&reg);
        Leer(reg);
    }
}

void Identificador(REG_EXPRESION * presul){
    Match(ID);
    *presul = ProcesarId();
}

void ListaExpresiones(void){
    TOKEN t;
    REG_EXPRESION reg;

    Expresion(&reg);
    Escribir(reg);

    for ( t = ProximoToken(); t == COMA; t = ProximoToken() )
    {
        Match(COMA);
        Expresion(&reg);
        Escribir(reg);
    }
}

void Expresion(REG_EXPRESION * presul){
    REG_EXPRESION operandoIzq, operandoDer;
    char operador[TAMLEX];
    TOKEN t;

    Primaria(&operandoIzq);

    for ( t = ProximoToken(); t == SUMA || t == RESTA; t = ProximoToken() ){
        OperadorAditivo(operador);
        Primaria(&operandoDer);
        operandoIzq = GenInfijo(operandoIzq, operador, operandoDer);
    }
    *presul = operandoIzq;
}

void ExpresionLogica(void) {
    REG_EXPRESION izq, der;
    Expresion(&izq);

    TOKEN operador = ProximoToken();
    char cadOp[TAMLEX];

    switch (operador) {
        case IGUAL:       strcpy(cadOp, "CMP_IGUAL");           break;
        case DISTINTO:    strcpy(cadOp, "CMP_DISTINTO");        break;
        case MENOR:       strcpy(cadOp, "CMP_MENOR");           break;
        case MENORIGUAL:  strcpy(cadOp, "CMP_MENORIGUAL");      break;
        case MAYOR:       strcpy(cadOp, "CMP_MAYOR");           break;
        case MAYORIGUAL:  strcpy(cadOp, "CMP_MAYORIGUAL");      break;
        default:          ErrorSintactico(); return;
    }

    Match(operador);
    Expresion(&der);
    Generar(cadOp, Extraer(&izq), Extraer(&der), "");
}

void Primaria(REG_EXPRESION * presul){
    TOKEN tok = ProximoToken();
    switch ( tok )
    {
        case ID :
            Identificador(presul);
            break;

        case CONSTANTE_INT:
            Match(CONSTANTE_INT);
            *presul = ProcesarConstante(T_INT);
            break;

        case CONSTANTE_FLOAT:
            Match(CONSTANTE_FLOAT);
            *presul = ProcesarConstante(T_FLOAT);
            break;

        case CONSTANTE_CHAR:
            Match(CONSTANTE_CHAR);
            *presul = ProcesarConstante(T_CHAR);
            break;

        case PARENIZQUIERDO :
            Match(PARENIZQUIERDO);
            Expresion(presul);
            Match(PARENDERECHO);
            break;

        default : return;
    }
}

void OperadorAditivo(char * presul){
    TOKEN t = ProximoToken();
    if ( t == SUMA || t == RESTA ){
        Match(t);
        strcpy(presul, ProcesarOp());
    } else
        ErrorSintactico();
}

/**********************Rutinas Semanticas******************************/
REG_EXPRESION ProcesarConstante(TIPO_CTE tipoEncontrado){
    REG_EXPRESION reg;
    strcpy(reg.nombre, buffer);
    reg.tipoDato = tipoEncontrado;
    switch (tipoEncontrado){
        case T_INT:
            reg.clase = CONSTANTE_INT;
            sscanf(buffer, "%d", &reg.valor.valorInt);
            break;
        case T_FLOAT:
            reg.clase = CONSTANTE_FLOAT;
            sscanf(buffer, "%f", &reg.valor.valorFloat);
            break;
        case T_CHAR:
            reg.clase = CONSTANTE_CHAR;
            reg.valor.valorChar = buffer[1];
            break;
        default:
            ErrorLexico();
            break;
    }
    return reg;
}

REG_EXPRESION ProcesarId(void){
    REG_EXPRESION reg;
    Chequear(buffer);
    reg.clase = ID;
    strcpy(reg.nombre, buffer);
    return reg;
}

char * ProcesarOp(void){
    return buffer;
}

void Leer(REG_EXPRESION in){
    Generar("Read", in.nombre, "Entera", "");
}

void Escribir(REG_EXPRESION out){
    Generar("Write", Extraer(&out), "Entera", "");
}

REG_EXPRESION GenInfijo(REG_EXPRESION e1, char * op, REG_EXPRESION e2){
    REG_EXPRESION reg;
    static unsigned int numTemp = 1;
    char cadTemp[TAMLEX] ="Temp&";
    char cadNum[TAMLEX];
    char cadOp[TAMLEX];

    if ( op[0] == '-' ) strcpy(cadOp, "Restar");
    if ( op[0] == '+' ) strcpy(cadOp, "Sumar");

    sprintf(cadNum, "%u", numTemp++);
    strcat(cadTemp, cadNum);

    if ( e1.clase == ID) Chequear(Extraer(&e1));
    if ( e2.clase == ID) Chequear(Extraer(&e2));

    Chequear(cadTemp);

    Generar(cadOp, Extraer(&e1), Extraer(&e2), cadTemp);
    strcpy(reg.nombre, cadTemp);
    reg.clase = ID;
    return reg;
}

/***************Funciones Auxiliares**********************************/
void Match(TOKEN t){
    if ( !(t == ProximoToken()) ) ErrorSintactico();
    flagToken = 0;
}

TOKEN ProximoToken(){
    if ( !flagToken )
    {
        tokenActual = scanner();
        if ( tokenActual == ERRORLEXICO ) ErrorLexico();
        flagToken = 1;
        if ( tokenActual == ID )
        {
            Buscar(buffer, TS, &tokenActual);
        }
    }
    return tokenActual;
}

void ErrorLexico(){
    fprintf(stderr, "Error Lexico\n");
}

void ErrorSintactico(){
    fprintf(stderr, "Error Sintactico\n");
}

void Generar(char * co, char * a, char * b, char * c){
    printf("%s %s,%s,%s\n", co, a, b, c);
}

char * Extraer(REG_EXPRESION * preg){
    return preg->nombre;
}

int Buscar(char * id, RegTS * TS, TOKEN * t){
    int i = 0;
    while ( strcmp("$", TS[i].identificadores) )
    {
        if ( !strcmp(id, TS[i].identificadores) )
        {
            *t = TS[i].t;
            return 1;
        }
        i++;
    }
    return 0;
}

void Colocar(char * id, RegTS * TS)
{
    int i = 4;
    while ( strcmp("$", TS[i].identificadores) ) i++;
    if ( i < 999 )
    {
        strcpy(TS[i].identificadores, id );
        TS[i].t = ID;
        strcpy(TS[++i].identificadores, "$" );
    }
}

void Chequear(char * s)
{
    TOKEN t;
    if ( !Buscar(s, TS, &t) )
    {
        Colocar(s, TS);
        Generar("Declara", s, "Entera", "");
    }
}

void Comenzar(void)
{
}

void Terminar(void)
{
    Generar("Detiene", "", "", "");
}

void Asignar(REG_EXPRESION izq, REG_EXPRESION der)
{
    Generar("Almacena", Extraer(&der), izq.nombre, "");
}

/**************************Scanner************************************/
TOKEN scanner() {
    static int tabla[NUMESTADOS][NUMCOLS] = {
        {  1,  3,  5,  6,  7,  8,  9, 10, 11, 13, 14, 15, 16, 17, 20,  0, 21 },
        {  1,  1,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2 },
        { 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21 },
        {  4,  3,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4, 16,  4,  4,  4,  4 },
        { 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21 },
        { 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21 },
        { 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21 },
        { 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21 },
        { 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21 },
        { 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21 },
        { 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21 },
        { 21, 21, 21, 21, 21, 21, 21, 21, 21, 12, 21, 21, 21, 21, 21, 21, 21 },
        { 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21 },
        { 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21 },
        { 23, 23, 23, 23, 23, 23, 23, 23, 23, 24, 23, 25, 23, 23, 23, 23, 23 },
        { 26, 26, 26, 26, 26, 26, 26, 26, 26, 27, 26, 26, 26, 26, 26, 26, 26 },
        { 21, 16, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21 },
        { 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18 },
        { 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 19, 21, 21, 21 },
        { 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21 },
        { 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21 },
        { 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21 },
        { 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21 },
        { 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21 },
        { 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21 },
        { 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21 },
        { 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21 }
    };

    int car, col, estado = 0, i = 0;

    do {
        car = fgetc(in);
        col = columna(car);
        estado = tabla[estado][col];

        if (col != 15 && col != 14) {
            if (i < TAMLEX - 1) buffer[i++] = (char)car;
        }
    } while (!estadoFinal(estado));

    buffer[i] = '\0';

    switch (estado) {
        case 2:
            if (col != 15 && col != 14) { ungetc(car, in); buffer[i-1] = '\0'; }
            return ID;

        case 4:
            if (col != 15 && col != 14) { ungetc(car, in); buffer[i-1] = '\0'; }
            return CONSTANTE_INT;

        case 5:  return SUMA;
        case 6:  return RESTA;
        case 7:  return PARENIZQUIERDO;
        case 8:  return PARENDERECHO;
        case 9:  return COMA;
        case 10: return PUNTOYCOMA;

        case 12: return ASIGNACION;
        case 13: return IGUAL;

        case 16:
            if (buffer[0] == '.' && buffer[1] == '\0') return ERRORLEXICO;
            return CONSTANTE_FLOAT;

        case 19: return CONSTANTE_CHAR;
        case 20: return FDT;

        case 23: return MENOR;
        case 24: return MENORIGUAL;
        case 25: return DISTINTO;
        case 26: return MAYOR;
        case 27: return MAYORIGUAL;

        case 21:
        default:
            return ERRORLEXICO;
    }
}

int estadoFinal(int e) {
    return !(e == 0 || e == 1 || e == 3 || e == 11 || e == 14 || e == 15 || e == 17 || e == 18);
}

int columna(int c) {
    if (isalpha(c))          return 0;
    if (isdigit(c))          return 1;
    if (c == '+')            return 2;
    if (c == '-')            return 3;
    if (c == '(')            return 4;
    if (c == ')')            return 5;
    if (c == ',')            return 6;
    if (c == ';')            return 7;
    if (c == ':')            return 8;
    if (c == '=')            return 9;
    if (c == '<')            return 10;
    if (c == '>')            return 11;
    if (c == '.')            return 12;
    if (c == '\'')           return 13;
    if (c == EOF)            return 14;
    if (isspace(c))          return 15;
    return 16;
}
/*************Fin Scanner**********************************************/
