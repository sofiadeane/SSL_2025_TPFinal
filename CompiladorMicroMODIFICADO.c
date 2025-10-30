#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define NUMESTADOS 30
#define NUMCOLS 17
#define TAMLEX 33
#define TAMNOM 21

/******************Declaraciones Globales*************************/
FILE * in;

typedef enum { T_INT, T_FLOAT, T_CHAR } TIPO_CTE;

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
    TIPO_CTE tipo;  // Nuevo campo para el tipo de dato
} RegTS;

// Tabla de símbolos inicial con palabras reservadas
// Tabla de símbolos actualizada
RegTS TS[1000] = {
    {"inicio", INICIO, T_INT},
    {"fin", FIN, T_INT},
    {"leer", LEER, T_INT},
    {"escribir", ESCRIBIR, T_INT},
    {"si", SI, T_INT},
    {"sino", ID, T_INT},
    {"entonces", ID, T_INT},    // Palabra clave para SI
    {"hacer", ID, T_INT},       // Palabra clave para MIENTRAS  
    {"mientras", MIENTRAS, T_INT},
    {"repetir", REPETIR, T_INT},
    {"hasta", HASTA, T_INT},
    {"$", FDT, T_INT}
};

int numTS = 12; // Número actual de elementos en la TS

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

char* ObtenerTipoStr(TIPO_CTE tipo);
int BuscarConTipo(char * id, RegTS * TS, TOKEN * t, TIPO_CTE *tipo);
TIPO_CTE TipoResultante(TIPO_CTE t1, TIPO_CTE t2);
void GenerarConversion(REG_EXPRESION *origen, TIPO_CTE tipoDestino, char *tempDestino);
void ColocarConTipo(char * id, RegTS * TS, TIPO_CTE tipo);
void ChequearConTipo(char * s, TIPO_CTE tipo);
void ActualizarTipoVariable(char *id, TIPO_CTE nuevoTipo);

void ExpresionLogicaCondicional(char* etiquetaFalso);
void ExpresionLogicaCondicionalInversa(char* etiquetaVerdadero);

void ProcesarSI(void);
void ProcesarMIENTRAS(void);
void ProcesarREPETIR(void);

REG_EXPRESION ProcesarConstante(TIPO_CTE tipoDato);
REG_EXPRESION ProcesarId(void);
char * ProcesarOp(void);

void ProcesarBloque(void);

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
    
    while (1) {
        TOKEN t = ProximoToken();
        
        if (t == FIN) {
            flagToken = 1;
            return;
        }
        
        if (t == ERRORLEXICO) {
            return;
        }
        
        // IGNORAR 'sino' - es parte de la estructura SI
        if (t == ID && strcmp(buffer, "sino") == 0) {
            continue;
        }
        
        switch (t) {
            case ID:
            case LEER:
            case ESCRIBIR:
            case SI:
            case MIENTRAS:
            case REPETIR:
                Sentencia();
                break;
            default:
                flagToken = 1;
                return;
        }
    }
}

void Sentencia(void){
    TOKEN token = tokenActual;
    
    switch ( token ){
        case ID : {
            REG_EXPRESION izq, der;
            Identificador(&izq);
            Match(ASIGNACION);
            Expresion(&der);
            Asignar(izq, der);
            Match(PUNTOYCOMA);
            break;
        }
        case LEER : {
            Match(LEER);
            Match(PARENIZQUIERDO);
            ListaIdentificadores();
            Match(PARENDERECHO);
            Match(PUNTOYCOMA);
            break;
        }
        case ESCRIBIR :
            Match(ESCRIBIR);
            Match(PARENIZQUIERDO);
            ListaExpresiones();
            Match(PARENDERECHO);
            Match(PUNTOYCOMA);
            break;

        case SI:
            ProcesarSI();
            break;

        case MIENTRAS:
            ProcesarMIENTRAS();
            break;

        case REPETIR:
            ProcesarREPETIR();
            break;

        default : 
            flagToken = 1;
            return;
    }
}

void ListaIdentificadores(void){
    TOKEN t;
    REG_EXPRESION reg;

    Identificador(&reg);
    // Asegurarnos de que la variable existe antes de leer
    Chequear(reg.nombre);
    Leer(reg);

    for ( t = ProximoToken(); t == COMA; t = ProximoToken() ){
        Match(COMA);
        Identificador(&reg);
        Chequear(reg.nombre);
        Leer(reg);
    }
}

void Identificador(REG_EXPRESION * presul){
    Match(ID);
    *presul = ProcesarId();
    // No llamar a Chequear aquí - la declaración se hará en la asignación
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

    t = ProximoToken();
    while (t == SUMA || t == RESTA) {
        OperadorAditivo(operador);
        Primaria(&operandoDer);
        operandoIzq = GenInfijo(operandoIzq, operador, operandoDer);
        t = ProximoToken();
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
        default:          
            fprintf(stderr, "Error: Operador logico esperado pero se encontro %d\n", operador);
            ErrorSintactico(); 
            return;
    }

    Match(operador);
    Expresion(&der);
    
    // Para ExpresionLogica normal, generar comparación sin salto
    char tempResult[TAMLEX];
    static int tempCounter = 0;
    sprintf(tempResult, "TempLogica%d", tempCounter++);
    
    Generar(cadOp, Extraer(&izq), Extraer(&der), tempResult);
    Generar("Declara", tempResult, "Entera", "");
}

void Primaria(REG_EXPRESION * presul){
    TOKEN token = ProximoToken();
    
    switch ( token )
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

        default : 
            fprintf(stderr, "Error en Primaria: Token inesperado %d (buffer: '%s')\n", token, buffer);
            ErrorSintactico();
            break;
    }
}

void OperadorAditivo(char * presul){
    TOKEN t = ProximoToken();
    if ( t == SUMA ) {
        Match(SUMA);
        strcpy(presul, "+");
    } else if ( t == RESTA ) {
        Match(RESTA);
        strcpy(presul, "-");
    } else {
        fprintf(stderr, "Error en OperadorAditivo: Se esperaba + o - pero se encontro %d\n", t);
        ErrorSintactico();
    }
}

/**********************Estructuras de Control Finales******************************/

void ProcesarSI(void) {
    static int contadorSI = 0;
    int etiquetaSI = contadorSI++;
    char etiquetaFalso[TAMLEX], etiquetaFin[TAMLEX];
    
    sprintf(etiquetaFalso, "SI_FALSO_%d", etiquetaSI);
    sprintf(etiquetaFin, "SI_FIN_%d", etiquetaSI);
    
    Match(SI);
    Match(PARENIZQUIERDO);
    ExpresionLogicaCondicional(etiquetaFalso);
    Match(PARENDERECHO);
    
    // Bloque verdadero (múltiples sentencias)
    ProcesarBloque();
    
    // Salto al final para evitar el bloque sino
    Generar("Salto", etiquetaFin, "", "");
    
    // Bloque falso (sino)
    Generar("Etiqueta", etiquetaFalso, "", "");
    
    // Verificar si hay sino
    if (ProximoToken() == ID && strcmp(buffer, "sino") == 0) {
        Match(ID);
        ProcesarBloque(); // Bloque sino (múltiples sentencias)
    } else {
        flagToken = 1; // Mantener el token si no hay sino
    }
    
    Generar("Etiqueta", etiquetaFin, "", "");
}

void ProcesarMIENTRAS(void) {
    static int contadorMIENTRAS = 0;
    int etiquetaMIENTRAS = contadorMIENTRAS++;
    char etiquetaInicio[TAMLEX], etiquetaFin[TAMLEX];
    
    sprintf(etiquetaInicio, "MIENTRAS_%d", etiquetaMIENTRAS);
    sprintf(etiquetaFin, "FIN_M_%d", etiquetaMIENTRAS);
    
    Match(MIENTRAS);
    Match(PARENIZQUIERDO);
    
    Generar("Etiqueta", etiquetaInicio, "", "");
    ExpresionLogicaCondicional(etiquetaFin);
    Match(PARENDERECHO);
    
    // Cuerpo del ciclo (múltiples sentencias)
    ProcesarBloque();
    
    Generar("Salto", etiquetaInicio, "", "");
    Generar("Etiqueta", etiquetaFin, "", "");
}

void ProcesarREPETIR(void) {
    static int contadorREPETIR = 0;
    int etiquetaREPETIR = contadorREPETIR++;
    char etiquetaInicio[TAMLEX];
    
    sprintf(etiquetaInicio, "REPETIR_%d", etiquetaREPETIR);
    
    Match(REPETIR);
    
    // Etiqueta de inicio del ciclo
    Generar("Etiqueta", etiquetaInicio, "", "");
    
    // Cuerpo del ciclo (múltiples sentencias)
    ProcesarBloque();
    
    Match(HASTA);
    Match(PARENIZQUIERDO);
    ExpresionLogicaCondicionalInversa(etiquetaInicio);
    Match(PARENDERECHO);
    Match(PUNTOYCOMA);
}

/**********************Función para Procesar Múltiples Sentencias******************************/

void ProcesarBloque(void) {
    
    while (1) {
        TOKEN t = ProximoToken();
        
        // Palabras clave que indican el fin del bloque
        if (t == FIN || 
            t == MIENTRAS || t == REPETIR || t == SI ||
            (t == ID && (strcmp(buffer, "sino") == 0 || strcmp(buffer, "hasta") == 0))) {
            flagToken = 1; // Mantener el token para la estructura padre
            return;
        }
        
        // Si no hay más tokens válidos, terminar
        if (t == ERRORLEXICO || t == FDT) {
            return;
        }
        
        // Si no es una sentencia válida, terminar
        if (!(t == ID || t == LEER || t == ESCRIBIR || t == SI || t == MIENTRAS || t == REPETIR)) {
            flagToken = 1;
            return;
        }
        
        // Procesar la sentencia actual
        Sentencia();
    }
}

/**********************Rutinas Semanticas******************************/
// Función auxiliar para obtener el nombre del tipo
char* ObtenerTipoStr(TIPO_CTE tipo) {
    switch(tipo) {
        case T_INT: return "Entera";
        case T_FLOAT: return "Real";
        case T_CHAR: return "Caracter";
        default: return "Entera";
    }
}

// Función para buscar variable en TS y devolver su tipo
int BuscarConTipo(char * id, RegTS * TS, TOKEN * t, TIPO_CTE *tipo) {
    int i = 0;
    while ( i < numTS ) {
        if ( strcmp(id, TS[i].identificadores) == 0 ) {
            *t = TS[i].t;
            *tipo = TS[i].tipo;
            return 1;
        }
        i++;
    }
    return 0;
}

// Función para determinar el tipo resultante de una operación
TIPO_CTE TipoResultante(TIPO_CTE t1, TIPO_CTE t2) {
    // Jerarquía de tipos: float > int > char
    if (t1 == T_FLOAT || t2 == T_FLOAT) return T_FLOAT;
    if (t1 == T_INT || t2 == T_INT) return T_INT;
    return T_CHAR;
}

// Función para generar conversión de tipo si es necesario
void GenerarConversion(REG_EXPRESION *origen, TIPO_CTE tipoDestino, char *tempDestino) {
    if (origen->tipoDato != tipoDestino) {
        char convOp[TAMLEX];
        switch(tipoDestino) {
            case T_FLOAT:
                strcpy(convOp, "ConvertirAReal");
                break;
            case T_INT:
                strcpy(convOp, "ConvertirAEntero");
                break;
            case T_CHAR:
                strcpy(convOp, "ConvertirACaracter");
                break;
        }
        Generar(convOp, Extraer(origen), tempDestino, "");
        strcpy(origen->nombre, tempDestino);
        origen->tipoDato = tipoDestino;
    }
}

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
            if (strlen(buffer) >= 3 && buffer[0] == '\'' && buffer[strlen(buffer)-1] == '\'') {
                reg.valor.valorChar = buffer[1];
                sprintf(reg.nombre, "%c", buffer[1]);
            } else {
                reg.valor.valorChar = buffer[0];
                sprintf(reg.nombre, "%c", buffer[0]);
            }
            break;
        default:
            ErrorLexico();
            break;
    }
    return reg;
}

REG_EXPRESION ProcesarId(void){
    REG_EXPRESION reg;
    TOKEN t;
    TIPO_CTE tipo;
    
    if (BuscarConTipo(buffer, TS, &t, &tipo)) {
        // Variable ya existe, usar su tipo
        reg.tipoDato = tipo;
    } else {
        // Variable nueva, tipo por defecto será determinado en asignación
        reg.tipoDato = T_INT; // Temporal, se actualizará
    }
    
    // No llamar a Chequear aquí - se hará en la asignación
    reg.clase = ID;
    strcpy(reg.nombre, buffer);
    return reg;
}

char * ProcesarOp(void){
    static char op[2];
    op[0] = buffer[0];
    op[1] = '\0';
    return op;
}

void Leer(REG_EXPRESION in){
    TOKEN t;
    TIPO_CTE tipo;
    if (BuscarConTipo(in.nombre, TS, &t, &tipo)) {
        Generar("Read", in.nombre, ObtenerTipoStr(tipo), "");
    } else {
        Generar("Read", in.nombre, "Entera", "");
    }
}

void Escribir(REG_EXPRESION out){
    Generar("Write", Extraer(&out), ObtenerTipoStr(out.tipoDato), "");
}

// Modificar GenInfijo para no declarar variables temporalmente
REG_EXPRESION GenInfijo(REG_EXPRESION e1, char * op, REG_EXPRESION e2){
    REG_EXPRESION reg;
    static unsigned int numTemp = 1;
    char cadTemp[TAMLEX] ="Temp";
    char cadNum[TAMLEX];
    char cadOp[TAMLEX];

    // Determinar tipo resultante
    TIPO_CTE tipoRes = TipoResultante(e1.tipoDato, e2.tipoDato);
    
    // Generar conversiones si son necesarias
    if (e1.tipoDato != tipoRes) {
        char temp1[TAMLEX];
        sprintf(temp1, "TempConv%d", numTemp++);
        GenerarConversion(&e1, tipoRes, temp1);
    }
    
    if (e2.tipoDato != tipoRes) {
        char temp2[TAMLEX];
        sprintf(temp2, "TempConv%d", numTemp++);
        GenerarConversion(&e2, tipoRes, temp2);
    }

    if ( op[0] == '-' ) 
        strcpy(cadOp, "Restar");
    else if ( op[0] == '+' ) 
        strcpy(cadOp, "Sumar");
    else
        strcpy(cadOp, "Operar");

    sprintf(cadNum, "%u", numTemp++);
    strcat(cadTemp, cadNum);

    // Declarar la temporal con el tipo correcto
    ColocarConTipo(cadTemp, TS, tipoRes);
    Generar("Declara", cadTemp, ObtenerTipoStr(tipoRes), "");

    Generar(cadOp, Extraer(&e1), Extraer(&e2), cadTemp);
    strcpy(reg.nombre, cadTemp);
    reg.clase = ID;
    reg.tipoDato = tipoRes;
    return reg;
}

/**********************Funciones Auxiliares para Condiciones*****************/

void ExpresionLogicaCondicional(char* etiquetaSalto) {
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
        default:          
            fprintf(stderr, "Error: Operador logico esperado pero se encontro %d\n", operador);
            ErrorSintactico(); 
            return;
    }

    Match(operador);
    Expresion(&der);
    
    // Generar salto condicional si la condición es FALSA
    // Formato: CMP_OPERADOR op1,op2,etiqueta_si_falso
    Generar(cadOp, Extraer(&izq), Extraer(&der), etiquetaSalto);
}

void ExpresionLogicaCondicionalInversa(char* etiquetaSalto) {
    REG_EXPRESION izq, der;
    Expresion(&izq);

    TOKEN operador = ProximoToken();
    char cadOp[TAMLEX];

    // Para REPETIR HASTA: saltar si la condición es FALSA
    switch (operador) {
        case IGUAL:       strcpy(cadOp, "CMP_DISTINTO");        break;  // Salta si NO son iguales
        case DISTINTO:    strcpy(cadOp, "CMP_IGUAL");           break;  // Salta si son iguales
        case MENOR:       strcpy(cadOp, "CMP_MAYORIGUAL");      break;  // Salta si es mayor o igual
        case MENORIGUAL:  strcpy(cadOp, "CMP_MAYOR");           break;  // Salta si es mayor
        case MAYOR:       strcpy(cadOp, "CMP_MENORIGUAL");      break;  // Salta si es menor o igual
        case MAYORIGUAL:  strcpy(cadOp, "CMP_MENOR");           break;  // Salta si es menor
        default:          
            fprintf(stderr, "Error: Operador logico esperado pero se encontro %d\n", operador);
            ErrorSintactico(); 
            return;
    }

    Match(operador);
    Expresion(&der);
    
    // Generar salto condicional si la condición es FALSA
    Generar(cadOp, Extraer(&izq), Extraer(&der), etiquetaSalto);
}

/***************Funciones Auxiliares**********************************/
// Función para actualizar el tipo de una variable en la TS
void ActualizarTipoVariable(char *id, TIPO_CTE nuevoTipo) {
    for (int i = 0; i < numTS; i++) {
        if (strcmp(TS[i].identificadores, id) == 0 && TS[i].t == ID) {
            TS[i].tipo = nuevoTipo;
            // Regenerar la declaración con el tipo correcto
            Generar("Declara", id, ObtenerTipoStr(nuevoTipo), "");
            return;
        }
    }
}

void Match(TOKEN t){
    TOKEN prox = ProximoToken();
    if ( t != prox ) {
        fprintf(stderr, "Error Sintactico: Se esperaba token %d pero se encontro %d (buffer: '%s')\n", t, prox, buffer);
        ErrorSintactico();
    }
    flagToken = 0;
}

TOKEN ProximoToken(){
    if ( !flagToken )
    {
        tokenActual = scanner();
        if ( tokenActual == ERRORLEXICO ) {
            fprintf(stderr, "Error Lexico en buffer: '%s'\n", buffer);
            ErrorLexico();
        }
        flagToken = 1;
        
        // Si es un ID, verificar si es palabra reservada
        if ( tokenActual == ID )
        {
            TOKEN t;
            if (Buscar(buffer, TS, &t)) {
                tokenActual = t; // Es palabra reservada
            }
        }
    }
    return tokenActual;
}

void ErrorLexico(){
    fprintf(stderr, "Error Lexico en buffer: '%s'\n", buffer);
    fprintf(stderr, "Posicion actual en el archivo:\n");
    
    // Mostrar contexto alrededor del error
    long pos = ftell(in);
    fseek(in, -50, SEEK_CUR); // Retroceder 50 caracteres
    char contexto[100];
    int leidos = fread(contexto, 1, 99, in);
    contexto[leidos] = '\0';
    fprintf(stderr, "Contexto: ...%s\n", contexto);
    fseek(in, pos, SEEK_SET); // Volver a la posición original
    
    exit(1);
}

void ErrorSintactico(){
    fprintf(stderr, "Error Sintactico\n");
    exit(1);
}

void Generar(char * co, char * a, char * b, char * c){
    printf("%s %s,%s,%s\n", co, a, b, c);
}

char * Extraer(REG_EXPRESION * preg){
    return preg->nombre;
}

int Buscar(char * id, RegTS * TS, TOKEN * t){
    int i = 0;
    while ( i < numTS )
    {
        if ( strcmp(id, TS[i].identificadores) == 0 )
        {
            *t = TS[i].t;
            return 1;
        }
        i++;
    }
    return 0;
}

void ColocarConTipo(char * id, RegTS * TS, TIPO_CTE tipo)
{
    if (numTS < 999) {
        strcpy(TS[numTS].identificadores, id);
        TS[numTS].t = ID;
        TS[numTS].tipo = tipo;
        numTS++;
        strcpy(TS[numTS].identificadores, "$");
        TS[numTS].tipo = T_INT;
    }
}

void ChequearConTipo(char * s, TIPO_CTE tipo) {
    TOKEN t;
    TIPO_CTE tipoExistente;
    if ( !BuscarConTipo(s, TS, &t, &tipoExistente) ) {
        ColocarConTipo(s, TS, tipo);
        Generar("Declara", s, ObtenerTipoStr(tipo), "");
    }
}

void Colocar(char * id, RegTS * TS)
{
    if (numTS < 999)
    {
        strcpy(TS[numTS].identificadores, id);
        TS[numTS].t = ID;
        numTS++;
        // Actualizar marcador de fin
        strcpy(TS[numTS].identificadores, "$");
    }
}

void Chequear(char * s) {
    // Esta función ahora es más simple - solo para uso interno
    TOKEN t;
    TIPO_CTE tipo;
    if ( !BuscarConTipo(s, TS, &t, &tipo) ) {
        // Por defecto, declarar como entero (se actualizará si es necesario)
        ColocarConTipo(s, TS, T_INT);
        Generar("Declara", s, "Entera", "");
    }
}

void Comenzar(void)
{
    printf("Codigo generado para programa Micro\n");
}

void Terminar(void)
{
    Generar("Detiene", "", "", "");
}

void Asignar(REG_EXPRESION izq, REG_EXPRESION der) {
    // Si la variable ya existe, usar su tipo actual
    TOKEN t;
    TIPO_CTE tipoActual;
    TIPO_CTE tipoDestino;
    
    if (BuscarConTipo(izq.nombre, TS, &t, &tipoActual)) {
        tipoDestino = tipoActual;
    } else {
        // Variable nueva, usar el tipo del valor asignado
        tipoDestino = der.tipoDato;
        ColocarConTipo(izq.nombre, TS, tipoDestino);
        Generar("Declara", izq.nombre, ObtenerTipoStr(tipoDestino), "");
    }
    
    // Si el tipo destino es diferente del tipo de la expresión, actualizar la variable
    if (tipoDestino != der.tipoDato) {
        ActualizarTipoVariable(izq.nombre, der.tipoDato);
        tipoDestino = der.tipoDato;
    }
    
    // Generar conversión si es necesario (ahora convierte al tipo de destino)
    if (der.tipoDato != tipoDestino) {
        char tempConv[TAMLEX];
        static int convCounter = 1;
        sprintf(tempConv, "TempConv%d", convCounter++);
        GenerarConversion(&der, tipoDestino, tempConv);
    }
    
    Generar("Almacena", Extraer(&der), izq.nombre, "");
}

/***************Funciones del Scanner**********************************/

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

int estadoFinal(int e) {
    return (e == 2 || e == 4 || e == 5 || e == 6 || e == 7 || e == 8 || 
            e == 9 || e == 10 || e == 12 || e == 13 || e == 16 || e == 19 || 
            e == 20 || e == 22 || e == 23 || e == 24 || e == 25 || e == 26 || 
            e == 27 || e == 29 || e == 21);
}

/**************************Scanner************************************/
TOKEN scanner() {
    static int tabla[NUMESTADOS][NUMCOLS] = {
        //  a    d    +    -    (    )    ,    ;    :    =    <    >    .    '   EOF  esp  ot
        {  1,  3,  5,  6,  7,  8,  9, 10, 11, 13, 14, 15, 16, 17, 20,  0, 21 }, // 0 - estado inicial
        {  1,  1,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2 }, // 1 - identificador
        { 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21 }, // 2 - identificador final
        {  4,  3,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4, 28,  4,  4,  4,  4 }, // 3 - entero
        { 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21 }, // 4 - entero final
        { 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21 }, // 5 - +
        { 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21 }, // 6 - -
        { 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21 }, // 7 - (
        { 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21 }, // 8 - )
        { 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21 }, // 9 - ,
        { 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21 }, // 10 - ;
        { 21, 21, 21, 21, 21, 21, 21, 21, 21, 12, 21, 21, 21, 21, 21, 21, 21 }, // 11 - :
        { 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21 }, // 12 - :=
        { 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21 }, // 13 - =
        { 23, 23, 23, 23, 23, 23, 23, 23, 23, 24, 23, 25, 23, 23, 23, 23, 23 }, // 14 - <
        { 26, 26, 26, 26, 26, 26, 26, 26, 26, 27, 26, 26, 26, 26, 26, 26, 26 }, // 15 - >
        { 21, 22, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21 }, // 16 - . (inicio float)
        { 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 19, 18, 18, 18 }, // 17 - ' (inicio char)
        { 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 19, 18, 18, 18 }, // 18 - char (continuación)
        { 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21 }, // 19 - char final
        { 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21 }, // 20 - FDT
        { 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21 }, // 21 - error
        { 21, 22, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21 }, // 22 - float (parte decimal)
        { 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21 }, // 23 - < final
        { 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21 }, // 24 - <=
        { 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21 }, // 25 - <>
        { 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21 }, // 26 - > final
        { 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21 }, // 27 - >=
        { 21, 29, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21 }, // 28 - . después de entero
        { 21, 29, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30 }  // 29 - float con parte decimal
    };

    int car, col, estado = 0, i = 0;
    int carAnterior = 0;
    
    // Limpiar buffer
    buffer[0] = '\0';

    do {
        car = fgetc(in);
        
        if (car == EOF) {
            col = 14;
        } else {
            col = columna(car);
        }
        
        estado = tabla[estado][col];

        // Agregar al buffer si no es espacio y no es estado inicial/error
        if (estado != 0 && estado != 21 && car != EOF) {
            if (i < TAMLEX - 1) {
                buffer[i++] = (char)car;
                buffer[i] = '\0';
            }
        }
        
        carAnterior = car;
        
    } while (!estadoFinal(estado));

    // Lógica de retroceso
    if (carAnterior != EOF && estado != 20 && estado != 21) {
        int necesitaRetroceso = 1;
        
        // Estados donde NO retrocedemos:
        if (estado == 12 || estado == 24 || estado == 25 || estado == 27 || // Operadores de 2 chars
            estado == 5 || estado == 6 || estado == 7 || estado == 8 ||     // Tokens de 1 char
            estado == 9 || estado == 10 || estado == 13 || estado == 16 ||
            estado == 23 || estado == 26 || estado == 19 ||                 // Caracter final
            estado == 29) {                                                // Float completo
            necesitaRetroceso = 0;
        }
        
        if (necesitaRetroceso) {
            ungetc(carAnterior, in);
            if (i > 0) {
                buffer[i-1] = '\0';
            }
        }
    }

    //printf("DEBUG Scanner Final: estado=%d, buffer='%s'\n", estado, buffer);

    switch (estado) {
        case 2:  return ID;
        case 4:  return CONSTANTE_INT;
        case 5:  return SUMA;
        case 6:  return RESTA;
        case 7:  return PARENIZQUIERDO;
        case 8:  return PARENDERECHO;
        case 9:  return COMA;
        case 10: return PUNTOYCOMA;
        case 12: return ASIGNACION;  // :=
        case 13: return IGUAL;       // =
        case 16: return ERRORLEXICO; // Solo punto
        case 22: return CONSTANTE_FLOAT;
        case 19: return CONSTANTE_CHAR;
        case 20: return FDT;
        case 23: return MENOR;
        case 24: return MENORIGUAL;
        case 25: return DISTINTO;
        case 26: return MAYOR;
        case 27: return MAYORIGUAL;
        case 29: return CONSTANTE_FLOAT; // Float completo
        case 21:
        default: return ERRORLEXICO;
    }
}