/*
    Compute the optimal Transportation Distance

    ot.c

    Last update: 7/1/2010

    This code is redistributed with the kind permission of Yossi
    Rubner.  

    All changes are Copyright (c) 2010 University of Wisconsin-Madison
    and were made by Hidayath Ansari and Michael Coen.  

    Email: mhcoen@cs.wisc.edu URL: http://biocomp.wisc.edu

    The substantial changes are: 

    (1) This code now computes the optimal solution for the
    Transportation Problem with equal masses, as described in
    "Introduction to Mathematical Programming" by F. S. Hillier and
    G. J. Lieberman, McGraw-Hill, 1990.  Most importantly, it no
    longer computes the Earth's Movers Distance, except for the
    special case where it equals the Mallow's Distance.  See Levina &
    Bickel (2002) for details of when this occurs.  Attempting to find
    the distance between samples with masses unequal to 1 will trigger
    an error condition.
    
    (2) Data is no longer stored on the stack.  Instead, it kept in a
    heap, which accelerates the code and greatly reduces its memory footprint.

	(3) There is no longer a hardcoded maximum number of flow iterations.

    // Previous header

    An implementation of the Earth Movers Distance.
    Based of the solution for the Transportation problem as described in
    "Introduction to Mathematical Programming" by F. S. Hillier and 
    G. J. Lieberman, McGraw-Hill, 1990.

    Copyright (C) 1998 Yossi Rubner
    Computer Science Department, Stanford University
    E-Mail: rubner@cs.stanford.edu   URL: http://vision.stanford.edu/~rubner
*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "ot.h"

#define DEBUG_LEVEL 0
/*
 DEBUG_LEVEL:
   0 = NO MESSAGES
   1 = PRINT THE NUMBER OF ITERATIONS AND THE FINAL RESULT
   2 = PRINT THE RESULT AFTER EVERY ITERATION
   3 = PRINT ALSO THE FLOW AFTER EVERY ITERATION
   4 = PRINT A LOT OF INFORMATION (PROBABLY USEFUL ONLY FOR THE AUTHOR)
*/


#define MAX_SIG_SIZE1  (MAX_SIG_SIZE+1)  /* FOR THE POSIBLE DUMMY FEATURE */

/* NEW TYPES DEFINITION */

/* node1_t IS USED FOR SINGLE-LINKED LISTS */
typedef struct node1_t {
  int i;
  double val;
  struct node1_t *Next;
} node1_t;

/* node1_t IS USED FOR DOUBLE-LINKED LISTS */
typedef struct node2_t {
  int i, j;
  double val;
  struct node2_t *NextC;               /* NEXT COLUMN */
  struct node2_t *NextR;               /* NEXT ROW */
} node2_t;



/* GLOBAL VARIABLE DECLARATION */
static int _n1, _n2, _n;                          /* SIGNATURES SIZES */
static float** _C;/* THE COST MATRIX */
static node2_t* _X;            /* THE BASIC VARIABLES VECTOR */
/* VARIABLES TO HANDLE _X EFFICIENTLY */
static node2_t *_EndX, *_EnterX;
static char** _IsX;
static node2_t **_RowsX, **_ColsX;
static double _maxW;
static float _maxC;

/* DECLARATION OF FUNCTIONS */
static float init(signature_t *Signature1, signature_t *Signature2,
		  float (*Dist)(feature_t *, feature_t *));
static void findBasicVariables(node1_t *U, node1_t *V);
static int isOptimal(node1_t *U, node1_t *V);
static int findLoop(node2_t **Loop);
static void newSol();
static void russel(double *S, double *D);
static void addBasicVariable(int minI, int minJ, double *S, double *D, 
			     node1_t *PrevUMinI, node1_t *PrevVMinJ,
			     node1_t *UHead);
#if DEBUG_LEVEL > 0
static void printSolution();
#endif


/******************************************************************************
float ot(signature_t *Signature1, signature_t *Signature2,
	  float (*Dist)(feature_t *, feature_t *), flow_t *Flow, int *FlowSize,
      int MaxIters)
  
where

   Signature1, Signature2  Pointers to signatures that their distance we want
              to compute.
   Dist       Pointer to the ground distance. i.e. the function that computes
              the distance between two features.
   Flow       (Optional) Pointer to a vector of flow_t (defined in ot.h) 
              where the resulting flow will be stored. Flow must have n1+n2-1
              elements, where n1 and n2 are the sizes of the two signatures
              respectively.
              If NULL, the flow is not returned.
   FlowSize   (Optional) Pointer to an integer where the number of elements in
              Flow will be stored
   MaxIters   Maximum number of iterations.
              
******************************************************************************/

float ot(signature_t *Signature1, signature_t *Signature2,
	  float (*Dist)(feature_t *, feature_t *),
	  flow_t *Flow, int *FlowSize, int MaxIters)
{
  int itr;
  double totalCost;
  float w;
  node2_t *XP;
  flow_t *FlowP;
  node1_t *U, *V;

  _n1 = Signature1->n;
  _n2 = Signature2->n;
  _n = _n1 > _n2 ? _n1+1 : _n2+1; /* +1 for the possible dummy feature */

  _C = (float**)malloc(_n*sizeof(float*));
  _X = (node2_t*)malloc(2*_n*sizeof(node2_t));
  _RowsX = (node2_t**)malloc(_n*sizeof(node2_t));
  _ColsX = (node2_t**)malloc(_n*sizeof(node2_t));
  _IsX = (char**)malloc(_n*sizeof(char*));

  U = (node1_t*)malloc(_n*sizeof(node1_t));
  V = (node1_t*)malloc(_n*sizeof(node1_t));
  for (itr=0; itr<_n; itr++) {
    _C[itr] = (float*)malloc(_n*sizeof(float));
    _IsX[itr] = (char*)malloc(_n*sizeof(char));
  }

  w = init(Signature1, Signature2, Dist);

#if DEBUG_LEVEL > 1
  printf("\nINITIAL SOLUTION:\n");
  printSolution();
#endif
 
  if (_n1 > 1 && _n2 > 1)  /* IF _n1 = 1 OR _n2 = 1 THEN WE ARE DONE */
    {
      for (itr = 1; itr < MaxIters; itr++)
	{
	  /* FIND BASIC VARIABLES */
	  findBasicVariables(U, V);
	  
	  /* CHECK FOR OPTIMALITY */
	  if (isOptimal(U, V))
	    break;
	  
	  /* IMPROVE SOLUTION */
	  newSol();
	  
#if DEBUG_LEVEL > 1
	  printf("\nITERATION # %d \n", itr);
	  printSolution();
#endif
	}

      if (itr == MaxIters)
	fprintf(stderr, "ot: Maximum number of iterations has been reached (%d)\n",
		MaxIters);
    }

  /* COMPUTE THE TOTAL FLOW */
  totalCost = 0;
  if (Flow != NULL)
    FlowP = Flow;
  for(XP=_X; XP < _EndX; XP++)
    {
      if (XP == _EnterX)  /* _EnterX IS THE EMPTY SLOT */
	continue;
      if (XP->i == Signature1->n || XP->j == Signature2->n)  /* DUMMY FEATURE */
	continue;
      
      if (XP->val == 0)  /* ZERO FLOW */
	continue;

      totalCost += (double)XP->val * _C[XP->i][XP->j];
      if (Flow != NULL)
	{
	  FlowP->from = XP->i;
	  FlowP->to = XP->j;
	  FlowP->amount = XP->val;
	  FlowP++;
	}
    }
  if (Flow != NULL)
    *FlowSize = FlowP-Flow;

#if DEBUG_LEVEL > 0
  printf("\n*** OPTIMAL SOLUTION (%d ITERATIONS): %f ***\n", itr, totalCost);
#endif

  /* Free the malloc'd arrays */
  free(_X);
  free(_RowsX); free(_ColsX);
  free(U); free(V);

  for (itr=0; itr<_n; itr++) {
    free(_C[itr]);
    free(_IsX[itr]);
  }
  free(_C); free(_IsX);
  /* RETURN THE COST == D_OT */
  return (float)(totalCost);
}





/**********************
   init
**********************/
static float init(signature_t *Signature1, signature_t *Signature2, 
		  float (*Dist)(feature_t *, feature_t *))
{
  int i, j;
  double sSum, dSum, diff;
  feature_t *P1, *P2;
  double *S, *D;

  S = (double*)malloc(_n*sizeof(double));
  D = (double*)malloc(_n*sizeof(double));
 

  if (_n1 > MAX_SIG_SIZE || _n2 > MAX_SIG_SIZE)
    {
      fprintf(stderr, "ot: Signature size is limited to %d\n", MAX_SIG_SIZE);
      exit(1);
    }
  
  /* COMPUTE THE DISTANCE MATRIX */
  _maxC = 0;
  for(i=0, P1=Signature1->Features; i < _n1; i++, P1++)
    for(j=0, P2=Signature2->Features; j < _n2; j++, P2++) 
      {
	_C[i][j] = Dist(P1, P2);
	if (_C[i][j] > _maxC)
	  _maxC = _C[i][j];
      }
	
  /* SUM UP THE SUPPLY AND DEMAND */
  sSum = 0.0;
  for(i=0; i < _n1; i++)
    {
      S[i] = Signature1->Weights[i];
      sSum += Signature1->Weights[i];
      _RowsX[i] = NULL;
    }
  dSum = 0.0;
  for(j=0; j < _n2; j++)
    {
      D[j] = Signature2->Weights[j];
      dSum += Signature2->Weights[j];
      _ColsX[j] = NULL;
    }

  if (fabs(sSum-dSum)>0.0001 || fabs(sSum-1)>0.0001)
    {
      fprintf(stderr, "ot: Signatures must each sum to 1\n");
      printf("\n\n\n=====\not: Signatures must both sum to 1\n=====\n\n\n");
      return 0;
    }


  /* IF SUPPLY DIFFERENT THAN THE DEMAND, ADD A ZERO-COST DUMMY CLUSTER */
  diff = sSum - dSum;
  if (fabs(diff) >= EPSILON * sSum)
    {
      if (diff < 0.0)
	{
	  for (j=0; j < _n2; j++)
	    _C[_n1][j] = 0;
	  S[_n1] = -diff;
	  _RowsX[_n1] = NULL;
	  _n1++;
	}
      else
	{
	  for (i=0; i < _n1; i++)
	    _C[i][_n2] = 0;
	  D[_n2] = diff;
	  _ColsX[_n2] = NULL;
	  _n2++;
	}
    }

  /* INITIALIZE THE BASIC VARIABLE STRUCTURES */
  for (i=0; i < _n1; i++)
    for (j=0; j < _n2; j++)
	_IsX[i][j] = 0;
  _EndX = _X;
   
  _maxW = sSum > dSum ? sSum : dSum;

  /* FIND INITIAL SOLUTION */
  russel(S, D);

  _EnterX = _EndX++;  /* AN EMPTY SLOT (ONLY _n1+_n2-1 BASIC VARIABLES) */

  /* Free the malloc'd arrays */
  free(S); free(D);

  return sSum > dSum ? dSum : sSum;
}


/**********************
    findBasicVariables
 **********************/
static void findBasicVariables(node1_t *U, node1_t *V)
{
  int i, j, found;
  int UfoundNum, VfoundNum;
  node1_t u0Head, u1Head, *CurU, *PrevU;
  node1_t v0Head, v1Head, *CurV, *PrevV;

  /* INITIALIZE THE ROWS LIST (U) AND THE COLUMNS LIST (V) */
  u0Head.Next = CurU = U;
  for (i=0; i < _n1; i++)
    {
      CurU->i = i;
      CurU->Next = CurU+1;
      CurU++;
    }
  (--CurU)->Next = NULL;
  u1Head.Next = NULL;

  CurV = V+1;
  v0Head.Next = _n2 > 1 ? V+1 : NULL;
  for (j=1; j < _n2; j++)
    {
      CurV->i = j;
      CurV->Next = CurV+1;
      CurV++;
    }
  (--CurV)->Next = NULL;
  v1Head.Next = NULL;

  /* THERE ARE _n1+_n2 VARIABLES BUT ONLY _n1+_n2-1 INDEPENDENT EQUATIONS,
     SO SET V[0]=0 */
  V[0].i = 0;
  V[0].val = 0;
  v1Head.Next = V;
  v1Head.Next->Next = NULL;

  /* LOOP UNTIL ALL VARIABLES ARE FOUND */
  UfoundNum=VfoundNum=0;
  while (UfoundNum < _n1 || VfoundNum < _n2)
    {

#if DEBUG_LEVEL > 3
      printf("UfoundNum=%d/%d,VfoundNum=%d/%d\n",UfoundNum,_n1,VfoundNum,_n2);
      printf("U0=");
      for(CurU = u0Head.Next; CurU != NULL; CurU = CurU->Next)
	printf("[%d]",CurU-U);
      printf("\n");
      printf("U1=");
      for(CurU = u1Head.Next; CurU != NULL; CurU = CurU->Next)
	printf("[%d]",CurU-U);
      printf("\n");
      printf("V0=");
      for(CurV = v0Head.Next; CurV != NULL; CurV = CurV->Next)
	printf("[%d]",CurV-V);
      printf("\n");
      printf("V1=");
      for(CurV = v1Head.Next; CurV != NULL; CurV = CurV->Next)
	printf("[%d]",CurV-V);
      printf("\n\n");
#endif
      
      found = 0;
      if (VfoundNum < _n2)
	{
	  /* LOOP OVER ALL MARKED COLUMNS */
	  PrevV = &v1Head;
	  for (CurV=v1Head.Next; CurV != NULL; CurV=CurV->Next)
	    {
	      j = CurV->i;
	      /* FIND THE VARIABLES IN COLUMN j */
	      PrevU = &u0Head;
	      for (CurU=u0Head.Next; CurU != NULL; CurU=CurU->Next)
		{
		  i = CurU->i;
		  if (_IsX[i][j])
		    {
		      /* COMPUTE U[i] */
		      CurU->val = _C[i][j] - CurV->val;
		      /* ...AND ADD IT TO THE MARKED LIST */
		      PrevU->Next = CurU->Next;
		      CurU->Next = u1Head.Next != NULL ? u1Head.Next : NULL;
		      u1Head.Next = CurU;
		      CurU = PrevU;
		    }
		  else
		    PrevU = CurU;
		}
	      PrevV->Next = CurV->Next;
	      VfoundNum++;
	      found = 1;
	    }
	}
     if (UfoundNum < _n1)
	{
	  /* LOOP OVER ALL MARKED ROWS */
	  PrevU = &u1Head;
	  for (CurU=u1Head.Next; CurU != NULL; CurU=CurU->Next)
	    {
	      i = CurU->i;
	      /* FIND THE VARIABLES IN ROWS i */
	      PrevV = &v0Head;
	      for (CurV=v0Head.Next; CurV != NULL; CurV=CurV->Next)
		{
		  j = CurV->i;
		  if (_IsX[i][j])
		    {
		      /* COMPUTE V[j] */
		      CurV->val = _C[i][j] - CurU->val;
		      /* ...AND ADD IT TO THE MARKED LIST */
		      PrevV->Next = CurV->Next;
		      CurV->Next = v1Head.Next != NULL ? v1Head.Next: NULL;
		      v1Head.Next = CurV;
		      CurV = PrevV;
		    }
		  else
		    PrevV = CurV;
		}
	      PrevU->Next = CurU->Next;
	      UfoundNum++;
	      found = 1;
	    }
	}
     if (! found)
       {
	 fprintf(stderr, "ot: Unexpected error in findBasicVariables!\n");
	 fprintf(stderr, "This typically happens when the EPSILON defined in\n");
	 fprintf(stderr, "ot.h is not right for the scale of the problem.\n");
	 exit(1);
       }
    }
}




/**********************
    isOptimal
 **********************/
static int isOptimal(node1_t *U, node1_t *V)
{    
  double delta, deltaMin;
  int i, j, minI, minJ;

  /* FIND THE MINIMAL Cij-Ui-Vj OVER ALL i,j */
  deltaMin = INFINITY;
  for(i=0; i < _n1; i++)
    for(j=0; j < _n2; j++)
      if (! _IsX[i][j])
	{
	  delta = _C[i][j] - U[i].val - V[j].val;
	  if (deltaMin > delta)
	    {
              deltaMin = delta;
	      minI = i;
	      minJ = j;
	    }
	}

#if DEBUG_LEVEL > 3
  printf("deltaMin=%f\n", deltaMin);
#endif

   if (deltaMin == INFINITY)
     {
       fprintf(stderr, "ot: Unexpected error in isOptimal.\n");
       exit(0);
     }
   
   _EnterX->i = minI;
   _EnterX->j = minJ;
   
   /* IF NO NEGATIVE deltaMin, WE FOUND THE OPTIMAL SOLUTION */
   return deltaMin >= -EPSILON * _maxC;

/*
   return deltaMin >= -EPSILON;
 */
}


/**********************
    newSol
**********************/
static void newSol()
{
    int i, j, k;
    double xMin;
    int steps;
    node2_t **Loop, *CurX, *LeaveX;

    Loop = (node2_t**)malloc(2*_n*sizeof(node2_t));
 
#if DEBUG_LEVEL > 3
    printf("EnterX = (%d,%d)\n", _EnterX->i, _EnterX->j);
#endif

    /* ENTER THE NEW BASIC VARIABLE */
    i = _EnterX->i;
    j = _EnterX->j;
    _IsX[i][j] = 1;
    _EnterX->NextC = _RowsX[i];
    _EnterX->NextR = _ColsX[j];
    _EnterX->val = 0;
    _RowsX[i] = _EnterX;
    _ColsX[j] = _EnterX;

    /* FIND A CHAIN REACTION */
    steps = findLoop(Loop);

    /* FIND THE LARGEST VALUE IN THE LOOP */
    xMin = INFINITY;
    for (k=1; k < steps; k+=2)
      {
	if (Loop[k]->val < xMin)
	  {
	    LeaveX = Loop[k];
	    xMin = Loop[k]->val;
	  }
      }

    /* UPDATE THE LOOP */
    for (k=0; k < steps; k+=2)
      {
	Loop[k]->val += xMin;
	Loop[k+1]->val -= xMin;
      }

#if DEBUG_LEVEL > 3
    printf("LeaveX = (%d,%d)\n", LeaveX->i, LeaveX->j);
#endif

    /* REMOVE THE LEAVING BASIC VARIABLE */
    i = LeaveX->i;
    j = LeaveX->j;
    _IsX[i][j] = 0;
    if (_RowsX[i] == LeaveX)
      _RowsX[i] = LeaveX->NextC;
    else
      for (CurX=_RowsX[i]; CurX != NULL; CurX = CurX->NextC)
	if (CurX->NextC == LeaveX)
	  {
	    CurX->NextC = CurX->NextC->NextC;
	    break;
	  }
    if (_ColsX[j] == LeaveX)
      _ColsX[j] = LeaveX->NextR;
    else
      for (CurX=_ColsX[j]; CurX != NULL; CurX = CurX->NextR)
	if (CurX->NextR == LeaveX)
	  {
	    CurX->NextR = CurX->NextR->NextR;
	    break;
	  }

    free(Loop);

    /* SET _EnterX TO BE THE NEW EMPTY SLOT */
    _EnterX = LeaveX;
}



/**********************
    findLoop
**********************/
static int findLoop(node2_t **Loop)
{
  int i, steps;
  node2_t **CurX, *NewX;
  char *IsUsed = (char*)malloc(2*_n*sizeof(char)); 
 
  for (i=0; i < _n1+_n2; i++)
    IsUsed[i] = 0;

  CurX = Loop;
  NewX = *CurX = _EnterX;
  IsUsed[_EnterX-_X] = 1;
  steps = 1;

  do
    {
      if (steps%2 == 1)
	{
	  /* FIND AN UNUSED X IN THE ROW */
	  NewX = _RowsX[NewX->i];
	  while (NewX != NULL && IsUsed[NewX-_X])
	    NewX = NewX->NextC;
	}
      else
	{
	  /* FIND AN UNUSED X IN THE COLUMN, OR THE ENTERING X */
	  NewX = _ColsX[NewX->j];
	  while (NewX != NULL && IsUsed[NewX-_X] && NewX != _EnterX)
	    NewX = NewX->NextR;
	  if (NewX == _EnterX)
	    break;
 	}

     if (NewX != NULL)  /* FOUND THE NEXT X */
       {
	 /* ADD X TO THE LOOP */
	 *++CurX = NewX;
	 IsUsed[NewX-_X] = 1;
	 steps++;
#if DEBUG_LEVEL > 3
	 printf("steps=%d, NewX=(%d,%d)\n", steps, NewX->i, NewX->j);    
#endif
       }
     else  /* DIDN'T FIND THE NEXT X */
       {
	 /* BACKTRACK */
	 do
	   {
	     NewX = *CurX;
	     do 
	       {
		 if (steps%2 == 1)
		   NewX = NewX->NextR;
		 else
		   NewX = NewX->NextC;
	       } while (NewX != NULL && IsUsed[NewX-_X]);
	     
	     if (NewX == NULL)
	       {
		 IsUsed[*CurX-_X] = 0;
		 CurX--;
		 steps--;
	       }
	 } while (NewX == NULL && CurX >= Loop);
	 
#if DEBUG_LEVEL > 3
	 printf("BACKTRACKING TO: steps=%d, NewX=(%d,%d)\n",
		steps, NewX->i, NewX->j);    
#endif
           IsUsed[*CurX-_X] = 0;
	   *CurX = NewX;
	   IsUsed[NewX-_X] = 1;
       }     
    } while(CurX >= Loop);
  
  if (CurX == Loop)
    {
      fprintf(stderr, "ot: Unexpected error in findLoop!\n");
      exit(1);
    }
#if DEBUG_LEVEL > 3
  printf("FOUND LOOP:\n");
  for (i=0; i < steps; i++)
    printf("%d: (%d,%d)\n", i, Loop[i]->i, Loop[i]->j);
#endif

  free(IsUsed);

  return steps;
}



/**********************
    russel
**********************/
static void russel(double *S, double *D)
{
  int i, j, found, minI, minJ;
  double deltaMin, oldVal, diff;
  double** Delta;
  node1_t *Ur, *Vr;
  node1_t uHead, *CurU, *PrevU;
  node1_t vHead, *CurV, *PrevV;
  node1_t *PrevUMinI, *PrevVMinJ, *Remember;

  Ur = (node1_t*)malloc(_n*sizeof(node1_t));
  Vr = (node1_t*)malloc(_n*sizeof(node1_t));
  Delta = (double**)malloc(_n*sizeof(double*));

  for (i=0; i<_n; i++) {
    Delta[i] = (double*)malloc(_n*sizeof(double));
  }




  /* INITIALIZE THE ROWS LIST (Ur), AND THE COLUMNS LIST (Vr) */
  uHead.Next = CurU = Ur;
  for (i=0; i < _n1; i++)
    {
      CurU->i = i;
      CurU->val = -INFINITY;
      CurU->Next = CurU+1;
      CurU++;
    }
  (--CurU)->Next = NULL;
  
  vHead.Next = CurV = Vr;
  for (j=0; j < _n2; j++)
    {
      CurV->i = j;
      CurV->val = -INFINITY;
      CurV->Next = CurV+1;
      CurV++;
    }
  (--CurV)->Next = NULL;
  
  /* FIND THE MAXIMUM ROW AND COLUMN VALUES (Ur[i] AND Vr[j]) */
  for(i=0; i < _n1 ; i++)
    for(j=0; j < _n2 ; j++)
      {
	float v;
	v = _C[i][j];
	if (Ur[i].val <= v)
	  Ur[i].val = v;
	if (Vr[j].val <= v)
	  Vr[j].val = v;
      }
  
  /* COMPUTE THE Delta MATRIX */
  for(i=0; i < _n1 ; i++)
    for(j=0; j < _n2 ; j++)
      Delta[i][j] = _C[i][j] - Ur[i].val - Vr[j].val;

  /* FIND THE BASIC VARIABLES */
  do
    {
#if DEBUG_LEVEL > 3
      printf("Ur=");
      for(CurU = uHead.Next; CurU != NULL; CurU = CurU->Next)
	printf("[%d]",CurU-Ur);
      printf("\n");
      printf("Vr=");
      for(CurV = vHead.Next; CurV != NULL; CurV = CurV->Next)
	printf("[%d]",CurV-Vr);
      printf("\n");
      printf("\n\n");
#endif
 
      /* FIND THE SMALLEST Delta[i][j] */
      found = 0; 
      deltaMin = INFINITY;      
      PrevU = &uHead;
      for (CurU=uHead.Next; CurU != NULL; CurU=CurU->Next)
	{
	  int i;
	  i = CurU->i;
	  PrevV = &vHead;
	  for (CurV=vHead.Next; CurV != NULL; CurV=CurV->Next)
	    {
	      int j;
	      j = CurV->i;
	      if (deltaMin > Delta[i][j])
		{
		  deltaMin = Delta[i][j];
		  minI = i;
		  minJ = j;
		  PrevUMinI = PrevU;
		  PrevVMinJ = PrevV;
		  found = 1;
		}
	      PrevV = CurV;
	    }
	  PrevU = CurU;
	}
      
      if (! found)
	break;

      /* ADD X[minI][minJ] TO THE BASIS, AND ADJUST SUPPLIES AND COST */
      Remember = PrevUMinI->Next;
      addBasicVariable(minI, minJ, S, D, PrevUMinI, PrevVMinJ, &uHead);

      /* UPDATE THE NECESSARY Delta[][] */
      if (Remember == PrevUMinI->Next)  /* LINE minI WAS DELETED */
	{
	  for (CurV=vHead.Next; CurV != NULL; CurV=CurV->Next)
	    {
	      int j;
	      j = CurV->i;
	      if (CurV->val == _C[minI][j])  /* COLUMN j NEEDS UPDATING */
		{
		  /* FIND THE NEW MAXIMUM VALUE IN THE COLUMN */
		  oldVal = CurV->val;
		  CurV->val = -INFINITY;
		  for (CurU=uHead.Next; CurU != NULL; CurU=CurU->Next)
		    {
		      int i;
		      i = CurU->i;
		      if (CurV->val <= _C[i][j])
			CurV->val = _C[i][j];
		    }
		  
		  /* IF NEEDED, ADJUST THE RELEVANT Delta[*][j] */
		  diff = oldVal - CurV->val;
		  if (fabs(diff) < EPSILON * _maxC)
		    for (CurU=uHead.Next; CurU != NULL; CurU=CurU->Next)
		      Delta[CurU->i][j] += diff;
		}
	    }
	}
      else  /* COLUMN minJ WAS DELETED */
	{
	  for (CurU=uHead.Next; CurU != NULL; CurU=CurU->Next)
	    {
	      int i;
	      i = CurU->i;
	      if (CurU->val == _C[i][minJ])  /* ROW i NEEDS UPDATING */
		{
		  /* FIND THE NEW MAXIMUM VALUE IN THE ROW */
		  oldVal = CurU->val;
		  CurU->val = -INFINITY;
		  for (CurV=vHead.Next; CurV != NULL; CurV=CurV->Next)
		    {
		      int j;
		      j = CurV->i;
		      if(CurU->val <= _C[i][j])
			CurU->val = _C[i][j];
		    }
		  
		  /* If NEEDED, ADJUST THE RELEVANT Delta[i][*] */
		  diff = oldVal - CurU->val;
		  if (fabs(diff) < EPSILON * _maxC)
		    for (CurV=vHead.Next; CurV != NULL; CurV=CurV->Next)
		      Delta[i][CurV->i] += diff;
		}
	    }
	}
    } while (uHead.Next != NULL || vHead.Next != NULL);

    for (i=0; i<_n; i++) {
      free(Delta[i]);
    }

    free(Delta);
    free(Ur); free(Vr);
}




/**********************
    addBasicVariable
**********************/
static void addBasicVariable(int minI, int minJ, double *S, double *D, 
			     node1_t *PrevUMinI, node1_t *PrevVMinJ,
			     node1_t *UHead)
{
  double T;
  
  if (fabs(S[minI]-D[minJ]) <= EPSILON * _maxW)  /* DEGENERATE CASE */
    {
      T = S[minI];
      S[minI] = 0;
      D[minJ] -= T; 
    }
  else if (S[minI] < D[minJ])  /* SUPPLY EXHAUSTED */
    {
      T = S[minI];
      S[minI] = 0;
      D[minJ] -= T; 
    }
  else  /* DEMAND EXHAUSTED */
    {
      T = D[minJ];
      D[minJ] = 0; 
      S[minI] -= T; 
    }

  /* X(minI,minJ) IS A BASIC VARIABLE */
  _IsX[minI][minJ] = 1; 

  _EndX->val = T;
  _EndX->i = minI;
  _EndX->j = minJ;
  _EndX->NextC = _RowsX[minI];
  _EndX->NextR = _ColsX[minJ];
  _RowsX[minI] = _EndX;
  _ColsX[minJ] = _EndX;
  _EndX++;

  /* DELETE SUPPLY ROW ONLY IF THE EMPTY, AND IF NOT LAST ROW */
  if (S[minI] == 0 && UHead->Next->Next != NULL)
    PrevUMinI->Next = PrevUMinI->Next->Next;  /* REMOVE ROW FROM LIST */
  else
    PrevVMinJ->Next = PrevVMinJ->Next->Next;  /* REMOVE COLUMN FROM LIST */
}





/**********************
    printSolution
**********************/
static void printSolution()
{
  node2_t *P;
  double totalCost;

  totalCost = 0;

#if DEBUG_LEVEL > 2
  printf("SIG1\tSIG2\tFLOW\tCOST\n");
#endif
  for(P=_X; P < _EndX; P++)
    if (P != _EnterX && _IsX[P->i][P->j])
      {
#if DEBUG_LEVEL > 2
	printf("%d\t%d\t%f\t%f\n", P->i, P->j, P->val, _C[P->i][P->j]);
#endif
	totalCost += (double)P->val * _C[P->i][P->j];
      }

  printf("COST = %f\n", totalCost);
}

int main() {
  return 0;
}
