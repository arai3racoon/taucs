/*********************************************************/
/* TAUCS                                                 */
/* Author: Sivan Toledo                                  */
/*********************************************************/
/* 

TAUCS_CONFIG_DEFAULT OFF
TAUCS_CONFIG BASE
TAUCS_CONFIG DREAL
TAUCS_CONFIG SREAL
TAUCS_CONFIG DCOMPLEX
TAUCS_CONFIG SCOMPLEX
TAUCS_CONFIG FACTOR
TAUCS_CONFIG OOC_LLT
TAUCS_CONFIG METIS
TAUCS_CONFIG GENMMD
TAUCS_CONFIG COLAMD
TAUCS_CONFIG AMD
TAUCS_CONFIG GENERIC_COMPLEX
TAUCS_CONFIG MALLOC_STUBS
TAUCS_CONFIG MATRIX_GENERATORS
TAUCS_CONFIG MATRIX_IO
TAUCS_CONFIG AD_HOC_TEST
TAUCS_CONFIG_END

*/

#include <stdio.h>
#include <stdlib.h>
#include "taucs.h"

/* this only works for generic complex */
#define taucs_im(x)    ((x).i)
#define taucs_re(x)    ((x).r)

#define TRUE  1
#define FALSE 0

char * gl_parameters = "";

void rnorm(taucs_ccs_matrix* A, void* x, void* b, void* aux)
{
  double relerr;
  
  taucs_ccs_times_vec(A,x,aux);

  taucs_vec_axpby(A->n,A->flags,1.0,aux,-1.0,b,aux);

  relerr = taucs_vec_norm2(A->n,A->flags,aux) 
           / taucs_vec_norm2(A->n,A->flags,b);

  taucs_printf("relative 2-norm of the residual %.2e \n",relerr);
}

void rnorm_many(taucs_ccs_matrix* A, void* x, void* b, void* aux,int nrhs)
{
  double relerr = 0.0;
  int i;
  
  taucs_ccs_times_vec_many(A,x,aux,nrhs);
  
  taucs_vec_axpby_many(A->m,A->flags,1.0,aux,-1.0,b,aux,nrhs);
  
  for (i=0;i<nrhs;i++) {
    if (A->flags & TAUCS_DOUBLE)
      relerr = taucs_vec_norm2(A->m,A->flags,(taucs_double *)aux+i*(A->m)) 
	/ taucs_vec_norm2(A->m,A->flags,(taucs_double *)b+i*(A->m));
    else if (A->flags & TAUCS_SINGLE)
      relerr = taucs_vec_norm2(A->m,A->flags,(taucs_single *)aux+i*(A->m)) 
	/ taucs_vec_norm2(A->m,A->flags,(taucs_single *)b+i*(A->m));
    else if (A->flags & TAUCS_DCOMPLEX)
      relerr = taucs_vec_norm2(A->m,A->flags,(taucs_dcomplex *)aux+i*(A->m)) 
	/ taucs_vec_norm2(A->m,A->flags,(taucs_dcomplex *)b+i*(A->m));
    else if (A->flags & TAUCS_SCOMPLEX)
      relerr = taucs_vec_norm2(A->m,A->flags,(taucs_scomplex *)aux+i*(A->m)) 
           / taucs_vec_norm2(A->m,A->flags,(taucs_scomplex *)b+i*(A->m));
    
    taucs_printf("relative 2-norm of the residual %.2e \n",relerr);
  }
}

int main(int argc, char* argv[])
{
  double start;
  int rc;
  int i,j;

  taucs_ccs_matrix*  A = NULL;
  void*      X;
  void*      B;
  void*      Y;
  void*      Z;
  void* opt_arg[] = { NULL };

  double opt_nrhs = 1.0;

  char* opt_ijv = NULL;
  char* opt_ijv_zero = NULL;
  char* opt_hb  = NULL;
  char* opt_bin = NULL;
  char* opt_log = "stdout";
  double opt_3d = -1.0;
  double opt_2d = -1.0;
  char*  opt_2d_type = "dirichlet";
  int opt_3d_rand = 0;/*not random*/
  double opt_3d_small = -1.0;/*regular*/

  double opt_shift = 0.0;

  int opt_sreal    = 0;
  int opt_dreal    = 0;
  int opt_scomplex = 0;
  int opt_dcomplex = 0;
  int datatype     = TAUCS_DOUBLE;

  int opt_all1rhs  = 0;

  double rerr;

  for (i=0; argv[i]; i++) {
    int understood = FALSE;
    
    understood |= taucs_getopt_boolean(argv[i],opt_arg,"taucs_run.sreal",&opt_sreal);
    understood |= taucs_getopt_boolean(argv[i],opt_arg,"taucs_run.dreal",&opt_dreal);
    understood |= taucs_getopt_boolean(argv[i],opt_arg,"taucs_run.scomplex",&opt_scomplex);
    understood |= taucs_getopt_boolean(argv[i],opt_arg,"taucs_run.dcomplex",&opt_dcomplex);

    understood |= taucs_getopt_string(argv[i],opt_arg,"taucs_run.ijv",&opt_ijv);
		understood |= taucs_getopt_string(argv[i],opt_arg,"taucs_run.ijvz",&opt_ijv_zero);
    understood |= taucs_getopt_string(argv[i],opt_arg,"taucs_run.hb", &opt_hb );
    understood |= taucs_getopt_string(argv[i],opt_arg,"taucs_run.bin", &opt_bin );
    understood |= taucs_getopt_string(argv[i],opt_arg,"taucs_run.log",&opt_log);
    understood |= taucs_getopt_double(argv[i],opt_arg,"taucs_run.mesh3d",&opt_3d);
    understood |= taucs_getopt_boolean(argv[i],opt_arg,"taucs_run.mesh3d.rand",&opt_3d_rand);
    understood |= taucs_getopt_double(argv[i],opt_arg,"taucs_run.mesh3d.small",&opt_3d_small);
    understood |= taucs_getopt_double(argv[i],opt_arg,"taucs_run.mesh2d",&opt_2d);
    understood |= taucs_getopt_string(argv[i],opt_arg,"taucs_run.mesh2d.type",&opt_2d_type);
    understood |= taucs_getopt_double(argv[i],opt_arg,"taucs_run.shift",&opt_shift);
    understood |= taucs_getopt_double(argv[i],opt_arg,"taucs_run.nrhs",&opt_nrhs);

    understood |= taucs_getopt_string(argv[i],opt_arg,"taucs_run.params",&gl_parameters);

    understood |= taucs_getopt_boolean(argv[i],opt_arg,"taucs_run.all1rhs",&opt_all1rhs);

    if (!understood) taucs_printf("taucs_run: illegal option [%s]\n",
				  argv[i]);
  }

  if (opt_sreal   ) datatype = TAUCS_SINGLE;
  if (opt_dreal   ) datatype = TAUCS_DOUBLE;
  if (opt_scomplex) datatype = TAUCS_SCOMPLEX;
  if (opt_dcomplex) datatype = TAUCS_DCOMPLEX;

  taucs_logfile(opt_log);

  if (opt_3d > 0) {
    if ( opt_3d_small > 0 ) {
        A = taucs_ccs_generate_mesh3d_random((int)opt_3d_small,(int)opt_3d,(int)opt_3d,opt_3d_rand);
    }
    A = taucs_ccs_generate_mesh3d_random((int)opt_3d,(int)opt_3d,(int)opt_3d,opt_3d_rand);
    if (!A) {
      taucs_printf("Matrix generation failed\n");
      return 1;
    }
    datatype = TAUCS_DOUBLE;
  }

  if (opt_2d > 0) {
    A = taucs_ccs_generate_mesh2d((int)opt_2d,opt_2d_type);
    if (!A) {
      taucs_printf("Matrix generation failed\n");
      return 1;
    }
    datatype = TAUCS_DOUBLE;
  }


  if (opt_ijv) {
    switch (datatype) {
    case TAUCS_SINGLE:
      A = taucs_ccs_read_ijv (opt_ijv,TAUCS_SYMMETRIC | TAUCS_SINGLE); break;
      break;
    case TAUCS_DOUBLE:
      A = taucs_ccs_read_ijv (opt_ijv,TAUCS_SYMMETRIC | TAUCS_DOUBLE); break;
      break;
    case TAUCS_SCOMPLEX:
      A = taucs_ccs_read_ijv (opt_ijv,TAUCS_HERMITIAN | TAUCS_SCOMPLEX); break;
      break;
    case TAUCS_DCOMPLEX:
      A = taucs_ccs_read_ijv (opt_ijv,TAUCS_HERMITIAN | TAUCS_DCOMPLEX); break;
      break;
    default:
      taucs_printf("taucs_run: incorrect datatype\n");
      return 1;
      break;
    }      
  }

  if (opt_ijv_zero) {
    switch (datatype) {
    case TAUCS_SINGLE:
      A = taucs_ccs_read_ijv_zero_based (opt_ijv_zero,TAUCS_SYMMETRIC | TAUCS_SINGLE); break;
      break;
    case TAUCS_DOUBLE:
      A = taucs_ccs_read_ijv_zero_based (opt_ijv_zero,TAUCS_SYMMETRIC | TAUCS_DOUBLE); break;
      break;
    case TAUCS_SCOMPLEX:
      A = taucs_ccs_read_ijv_zero_based(opt_ijv_zero,TAUCS_HERMITIAN | TAUCS_SCOMPLEX); break;
      break;
    case TAUCS_DCOMPLEX:
      A = taucs_ccs_read_ijv_zero_based(opt_ijv_zero,TAUCS_HERMITIAN | TAUCS_DCOMPLEX); break;
      break;
    default:
      taucs_printf("taucs_run: incorrect datatype\n");
      return 1;
      break;
    }      
  }
  
  if (opt_hb) {
    switch (datatype) {
    case TAUCS_SINGLE:
      A = taucs_ccs_read_hb (opt_hb, TAUCS_SINGLE); break;
      break;
    case TAUCS_DOUBLE:
      A = taucs_ccs_read_hb (opt_hb, TAUCS_DOUBLE); break;
      break;
    case TAUCS_SCOMPLEX:
      A = taucs_ccs_read_hb (opt_hb, TAUCS_SCOMPLEX); break;
      break;
    case TAUCS_DCOMPLEX:
      A = taucs_ccs_read_hb (opt_hb, TAUCS_DCOMPLEX); break;
      break;
    default:
      taucs_printf("taucs_run: incorrect datatype\n");
      return 1;
      break;
    }
    datatype = A->flags;
  }

  if (opt_bin) {
    A = taucs_ccs_read_binary(opt_bin);
  }
  if (!A) {
    taucs_printf("taucs_run: there is no matrix!\n");
    return 1;
  }


  if (opt_shift) {
    int i,j;
    int ip;
    if (datatype & TAUCS_DOUBLE) {
      for (j=0; j<A->n; j++) {
	int found = 0;
	for (ip = (A->colptr)[j]; ip < (A->colptr)[j+1]; ip++) {
	  i =  (A->rowind)[ip];
	  if (i == j) {
	    (A->values.d)[ip] += opt_shift;
	    found = 1;
	    break;
	  }
	}
	if (!found) {
	  taucs_printf("taucs_run: the matrix is missing a diagonal element so I can't shift\n");
	  return 1;
	}
      }
		} else {
      taucs_printf("taucs_run: shift only works on double-precision matrices\n");
			return 1;
    } 
  }   
    

  taucs_printf("taucs_run: matrix dimentions %d by %d with %d nonzeros\n",
	       A->m, A->n, (A->colptr)[ A->n ]);


  X = taucs_vec_create((A->n)*(int)opt_nrhs,A->flags);
  B = taucs_vec_create((A->m)*(int)opt_nrhs,A->flags);
  Y = taucs_vec_create((A->n)*(int)opt_nrhs,A->flags);
  Z = taucs_vec_create((A->m)*(int)opt_nrhs,A->flags);
  if (!X || !B || !Y || !Z) {
    taucs_printf("taucs_run: vector allocation failed\n");
    return 1;
  }

  if (!opt_all1rhs) {
	for(j=0;j<(int)opt_nrhs;j++) {
	  for(i=0; i<A->n; i++) {

#ifdef TAUCS_SINGLE_IN_BUILD
		  if (datatype & TAUCS_SINGLE) 
		    ((taucs_single*)X)[i+j*(A->n)]=(taucs_single) ((double)rand()/(double)RAND_MAX);
#endif

#ifdef TAUCS_DOUBLE_IN_BUILD
		  if (datatype & TAUCS_DOUBLE) 
		    //((taucs_double*)X)[i+j*(A->n)]=(taucs_double) ((double)rand()/(double)RAND_MAX);
		  ((taucs_double*)X)[i+j*(A->n)]=0.1 + i * 0.01;
#endif

#ifdef TAUCS_SCOMPLEX_IN_BUILD
		  if (datatype & TAUCS_SCOMPLEX) {
		    taucs_single   cre,cim;
		    cre = (taucs_single) ((double)rand()/(double)RAND_MAX);
		    cim = (taucs_single) ((double)rand()/(double)RAND_MAX);
		    ((taucs_scomplex*)X)[i+j*(A->n)] = taucs_ccomplex_create(cre,cim);
		  }
#endif

#ifdef TAUCS_DCOMPLEX_IN_BUILD
		  if (datatype & TAUCS_DCOMPLEX) {
		    taucs_single   zre,zim;
		    zre = (taucs_double) ((double)rand()/(double)RAND_MAX);
		    zim = (taucs_double) ((double)rand()/(double)RAND_MAX);
		    ((taucs_dcomplex*)X)[i+j*(A->n)] = taucs_zcomplex_create(zre,zim);
		  }
#endif
		}
	}

	taucs_ccs_times_vec_many(A,X,B,(int)opt_nrhs);
  } else { 
    for(j=0;j<(int)opt_nrhs;j++) {
      for(i=0; i<A->m; i++) {
#ifdef TAUCS_SINGLE_IN_BUILD
	if (datatype & TAUCS_SINGLE) 
	  ((taucs_single*)B)[i+j*(A->m)]= 1.0;
#endif
	
#ifdef TAUCS_DOUBLE_IN_BUILD
	if (datatype & TAUCS_DOUBLE) 
	  ((taucs_double*)B)[i+j*(A->m)]= 1.0;
#endif

#ifdef TAUCS_SCOMPLEX_IN_BUILD
	if (datatype & TAUCS_SCOMPLEX) {
	  taucs_single   cre,cim;
	  cre = 1.0;
	  cim = 0.0;
	  ((taucs_scomplex*)B)[i+j*(A->m)] = taucs_ccomplex_create(cre,cim);
	}
#endif

#ifdef TAUCS_DCOMPLEX_IN_BUILD
	if (datatype & TAUCS_DCOMPLEX) {
	  taucs_single   zre,zim;
	  zre = 1.0;
	  zim = 0.0;
	  ((taucs_dcomplex*)B)[i+j*(A->m)] = taucs_zcomplex_create(zre,zim);
	}
#endif
      }
    }   
  }

  start = taucs_wtime();
  rc = taucs_linsolve(A,NULL,(int)opt_nrhs,Y,B,argv,opt_arg);
  taucs_printf("total solve time is %.2e sec\n", taucs_wtime() - start);
  
  if (!opt_all1rhs && opt_nrhs == 1) {
    taucs_vec_axpby_many(A->n,A->flags,1.0,X,-1.0,Y,Z,(int)opt_nrhs);
    rerr = taucs_vec_norm2(A->n, A->flags,Z) / taucs_vec_norm2(A->n, A->flags, X);
    taucs_printf("relative 2-norm of error is %.2e\n", rerr);
  }

  rnorm_many(A,Y,B,Z,(int)opt_nrhs);

  if (opt_nrhs == 1) 
  {
    double xnorm = taucs_vec_norm2(A->n,A->flags,Y);
    taucs_printf("2-norm of x is %.2e\n", xnorm);
  }


  // Silencing valgrind
  if (X) 
    taucs_vec_free(A->flags,X);
  if (B) 
    taucs_vec_free(A->flags,B);
  if (Y) 
    taucs_vec_free(A->flags,Y);
  if (Z) 
    taucs_vec_free(A->flags,Z);
  
  taucs_ccs_free(A);
  
  return 0;
}
