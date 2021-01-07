#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <stdio.h>

#include "parametres.h"
#include "utils.h"
#include "conservar.h"
#include "perfcnt.h"

#define BLOCKING 0
#define SSST 32
#define JJST 32

void
gatherConservativeVars(const int idim,
		       const int rowcol,
		       const int Himin,
		       const int Himax,
		       const int Hjmin,
		       const int Hjmax,
		       const int Hnvar,
		       const int Hnxt,
		       const int Hnyt,
		       const int Hnxyt,
		       const int slices, const int Hstep,
		       real_t uold[Hnvar * Hnxt * Hnyt],
		       real_t u[Hnvar][Hstep][Hnxyt]
    )
{
    int i, j, ivar, s;

#define IHU(i, j, v)  ((i) + Hnxt  * ((j) + Hnyt  * (v)))
#define IHST(v,s,i)   ((i) + Hstep * ((j) + Hnvar * (v)))

    WHERE("gatherConservativeVars");
    if (idim == 1) {
	// Gather conservative variables
#ifdef TARGETON
#pragma message "TARGET on GATHERCONSERVATIVEVARS"
#pragma omp target				\
	map(from:u[0:Hnvar][0:Hstep][Himin:Himax])	\
	map(to:uold[0:Hnvar *Hnxt * Hnyt])
#pragma omp teams distribute parallel for default(none) private(s, i), shared(u, uold) collapse(2)
#else
#pragma omp parallel for private(i, s), shared(u) COLLAPSE
#endif
	for (s = 0; s < slices; s++) {
	    for (i = Himin; i < Himax; i++) {
		int idxuoID = IHU(i, rowcol + s, ID);
		u[ID][s][i] = uold[idxuoID];

		int idxuoIU = IHU(i, rowcol + s, IU);
		u[IU][s][i] = uold[idxuoIU];

		int idxuoIV = IHU(i, rowcol + s, IV);
		u[IV][s][i] = uold[idxuoIV];

		int idxuoIP = IHU(i, rowcol + s, IP);
		u[IP][s][i] = uold[idxuoIP];
	    }
	}

	if (Hnvar > IP) {
	    for (ivar = IP + 1; ivar < Hnvar; ivar++) {
		for (s = 0; s < slices; s++) {
		    for (i = Himin; i < Himax; i++) {
			u[ivar][s][i] = uold[IHU(i, rowcol + s, ivar)];
		    }
		}
	    }
	}
	//
    } else {
	// Gather conservative variables
#ifdef TARGETON
#pragma message "TARGET on GATHERCONSERVATIVEVARS"
#pragma omp target				\
	map(from:u[0:Hnvar][0:Hstep][Himin:Himax])	\
	map(to:uold[0:Hnvar *Hnxt * Hnyt])
#pragma omp teams distribute parallel for default(none) private(s, i), shared(u, uold) collapse(2)
#else
#pragma omp parallel for private(j, s), shared(u)
#endif
	for (s = 0; s < slices; s++) {
	    for (j = Hjmin; j < Hjmax; j++) {
		u[ID][s][j] = uold[IHU(rowcol + s, j, ID)];
		u[IU][s][j] = uold[IHU(rowcol + s, j, IV)];
		u[IV][s][j] = uold[IHU(rowcol + s, j, IU)];
		u[IP][s][j] = uold[IHU(rowcol + s, j, IP)];
	    }
	}

	if (Hnvar > IP) {
	    for (ivar = IP + 1; ivar < Hnvar; ivar++) {
		for (s = 0; s < slices; s++) {
		    for (j = Hjmin; j < Hjmax; j++) {
			u[ivar][s][j] = uold[IHU(rowcol + s, j, ivar)];
		    }
		}
	    }
	}
    }
}

#undef IHU

void
updateConservativeVars(const int idim,
		       const int rowcol,
		       const real_t dtdx,
		       const int Himin,
		       const int Himax,
		       const int Hjmin,
		       const int Hjmax,
		       const int Hnvar,
		       const int Hnxt,
		       const int Hnyt,
		       const int Hnxyt,
		       const int slices, const int Hstep,
		       real_t uold[Hnvar * Hnxt * Hnyt],
		       real_t u[Hnvar][Hstep][Hnxyt],
		       real_t flux[Hnvar][Hstep][Hnxyt]
    )
{
    int i, j, ivar, s;
    WHERE("updateConservativeVars");

#define IHU(i, j, v)  ((i) + Hnxt * ((j) + Hnyt * (v)))

    if (idim == 1) {

	// Update conservative variables
#ifdef TARGETON
#pragma message "TARGET on UPDATECONSERVATIVEVARS"
#pragma omp target				\
	map(tofrom:u[0:Hnvar][0:Hstep][0:Hnxyt])	\
	map(tofrom:flux[0:Hnvar][0:Hstep][0:Hnxyt])	\
	map(tofrom:uold[0:Hnvar * Hnxt * Hnyt])
#pragma omp teams distribute parallel for default(none) private(s, i, ivar), shared(u, uold, flux) collapse(2)
#else
#pragma omp parallel for private(ivar, s,i), shared(uold) COLLAPSE
#endif
	for (s = 0; s < slices; s++) {
	    for (ivar = 0; ivar <= IP; ivar++) {
		for (i = Himin + ExtraLayer; i < Himax - ExtraLayer; i++) {
		    uold[IHU(i, rowcol + s, ivar)] =
			u[ivar][s][i] + (flux[ivar][s][i - 2] -
					 flux[ivar][s][i - 1]) * dtdx;
		}
	    }
	}
	{
	    int nops =
		(IP + 1) * slices * ((Himax - ExtraLayer) -
				     (Himin + ExtraLayer));
	    FLOPS(3 * nops, 0 * nops, 0 * nops, 0 * nops);
	}

	if (Hnvar > IP) {
	    for (ivar = IP + 1; ivar < Hnvar; ivar++) {
		for (s = 0; s < slices; s++) {
		    for (i = Himin + ExtraLayer; i < Himax - ExtraLayer; i++) {
			uold[IHU(i, rowcol + s, ivar)] =
			    u[ivar][s][i] + (flux[ivar][s][i - 2] -
					     flux[ivar][s][i - 1]) * dtdx;
		    }
		}
	    }
	}
    } else {
	// Update conservative variables
#ifdef TARGETON
#pragma message "TARGET on UPDATECONSERVATIVEVARS"
#pragma omp target				\
	map(tofrom:u[0:Hnvar][0:Hstep][0:Hnxyt])	\
	map(tofrom:flux[0:Hnvar][0:Hstep][0:Hnxyt])	\
	map(tofrom:uold[0:Hnvar * Hnxt * Hnyt])
#pragma omp teams distribute parallel for default(none) private(s, i, ivar), shared(u, uold, flux) collapse(2)
#else
#pragma omp parallel for private(j, s), shared(uold)
#endif
	for (s = 0; s < slices; s++) {
	    for (j = (Hjmin + ExtraLayer); j < (Hjmax - ExtraLayer); j++) {
		uold[IHU(rowcol + s, j, ID)] =
		    u[ID][s][j] + (flux[ID][s][j - 2] -
				   flux[ID][s][j - 1]) * dtdx;
		uold[IHU(rowcol + s, j, IV)] =
		    u[IU][s][j] + (flux[IU][s][j - 2] -
				   flux[IU][s][j - 1]) * dtdx;
		uold[IHU(rowcol + s, j, IU)] =
		    u[IV][s][j] + (flux[IV][s][j - 2] -
				   flux[IV][s][j - 1]) * dtdx;
		uold[IHU(rowcol + s, j, IP)] =
		    u[IP][s][j] + (flux[IP][s][j - 2] -
				   flux[IP][s][j - 1]) * dtdx;
	    }
	}
	{
	    int nops = slices * ((Hjmax - ExtraLayer) - (Hjmin + ExtraLayer));
	    FLOPS(12 * nops, 0 * nops, 0 * nops, 0 * nops);
	}

	if (Hnvar > IP) {
	    for (ivar = IP + 1; ivar < Hnvar; ivar++) {
		for (s = 0; s < slices; s++) {
		    for (j = Hjmin + ExtraLayer; j < Hjmax - ExtraLayer; j++) {
			uold[IHU(rowcol + s, j, ivar)] =
			    u[ivar][s][j] + (flux[ivar][s][j - 2] -
					     flux[ivar][s][j - 1]) * dtdx;
		    }
		}
	    }
	}
    }
}

#undef IHU
//EOF
