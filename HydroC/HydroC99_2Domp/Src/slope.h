#ifndef SLOPE_H_INCLUDED
#define SLOPE_H_INCLUDED

void slope(const int n,
	   const int Hnvar,
	   const int Hnxyt,
	   const real_t Hslope_type,
	   const int slices, const int Hstep, real_t q[Hnvar][Hstep][Hnxyt],
	   real_t dq[Hnvar][Hstep][Hnxyt]);

#ifdef TARGET
// #pragma message "TARGET on SLOPE declare to(SLOPE)"
#pragma omp declare target to(slope)
#endif
#endif				// SLOPE_H_INCLUDED
