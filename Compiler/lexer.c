#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

// Hexa-Wert, der eine der Funktionen repräsentiert
typedef enum T_Fx
{
  ifl   = 0x0,    //ifl  = Integer_Funktion_Lesen   = 0
  ifb   = 0x10,   //ifb  = Integer_Funktion_Beenden = 1
  ifgl  = 0x20,   //ifgl = Integer_Funktion_Großschrieben_Lesen = 2
  ifsl  = 0x30,   //ifsl = Integer_Funktion_Schreiben_Lesen = 3
  ifslb = 0x40    //ifslb= Integer_Funktion_Schreiben_Lesen_Beenden = 4
}tFx;

//Morphemcodes
typedef enum T_MC
{
  mcEmpty, // leer
  mcSymb,  // Symbol
  mcNumb,  // Zahl
  mcIdent  // Bezeichner, Variable, Identifier
}tMC;


// Kategorieneinteilung / Schlüsselwörter rausbekommen
typedef enum T_ZS
{
  zNIL,     // NIL
  zErg=128, // Zuweisung
  zle,      // lower equal <=
  zge,      // greater equal >=
  zBGN,zCLL,zCST,zDO,zEND,zELS,zIF,zODD,zPRC, zPUT,zTHN,zVAR,zWHL // Schlüsselwörter
}tZS;

// Struktur für ein Morphem
typedef struct morph
{
	tMC MC;			  // Morphemcode
	int PosLine;	// Zeile
	int PosCol;		// Spalte
	int MLen;		  // Länge
	union VAL 		// Inhalt -> union, nur eines der folgenden ist belegt
	{
		long Numb;	// Zahl
		char *pStr; // String
		int  Symb;	// Symbol
	}Val;
}tMorph;


int initLex(char* fname);
tMorph* Lex(void);


// Zeichenklassenvektor
// repräsentiert ASCII-Tabelle -> teilt in Klassen ein
// Folgende Klassen gibt es:
// 0 Sonderzeichen (alle oder nur erlaubte) 
// 1 Ziffern
// 2 Buchstaben
// 3 :
// 4 =
// 5 <
// 6 >
// 7 Sonstige Steuerzeichen
static char vZKl[128]=
/*      0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F     */
/*---------------------------------------------------------*/
 /* 0*/{7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,/* 0*/
 /*10*/ 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,/*10*/
 /*20*/ 7, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,/*20*/
 /*30*/ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 3, 0, 5, 4, 6, 0,/*30*/
 /*40*/ 0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,/*40*/
 /*50*/ 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0, 0, 0, 0, 0,/*50*/
 /*60*/ 0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,/*60*/
 /*70*/ 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0, 0, 0, 0, 0}/*70*/;

//Schalt- und Ausgabefunktionen des Automaten
//bedeutung siehe oben -->T_Fx
static void fl  (void); // lesen
static void fb  (void); // beenden
static void fgl (void); // Großbuchstaben schreiben und lesen -> Case Insensitve Reading
static void fsl (void); // schreiben und lesen
static void fslb(void); // schreiben und lesen und beenden

typedef void (*FX)(void); // Typedef für Funktionspointer (FX)

static FX vfx[]={fl,fb,fgl,fsl,fslb}; // Array der Funktionen

// Automatentabelle
// Beginn ist Z0 -> Je nach Zeichen nächster Zustand + jeweilige Funktion
static char vSMatrix[][8]=
/*        So      Zi      Bu      ':'      '='    '<' 	  '>'   Space */
/*--------------------------------------------------------------------*/
/* Z0 */{0+ifslb,1+ifsl ,2+ifgl ,3+ifsl ,0+ifslb,4+ifsl ,5+ifsl ,0+ifl ,
/* Z1 */ 0+ifb  ,1+ifsl ,0+ifb  ,0+ifb  ,0+ifb  ,0+ifb  ,0+ifb  ,0+ifb ,
/* Z2 */ 0+ifb  ,2+ifsl ,2+ifgl ,0+ifb  ,0+ifb  ,0+ifb  ,0+ifb  ,0+ifb ,
/* Z3 */ 0+ifb  ,0+ifb  ,0+ifb  ,0+ifb  ,6+ifsl ,0+ifb  ,0+ifb  ,0+ifb ,
/* Z4 */ 0+ifb  ,0+ifb  ,0+ifb  ,0+ifb  ,7+ifsl ,0+ifb  ,0+ifb  ,0+ifb ,
/* Z5 */ 0+ifb  ,0+ifb  ,0+ifb  ,0+ifb  ,8+ifsl ,0+ifb  ,0+ifb  ,0+ifb ,
/* Z6 */ 0+ifb  ,0+ifb  ,0+ifb  ,0+ifb  ,0+ifb  ,0+ifb  ,0+ifb  ,0+ifb ,
/* Z7 */ 0+ifb  ,0+ifb  ,0+ifb  ,0+ifb  ,0+ifb  ,0+ifb  ,0+ifb  ,0+ifb ,
/* Z8 */ 0+ifb  ,0+ifb  ,0+ifb  ,0+ifb  ,0+ifb  ,0+ifb  ,0+ifb  ,0+ifb 
};

static FILE  *pIF;			    // FILE-Stream
static tMorph MorphInit;    // Leere Struktur Morphem zur Initialisierung
static tMorph Morph;		    // aktuelles Morphem
static int    X;			      // aktuelles Eingabezeichen					
static int    Z;			      // Zustand des Automaten

static char   vBuf[1024+1]; // Puffer zur Morphembildung							
static char*  pBuf;         // Zeiger in den Puffer vBuf
static int    line,col;     // Morphemposition in Zeile und Spalte
static int    Ende;			    // Flag über das Beenden

// Struktur, um keyWord und internen Code zu übersetzen
typedef struct {
  char* pKeyWord;
  tZS KWCode;
}tKeyWordTab;

tKeyWordTab mSclW['Z'-'A'+1][2]=
{
/*j: 0 1
*/
/* A */ {{0L ,zNIL},{0L, zNIL}},
/* B */ {{"EGIN",zBGN},{0L,zNIL}},
/* C */ {{"ALL",zCLL},{"ONST",zCST}},
/* D */ {{"O",zDO },{0L, zNIL}},
/* E */ {{"ND",zEND},{"LSE", zELS}},
/* F */ {{0L ,zNIL},{0L, zNIL}},
/* G */ {{0L, zNIL},{0L, zNIL}},
/* H */ {{0L,zNIL},{0L, zNIL}},
/* I */ {{"F",zIF },{0L, zNIL}},
/* J */ {{0L, zNIL},{0L, zNIL}},
/* K */ {{0L, zNIL},{0L, zNIL}},
/* L */ {{0L, zNIL},{0L, zNIL}},
/* M */ {{0L, zNIL},{0L, zNIL}},
/* N */ {{0L, zNIL},{0L, zNIL}},
/* O */ {{"DD",zODD},{0L, zNIL}},
/* P */ {{"ROCEDURE",zPRC},{"UT", zPUT}},
/* Q */ {{0L, zNIL},{0L, zNIL}},
/* R */ {{0L, zNIL},{0L, zNIL}},
/* S */ {{0L, zNIL},{0L, zNIL}},
/* T */ {{"HEN",zTHN},{ 0L, zNIL}},
/* U */ {{0L, zNIL},{0L, zNIL}},
/* V */ {{"AR",zVAR},{0L, zNIL}},
/* W */ {{"HILE",zWHL},{0L,zNIL}},
/* X */ {{0L, zNIL},{0L, zNIL}},
/* Y */ {{0L, zNIL},{0L, zNIL}},
/* Z */ {{0L, zNIL},{0L, zNIL}}
};

//---- Initialisierung der lexiaklischen Analyse ----
int initLex(char* fname)
{
  char vName[128+1];

  strcpy(vName,fname);
  if (strstr(vName,".pl0")== NULL) 
  {
	  strcat(vName,".pl0");
  }

  pIF=fopen(vName,"r+t");
  if (pIF!=NULL) 
  {
	  X=fgetc(pIF);  //Lesen der 1. Zeichens
    return 0;  //OK zu 0 geaendert
  }  
  return -1;   //FAIL zu -1 geaendert
}

// Rückgabewert wird nicht verwendet
tMorph* Lex(void)
{
  int zx;				       // Folgezustand
  Morph = MorphInit;	 // Neues Morphem initialisieren
  Morph.PosLine = line;
  Morph.PosCol = col;
  pBuf = vBuf;			   // Pointer auf vBuf setzen
  Ende = 0;				     // Flag, um do-while zu beenden
  do
  {
    zx = vSMatrix[Z][vZKl[X&0x7f]]&0xF;			// Folgezustand aus Matrix
    											// Dabei wird zuerst in die Zeile des aktuellen Zustands Z gegangen
    											// Spalte ergibt sich aus Zeichenklassenvektor
    											// &0x7F filtert negative Zahlen heraus (0111 1111)
    											// &0xF  filtert Folgezustand OHNE Folgefunktion (siehe Funktionenenum)

    vfx[((vSMatrix[Z][vZKl[X&0x7f]]))>>4]();	// Funktion wird aus Funktionenarray herausgesucht. 
    											// Um die richtige Stelle zu finden, wird das gleiche wie in der Zeile darüber 
        										// gemacht, nur dass nicht &0xF, sondern >>4 angewandt wird.
        										// Dadurch kann das rausfiltern gelassen werden und aus
        										// 0x10 (16) wird 0x1, 0x20 (32) wird 0x2, 0x30 (48) wird 0x3..

    Z=zx;										// Folgezustand wird aktueller Zustand (für Markus N.: wie ne Liste..)
  } while (Ende == 0);							// Das Ganze wird beendet, wenn Ende-Flag von einer der Funktionen umgeändert wurde.
  return &Morph;								// Unnötig, weil mit globalen Variablen gearbeitet wird	
  												// (für Markus N.: lass ma ni mehr so viele kommentare machn)
}

//---- lesen ----
static void fl  (void)
{
  X=fgetc(pIF);
  if (X=='\n') 
  {
	  line++,col=0;
  }
  else 
  {
	  col++;
  }
}


//---- schreiben als Grossbuchstabe, lesen ----
static void fgl (void)
{
  *pBuf = (char)toupper(X);	// oder  *Buf=(char)X&0xDF;
  *(++pBuf) = 0;			// Pointer pBuf auf vBuf wird um eins erhöht, damit das Zeichen nicht überschrieben wird
  fl();
}


//---- schreiben, lesen ----
static void fsl (void)
{
  *pBuf = (char)X;
  *(++pBuf) = 0;
  fl();
}


//---- schreiben, lesen, beenden ----
static void fslb(void)
{
  fsl();
  fb();
}


//---- beenden ----
static void fb  ()
{
  int i,j;
  switch (Z)
  {
    // Werte ergeben sich aus Automatentabelle
    // Symbol
    case 3:									// :
    case 4: 								// <
    case 5: 								// >
    case 0:Morph.Val.Symb=vBuf[0];
	   Morph.MC =mcSymb;
	   break;

    //Zahl
    case 1:Morph.Val.Numb = atol(vBuf);
	   Morph.MC = mcNumb;
	   break;
	
    //Bezeichner/Wortsymbol
    case 2: 
      i=vBuf[0]-'A';
      for(j=0;j<2;j++){
      	if(mSclW[i][j].pKeyWord && !strcmp(vBuf+1,mSclW[i][j].pKeyWord))
      	{
      	  Morph.MC =mcSymb;
      	  Morph.Val.Symb=mSclW[i][j].KWCode;
      	  break;
      	}
      }
      if(!Morph.MC){
      	Morph.Val.pStr=vBuf;
      	Morph.MC =mcIdent;
      }
      break;

    // :=
    case 6:Morph.Val.Symb = (long)zErg;
           Morph.MC = mcSymb;
           break;
    // <=
    case 7:Morph.Val.Symb = (long)zle;
           Morph.MC = mcSymb;
           break;
    // >=
    case 8:Morph.Val.Symb = (long)zge;
           Morph.MC = mcSymb;
           break;
  }
  Ende = 1;	// Ende-Flag wird gesetzt
}

/*---- Testprogramm fuer lexikalische Analyse ----*/
/*
int main(int argc, char *argv[])
{
  initLex(argv[1]);
  while (X != EOF)
  {
	Lex();
    printf("Line: %d\nCol: %d\n",Morph.PosLine, Morph.PosCol);
	 switch(Morph.MC)
    {
       case mcEmpty : 
        break;
       case mcSymb :
	    if (Morph.Val.Symb == zErg)   	   printf("Symbol: :=\n");    
	    else if (Morph.Val.Symb == zle )   printf("Symbol: <=\n");    
	    else if (Morph.Val.Symb == zge )   printf("Symbol: >=\n");    
	    else if (Morph.Val.Symb == zBGN)   printf("Symbol: _BEGIN\n");
	    else if (Morph.Val.Symb == zCLL)   printf("Symbol: _CALL\n"); 
	    else if (Morph.Val.Symb == zCST)   printf("Symbol: _CONST\n");
	    else if (Morph.Val.Symb == zDO )   printf("Symbol: _DO\n");   
	    else if (Morph.Val.Symb == zEND)   printf("Symbol: _END\n");  
	    else if (Morph.Val.Symb == zIF )   printf("Symbol: _IF\n");   
  		else if (Morph.Val.Symb == zODD)   printf("Symbol: _ODD\n");  
  		else if (Morph.Val.Symb == zPRC)   printf("Symbol: _PROCEDURE\n");
  		else if (Morph.Val.Symb == zTHN)   printf("Symbol: _THEN\n"); 
  		else if (Morph.Val.Symb == zVAR)   printf("Symbol: _VAR\n");  
  		else if (Morph.Val.Symb == zWHL)   printf("Symbol: _WHILE\n");

      if (isprint(Morph.Val.Symb))   	   printf("Symbol: %c\n",(char)Morph.Val.Symb);
			
		  break;

      case mcNumb :
          printf("Zahl: %ld\n",Morph.Val.Numb);
				break;
      case mcIdent:
	    printf("Ident: %s\n",(char*)Morph.Val.pStr);
				break;
    }
    printf("-----\n");
  }
  return 0;
}
*/