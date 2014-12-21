#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "lexer.h"

//Bogentyp
typedef enum 
{
  BgNl= 0,  // NIL        0000 
  BgSy= 1,  // Symbol     0001
  BgMo= 2,  // Morphem    0010
  BgGr= 4,  // Graph      0100
  BgEn= 8,  // Graphende  1000
}tBg;

/*struct tBog;

typedef struct{
  struct tBog* pBogen;
  char* Bez;
}tIdxGr;*/

typedef struct BOGEN
{
  tBg BgD;                //Bogentyp (siehe oben)
  union BGX               //was erkannt werden soll/ in welchen Graphen gesprungen werden soll
  {
    unsigned long X;      //
    int           S;      //Symbol
    tMC           M;      //Morphem
    struct BOGEN* G;      //Zeiger auf nächsten Bogen 
  }BgX;
  int (*fx)(void); 
  int iNext; 
  int iAlt; 
}tBog;

/* Pars-Funktion */
int pars(tBog* pGraph)    //prüft syntaktische Korrektheit des Programms durch Schleifen-Durchläufe (ein Durchlauf = ein Bogen)
{
  tBog* pBog=pGraph;
  int succ=0;
  if (Morph.MC==mcEmpty)
    Lex();                //beim ersten Aufruf des Parsers
  while(1)
  {
    switch(pBog->BgD)
    {
      case BgNl:
      	succ=1;
      	break;
      case BgSy:
      	succ=(Morph.Val.Symb==pBog->BgX.S);
      	break;
      case BgMo:
      	succ=(Morph.MC==(tMC)pBog->BgX.M);
      	break;
      case BgGr:
      	succ=pars(pBog->BgX.G);
      	break;
      case BgEn:
	      return 1;   /* Ende erreichet  Erfolg */
    }
    if (succ && pBog->fx!=NULL)
      succ=pBog->fx();
    if (!succ)
      /* Alternativbogen probieren */
      if (pBog->iAlt != 0)
      pBog=pGraph+pBog->iAlt;
      else return FAIL;
    else /* Morphem formal akzeptiert */
    {
      if (pBog->BgD & BgSy || pBog->BgD & BgMo) 
	Lex();
      pBog=pGraph+pBog->iNext;
    }
  }/* while */
}

/*---- Sammlung von Parserbögen ----*/


//Bogentyp, nächster zu verarbeitender Bogen/Morphem/Symbol... , Funktion zur Codeerzeugung, Folgebogen, Alternativbogen


tBog gExpression[];

//Factor
tBog gFact[6]=
{
/* 0*/ {BgMo,{(ul)mcIdent}, NULL, 5, 1},
/* 1*/ {BgMo,{(ul)mcNumb }, NULL, 5, 2},
/* 2*/ {BgSy,{(ul)'('    }, NULL, 3, 0},
/* 3*/ {BgGr,{(ul)gExpression  }, NULL, 4, 0},
/* 4*/ {BgSy,{(ul)')'    }, NULL, 5, 0},
/* 5*/ {BgEn,{(ul)0      }, NULL, 0, 0}
};

//Term
tBog gTerm[7]=
{
  /*0*/ {BgGr, {(ul)gFact}, NULL, 1, 0},
  /*1*/ {BgSy, {(ul)'*'},   NULL, 2, 3},
  /*2*/ {BgGr, {(ul)gFact}, NULL, 1, 0},
  /*3*/ {BgSy, {(ul)'/'},   NULL, 4, 5},
  /*4*/ {BgGr, {(ul)gFact}, NULL, 1, 0},
  /*5*/ {BgNl, {(ul)0},     NULL, 6, 0},
  /*6*/ {BgEn, {(ul)0},     NULL, 9, 0}
};

/* Condition */
/*
tBog gCondition[11]=
{
  {BgSy,{(ul)zODD	},       NULL, 1, 2},
  {BgGr,{(ul)gExpression}, NULL,10, 0},
  {BgGr,{(ul)gExpression}, NULL, 3, 0},
  {BgSy,{(ul)'='	},       NULL, 9, 4},
  {BgSy,{(ul)'#'	},       NULL, 9, 5},
  {BgSy,{(ul)'<'	},       NULL, 9, 6},
  {BgSy,{(ul)zle	},       NULL, 9, 7},
  {BgSy,{(ul)'>'	},       NULL, 9, 8},
  {BgSy,{(ul)zge	},       NULL, 9, 0},
  {BgGr,{(ul)gExpression}, NULL,10, 0},
  {BgNl,{(ul)0		},       NULL, 0, 0}
};
*/
tBog gCondition[11]=
{
  /*0*/ {BgSy, {(ul)zODD},        NULL, 1, 2},
  /*1*/ {BgGr, {(ul)gExpression}, NULL, 10, 0},
  /*2*/ {BgGr, {(ul)gExpression}, NULL, 3, 0},
  /*3*/ {BgSy, {(ul)'='},         NULL, 9, 4},
  /*4*/ {BgSy, {(ul)'#'},         NULL, 9, 5},
  /*5*/ {BgSy, {(ul)'<'},         NULL, 9, 6},
  /*6*/ {BgSy, {(ul)zle},         NULL, 9, 7},
  /*7*/ {BgSy, {(ul)'>'},         NULL, 9, 8},
  /*8*/ {BgSy, {(ul)zge},         NULL, 9, 0},
  /*9*/ {BgGr, {(ul)gExpression}, NULL, 10, 0},
  /*10*/ {BgEn, {(ul)0},          NULL, 0, 0}
};

//Expression
tBog gExpression[8]=
{
  {BgSy,{(ul)'-'	}, NULL, 1, 3},
  {BgGr,{(ul)gTerm	}, NULL, 3, 0},
  {BgGr,{(ul)gTerm	}, NULL, 3, 0},
  {BgSy,{(ul)'+'	}, NULL, 4, 5},
  {BgGr,{(ul)gTerm	}, NULL, 3, 0}, // 4
  {BgSy,{(ul)'-'	}, NULL, 6, 7},
  {BgGr,{(ul)gTerm	}, NULL, 3, 0},
  {BgNl,{(ul)0		}, NULL, 0, 0}
};

//Statement
tBog gStatement[23]=
{
  {BgMo,{(ul)mcIdent	}, NULL, 1, 3},
  {BgSy,{(ul)zErg	}, NULL, 2, 0},
  {BgGr,{(ul)gExpression}, NULL,22, 0},
  {BgSy,{(ul)zIF	}, NULL, 4, 7},
  {BgGr,{(ul)gCondition	}, NULL, 5, 0}, // 4
  {BgSy,{(ul)zTHN	}, NULL, 6, 0},
  {BgGr,{(ul)gStatement	}, NULL,22, 0},
  {BgSy,{(ul)zWHL	}, NULL, 8,11},
  {BgGr,{(ul)gCondition	}, NULL, 9, 0},
  {BgSy,{(ul)zDO	}, NULL,10, 0}, // 9
  {BgGr,{(ul)gStatement	}, NULL,22, 0},
  {BgSy,{(ul)zBGN	}, NULL,12,15},
  {BgGr,{(ul)gStatement	}, NULL,13, 0},
  {BgSy,{(ul)zEND	}, NULL,22,14},
  {BgSy,{(ul)';'	}, NULL,12, 0}, //14
  {BgSy,{(ul)zCLL	}, NULL,16,17},
  {BgMo,{(ul)mcIdent	}, NULL,22, 0},
  {BgSy,{(ul)'?'	}, NULL,18,19},
  {BgMo,{(ul)mcIdent	}, NULL,22, 0},
  {BgSy,{(ul)'!'	}, NULL,20,21}, //19
  {BgGr,{(ul)gExpression}, NULL,22, 0},
  {BgNl,{(ul)0		}, NULL,22, 0},
  {BgEn,{(ul)0		}, NULL, 0, 0}
};

//Block
tBog gBlock[20]=
{
  {BgSy,{(ul)zCST	}, NULL, 1, 6},
  {BgMo,{(ul)mcIdent	}, NULL, 2, 0},
  {BgSy,{(ul)'='	}, NULL, 3, 0},
  {BgMo,{(ul)mcNumb	}, NULL, 4, 0},
  {BgSy,{(ul)','	}, NULL, 1, 5}, // 4
  {BgSy,{(ul)';'	}, NULL, 7, 0},
  {BgNl,{(ul)0		}, NULL, 7, 0},
  {BgSy,{(ul)zVAR	}, NULL, 8,11},
  {BgMo,{(ul)mcIdent	}, NULL, 9, 0},
  {BgSy,{(ul)','	}, NULL, 8,10}, // 9
  {BgSy,{(ul)';'	}, NULL,12, 0},
  {BgNl,{(ul)0		}, NULL,12, 0},
  {BgSy,{(ul)zPRC	}, NULL,13,17},
  {BgMo,{(ul)mcIdent	}, NULL,14, 0},
  {BgSy,{(ul)';'	}, NULL,15, 0}, //14
  {BgGr,{(ul)gBlock	}, NULL,16, 0},
  {BgSy,{(ul)';'	}, NULL,12, 0},
  {BgNl,{(ul)0		}, NULL,18, 0},
  {BgGr,{(ul)gStatement	}, NULL,19, 0},
  {BgEn,{(ul)0		}, NULL, 0, 0}  //19
};

//Programm
tBog gProgramm[3]=
{
  {BgGr,{(ul)gBlock	}, NULL, 1, 0},
  {BgSy,{(ul)'.'	}, NULL, 2, 0},
  {BgEn,{(ul)0		}, NULL, 0, 0}  
};

int main()
{
  return 0;
}
