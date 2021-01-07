// #include <stdlib.h>
// #include <unistd.h>
#include <math.h>
#include <stdio.h>
#ifdef _OPENMP
#include <omp.h>
#endif

#include "equation_of_state.h"
#include "parametres.h"
#include "perfcnt.h"
#include "utils.h"

void
equation_of_state(int imin,
		  int imax,
		  const int Hnxyt,
		  const int Hnvar,
		  const real_t Hsmallc,
		  const real_t Hgamma,
		  const int slices, const int Hstep,
		  real_t eint[Hstep][Hnxyt], real_t q[Hnvar][Hstep][Hnxyt],
		  real_t c[Hstep][Hnxyt])
{
    int k, s;
    int inpar = 0;
    real_t smallp;

    WHERE("equation_of_state");
    smallp = Square(Hsmallc) / Hgamma;

    // printf("EOS: %d %d %d %d %g %g %d %d\n", imin, imax, Hnxyt, Hnvar, Hsmallc, Hgamma, slices, Hstep);
#ifdef TARGETON
#pragma omp target			\
	map(q[0:Hnvar][0:Hstep][0:Hnxyt]) \
	map(c[0:Hstep][0:Hnxyt]) \
	map(eint[0:Hstep][0:Hnxyt])
#pragma omp teams distribute parallel for default(none), \
	firstprivate(slices, Hgamma, smallp, imin, imax) \
	private(s,k), \
	shared(c,q, eint) collapse(2)
#else
#pragma omp parallel for default(none) firstprivate(slices, Hgamma, smallp, imin, imax) private(s,k), shared(c,q, eint) COLLAPSE
#endif
    for (s = 0; s < slices; s++) {
	for (k = imin; k < imax; k++) {
	    register real_t rhok = q[ID][s][k];
	    register real_t base = (Hgamma - one) * rhok * eint[s][k];
	    base = MAX(base, (real_t) (rhok * smallp));

	    q[IP][s][k] = base;
	    c[s][k] = sqrt(Hgamma * base / rhok);
	}
    }
    {
	int nops = slices * (imax - imin);
	FLOPS(1, 1, 0, 0);
	FLOPS(5 * nops, 2 * nops, 1 * nops, 0 * nops);
    }
}				// equation_of_state

// EOF
