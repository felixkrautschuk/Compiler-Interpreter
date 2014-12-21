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
  mcEmpty, 
  mcSymb, 
  mcNumb, 
  mcIdent
}tMC;

typedef enum T_ZS
{
  zNIL,
  zErg=128,zle,zge,
  zBGN,zCLL,zCST,zDO,zEND,zIF,zODD,zPRC,zTHN,zVAR,zWHL
}tZS;

typedef struct morph
{
	tMC MC;
	int PosLine;
	int PosCol;
	int MLen;
	union VAL
	{
		long Numb;
		char *pStr;
		int  Symb;
	}Val;
}tMorph;

int initLex(char* fname);
tMorph* Lex(void);

static tMorph Morph;			    //aktuelles Morphem

#define FAIL -1

typedef unsigned long ul;

