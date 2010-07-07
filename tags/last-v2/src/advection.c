/*****************************************************************************
 *
 *  advection.c
 *
 *  Computes advective order parameter fluxes from the current
 *  velocity field (from hydrodynamics) and the the current
 *  order parameter(s).
 *
 *  Fluxes are all computed at the interface of the control cells
 *  surrounding each lattice site. Unique face fluxes guarantee
 *  conservation of the order parameter.
 *
 *  To deal with Lees-Edwards boundaries positioned at x = constant
 *  we have to allow the 'east' face flux to be stored separately
 *  to the 'west' face flux. There's no effect in the y- or z-
 *  directions.
 *
 *  $Id: advection.c,v 1.2 2009-10-08 17:24:41 kevin Exp $
 *
 *  Edinburgh Soft Matter and Statistical Physics Group and
 *  Edinburgh Parallel Computing Centre
 *
 *  Kevin Stratford (kevin@epcc.ed.ac.uk)
 *  (c) The University of Edinburgh (2009)
 *
 *****************************************************************************/

#include <assert.h>

#include "pe.h"
#include "coords.h"
#include "leesedwards.h"
#include "lattice.h"
#include "phi.h"

extern double * phi_site;

/*****************************************************************************
 *
 *  advection_upwind
 *
 *  The advective fluxes are computed via first order upwind.
 * 
 *  The following are set (as for all the upwind routines):
 *
 *  fluxw  ('west') is the flux in x-direction between cells ic-1, ic
 *  fluxe  ('east') is the flux in x-direction between cells ic, ic+1
 *  fluxy           is the flux in y-direction between cells jc, jc+1
 *  fluxz           is the flux in z-direction between cells kc, kc+1
 *
 *****************************************************************************/

void advection_upwind(double * fluxe, double * fluxw,
		      double * fluxy, double * fluxz) {

  int nlocal[3];
  int ic, jc, kc;            /* Counters over faces */
  int index0, index1, n;
  int icm1, icp1;
  double u0[3], u1[3], u;
  double phi0;

  assert(fluxe);
  assert(fluxw);
  assert(fluxy);
  assert(fluxz);

  get_N_local(nlocal);

  for (ic = 1; ic <= nlocal[X]; ic++) {
    icm1 = le_index_real_to_buffer(ic, -1);
    icp1 = le_index_real_to_buffer(ic, +1);
    for (jc = 0; jc <= nlocal[Y]; jc++) {
      for (kc = 0; kc <= nlocal[Z]; kc++) {

	index0 = ADDR(ic, jc, kc);

	for (n = 0; n < nop_; n++) {

	  phi0 = phi_site[nop_*index0 + n];
	  hydrodynamics_get_velocity(index0, u0);

	  /* west face (icm1 and ic) */

	  index1 = ADDR(icm1, jc, kc);
	  hydrodynamics_get_velocity(index1, u1);
	  u = 0.5*(u0[X] + u1[X]);

	  if (u > 0.0) {
	    fluxw[nop_*index0 + n] = u*phi_site[nop_*index1 + n];
	  }
	  else {
	    fluxw[nop_*index0 + n] = u*phi0;
	  }

	  /* east face (ic and icp1) */

	  index1 = ADDR(icp1, jc, kc);
	  hydrodynamics_get_velocity(index1, u1);
	  u = 0.5*(u0[X] + u1[X]);

	  if (u < 0.0) {
	    fluxe[nop_*index0 + n] = u*phi_site[nop_*index1 + n];
	  }
	  else {
	    fluxe[nop_*index0 + n] = u*phi0;
	  }

	  /* y direction */

	  index1 = le_site_index(ic, jc+1, kc);
	  hydrodynamics_get_velocity(index1, u1);
	  u = 0.5*(u0[Y] + u1[Y]);

	  if (u < 0.0) {
	    fluxy[nop_*index0 + n] = u*phi_site[nop_*index1 + n];
	  }
	  else {
	    fluxy[nop_*index0 + n] = u*phi0;
	  }

	  /* z direction */

	  index1 = ADDR(ic, jc, kc+1);
	  hydrodynamics_get_velocity(index1, u1);
	  u = 0.5*(u0[Z] + u1[Z]);

	  if (u < 0.0) {
	    fluxz[nop_*index0 + n] = u*phi_site[nop_*index1 + n];
	  }
	  else {
	    fluxz[nop_*index0 + n] = u*phi0;
	  }
	}
	/* Next site */
      }
    }
  }

  return;
}

/*****************************************************************************
 *
 *  advection_upwind_third_order
 *
 *  Advective fluxes.
 *
 *  In fact, formally second order wave-number extended scheme
 *  folowing Li, J. Comp. Phys. 113 235--255 (1997).
 *
 *  The stencil is three points, biased in upwind direction,
 *  with weights a1, a2, a3.
 *
 *****************************************************************************/

void advection_upwind_third_order(double * fluxe, double * fluxw,
				  double * fluxy, double * fluxz) {

  int nlocal[3];
  int ic, jc, kc;
  int n;
  int index0, index1;
  int icm2, icm1, icp1, icp2;
  double u0[3], u1[3], u;

  const double a1 = -0.213933;
  const double a2 =  0.927865;
  const double a3 =  0.286067;

  get_N_local(nlocal);
  assert(nhalo_ >= 2);

  assert(fluxe);
  assert(fluxw);
  assert(fluxy);
  assert(fluxz);

  for (ic = 1; ic <= nlocal[X]; ic++) {
    icm2 = le_index_real_to_buffer(ic, -2);
    icm1 = le_index_real_to_buffer(ic, -1);
    icp1 = le_index_real_to_buffer(ic, +1);
    icp2 = le_index_real_to_buffer(ic, +2);
    for (jc = 0; jc <= nlocal[Y]; jc++) {
      for (kc = 0; kc <= nlocal[Z]; kc++) {

	index0 = ADDR(ic, jc, kc);
	hydrodynamics_get_velocity(index0, u0);

	/* west face (icm1 and ic) */

	index1 = ADDR(icm1, jc, kc);
	hydrodynamics_get_velocity(index1, u1);
	u = 0.5*(u0[X] + u1[X]);

	if (u > 0.0) {
	  for (n = 0; n < nop_; n++) {
	    fluxw[nop_*index0 + n] =
	      u*(a1*phi_site[nop_*ADDR(icm2,jc,kc) + n]
	       + a2*phi_site[nop_*index1 + n]
	       + a3*phi_site[nop_*index0 + n]);
	  }
	}
	else {
	  for (n = 0; n < nop_; n++) {
	    fluxw[nop_*index0 + n] =
	      u*(a1*phi_site[nop_*ADDR(icp1,jc,kc) + n]
	       + a2*phi_site[nop_*index0 + n]
	       + a3*phi_site[nop_*index1 + n]);
	  }
	}

	/* east face (ic and icp1) */

	index1 = ADDR(icp1, jc, kc);
	hydrodynamics_get_velocity(index1, u1);
	u = 0.5*(u0[X] + u1[X]);

	if (u < 0.0) {
	  for (n = 0; n < nop_; n++) {
	    fluxe[nop_*index0 + n] =
	      u*(a1*phi_site[nop_*ADDR(icp2,jc,kc) + n]
	       + a2*phi_site[nop_*index1 + n]
	       + a3*phi_site[nop_*index0 + n]);
	  }
	}
	else {
	  for (n = 0; n < nop_; n++) {
	    fluxe[nop_*index0 + n] =
	      u*(a1*phi_site[nop_*ADDR(icm1,jc,kc) + n]
	       + a2*phi_site[nop_*index0 + n]
	       + a3*phi_site[nop_*index1 + n]);
	  }
	}

	/* y direction */

	index1 = le_site_index(ic, jc+1, kc);
	hydrodynamics_get_velocity(index1, u1);
	u = 0.5*(u0[Y] + u1[Y]);

	if (u < 0.0) {
	  for (n = 0; n < nop_; n++) {
	    fluxy[nop_*index0 + n] =
	      u*(a1*phi_site[nop_*ADDR(ic,jc+2,kc) + n]
	       + a2*phi_site[nop_*index1 + n]
	       + a3*phi_site[nop_*index0 + n]);
	  }
	}
	else {
	  for (n = 0; n < nop_; n++) {
	    fluxy[nop_*index0 + n] =
	      u*(a1*phi_site[nop_*ADDR(ic,jc-1,kc) + n]
	       + a2*phi_site[nop_*index0 + n]
	       + a3*phi_site[nop_*index1 + n]);
	  }
	}

	/* z direction */

	index1 = ADDR(ic, jc, kc+1);
	hydrodynamics_get_velocity(index1, u1);
	u = 0.5*(u0[Z] + u1[Z]);

	if (u < 0.0) {
	  for (n = 0; n < nop_; n++) {
	    fluxz[nop_*index0 + n] =
	      u*(a1*phi_site[nop_*ADDR(ic,jc,kc+2) + n]
	       + a2*phi_site[nop_*index1 + n]
	       + a3*phi_site[nop_*index0 + n]);
	  }
	}
	else {
	  for (n = 0; n < nop_; n++) {
	    fluxz[nop_*index0 + n] =
	      u*(a1*phi_site[nop_*ADDR(ic,jc,kc-1) + n]
	       + a2*phi_site[nop_*index0 + n]
	       + a3*phi_site[nop_*index1 + n]);
	  }
	}

	/* Next site */
      }
    }
  }

  return;
}

/****************************************************************************
 *
 *  advection_upwind_fifth_order
 *
 *  Advective fluxes.
 *
 *  Formally fourth-order accurate wavenumber-extended scheme of
 *  Li, J. Comp. Phys. 133 235-255 (1997).
 *
 *  The stencil is five points, biased in the upwind direction,
 *  with weights a1--a5.
 *
 ****************************************************************************/

void advection_upwind_fifth_order(double * fluxe, double * fluxw,
				  double * fluxy, double * fluxz) {

  int nlocal[3];
  int ic, jc, kc;
  int n;
  int index0, index1;
  int icm3, icm2, icm1, icp1, icp2, icp3;
  double u0[3], u1[3], u;

  const double a1 =  0.055453;
  const double a2 = -0.305147;
  const double a3 =  0.916054;
  const double a4 =  0.361520;
  const double a5 = -0.027880;

  get_N_local(nlocal);
  assert(nhalo_ >= 3);

  assert(fluxe);
  assert(fluxw);
  assert(fluxy);
  assert(fluxz);

  for (ic = 1; ic <= nlocal[X]; ic++) {
    icm3 = le_index_real_to_buffer(ic, -3);
    icm2 = le_index_real_to_buffer(ic, -2);
    icm1 = le_index_real_to_buffer(ic, -1);
    icp1 = le_index_real_to_buffer(ic, +1);
    icp2 = le_index_real_to_buffer(ic, +2);
    icp3 = le_index_real_to_buffer(ic, +3);
    for (jc = 0; jc <= nlocal[Y]; jc++) {
      for (kc = 0; kc <= nlocal[Z]; kc++) {

	index0 = ADDR(ic, jc, kc);
	hydrodynamics_get_velocity(index0, u0);

	/* west face (icm1 and ic) */

	index1 = ADDR(icm1, jc, kc);
	hydrodynamics_get_velocity(index1, u1);
	u = 0.5*(u0[X] + u1[X]);

	if (u > 0.0) {
	  for (n = 0; n < nop_; n++) {
	    fluxw[nop_*index0 + n] =
	      u*(a1*phi_site[nop_*ADDR(icm3, jc, kc) + n]
	       + a2*phi_site[nop_*ADDR(icm2, jc, kc) + n]
	       + a3*phi_site[nop_*index1 + n]
	       + a4*phi_site[nop_*index0 + n]
	       + a5*phi_site[nop_*ADDR(icp1, jc, kc) + n]);
	  }
	}
	else {
	  for (n = 0; n < nop_; n++) {
	    fluxw[nop_*index0 + n] =
	      u*(a1*phi_site[nop_*ADDR(icp2, jc, kc) + n]
	       + a2*phi_site[nop_*ADDR(icp1, jc, kc) + n]
	       + a3*phi_site[nop_*index0 + n]
	       + a4*phi_site[nop_*index1 + n]
	       + a5*phi_site[nop_*ADDR(icm2, jc, kc) + n]);
	  }
	}

	/* east face */

	index1 = ADDR(icp1, jc, kc);
	hydrodynamics_get_velocity(index1, u1);
	u = 0.5*(u0[X] + u1[X]);

	if (u < 0.0) {
	  for (n = 0; n < nop_; n++) {
	    fluxe[nop_*index0 + n] =
	      u*(a1*phi_site[nop_*ADDR(icp3, jc, kc) + n]
	       + a2*phi_site[nop_*ADDR(icp2, jc, kc) + n]
	       + a3*phi_site[nop_*index1 + n]
	       + a4*phi_site[nop_*index0 + n]
	       + a5*phi_site[nop_*ADDR(icm1, jc, kc) + n]);
	  }
	}
	else {
	  for (n = 0; n < nop_; n++) {
	    fluxe[nop_*index0 + n] =
	      u*(a1*phi_site[nop_*ADDR(icm2, jc, kc) + n]
	       + a2*phi_site[nop_*ADDR(icm1, jc, kc) + n]
	       + a3*phi_site[nop_*index0 + n]
	       + a4*phi_site[nop_*index1 + n]
	       + a5*phi_site[nop_*ADDR(icp2, jc, kc) + n]);
	  }
	}

	/* y-direction */

	index1 = ADDR(ic, jc+1, kc);
	hydrodynamics_get_velocity(index1, u1);
	u = 0.5*(u0[Y] + u1[Y]);

	if (u < 0.0) {
	  for (n = 0; n < nop_; n++) {
	    fluxy[nop_*index0 + n] =
	      u*(a1*phi_site[nop_*ADDR(ic, jc+3, kc) + n]
	       + a2*phi_site[nop_*ADDR(ic, jc+2, kc) + n]
	       + a3*phi_site[nop_*index1 + n]
	       + a4*phi_site[nop_*index0 + n]
	       + a5*phi_site[nop_*ADDR(ic, jc-1, kc) + n]);
	  }
	}
	else {
	  for (n = 0; n < nop_; n++) {
	    fluxy[nop_*index0 + n] =
	      u*(a1*phi_site[nop_*ADDR(ic, jc-2, kc) + n]
	       + a2*phi_site[nop_*ADDR(ic, jc-1, kc) + n]
	       + a3*phi_site[nop_*index0 + n]
	       + a4*phi_site[nop_*index1 + n]
	       + a5*phi_site[nop_*ADDR(ic, jc+2, kc) + n]);
	  }
	}

	/* z-direction */

	index1 = ADDR(ic, jc, kc+1);
	hydrodynamics_get_velocity(index1, u1);
	u = 0.5*(u0[Z] + u1[Z]);

	if (u < 0.0) {
	  for (n = 0; n < nop_; n++) {
	    fluxz[nop_*index0 + n] =
	      u*(a1*phi_site[nop_*ADDR(ic, jc, kc+3) + n]
	       + a2*phi_site[nop_*ADDR(ic, jc, kc+2) + n]
	       + a3*phi_site[nop_*index1 + n]
	       + a4*phi_site[nop_*index0 + n]
	       + a5*phi_site[nop_*ADDR(ic, jc, kc-1) + n]);
	  }
	}
	else {
	  for (n = 0; n < nop_; n++) {
	    fluxz[nop_*index0 + n] =
	      u*(a1*phi_site[nop_*ADDR(ic, jc, kc-2) + n]
	       + a2*phi_site[nop_*ADDR(ic, jc, kc-1) + n]
	       + a3*phi_site[nop_*index0 + n]
	       + a4*phi_site[nop_*index1 + n]
	       + a5*phi_site[nop_*ADDR(ic, jc, kc+2) + n]);
	  }
	}

	/* Next interface. */
      }
    }
  }

  return;
}

/*****************************************************************************
 *
 *  advection_upwind_seventh_order
 *
 *  Advective fluxes.
 *
 *  Formally sixth order accurate wavenumber-extended scheme following
 *  Li J. Comp. Phys. 133 235--255 (1997).
 *
 *  The stencil is seven points, biased in the upwind direction,
 *  with weights a1--a7.
 *
 *****************************************************************************/

void advection_upwind_seventh_order(double * fluxe, double * fluxw,
				    double * fluxy, double * fluxz) {

  int nlocal[3];
  int ic, jc, kc;
  int icm1, icm2, icm3, icm4;
  int icp1, icp2, icp3, icp4;
  int index0, index1;
  int n;
  double u0[3], u1[3], u;

  const double a1 = -0.015825;
  const double a2 =  0.111617;
  const double a3 = -0.370709;
  const double a4 =  0.933168;
  const double a5 =  0.379291;
  const double a6 = -0.038383;
  const double a7 =  0.000842;

  get_N_local(nlocal);
  assert(nhalo_ >= 4);

  assert(fluxe);
  assert(fluxw);
  assert(fluxy);
  assert(fluxz);

  for (ic = 1; ic <= nlocal[X]; ic++) {
    icm4 = le_index_real_to_buffer(ic, -4);
    icm3 = le_index_real_to_buffer(ic, -3);
    icm2 = le_index_real_to_buffer(ic, -2);
    icm1 = le_index_real_to_buffer(ic, -1);
    icp1 = le_index_real_to_buffer(ic, +1);
    icp2 = le_index_real_to_buffer(ic, +2);
    icp3 = le_index_real_to_buffer(ic, +3);
    icp4 = le_index_real_to_buffer(ic, +4);
    for (jc = 0; jc <= nlocal[Y]; jc++) {
      for (kc = 0; kc <= nlocal[Z]; kc++) {

	index0 = ADDR(ic, jc, kc);
	hydrodynamics_get_velocity(index0, u0);
 
	/* x direction */

	index1 = ADDR(icp1, jc, kc);
	hydrodynamics_get_velocity(index1, u1);
	u = 0.5*(u0[X] + u1[X]);

	if (u > 0.0) {
	  for (n = 0; n < nop_; n++) {
	    fluxw[nop_*index0 + n] =
	      u*(a1*phi_site[nop_*ADDR(icm4, jc, kc) + n]
	       + a2*phi_site[nop_*ADDR(icm3, jc, kc) + n]
	       + a3*phi_site[nop_*ADDR(icm2, jc, kc) + n]
	       + a4*phi_site[nop_*ADDR(icm1, jc, kc) + n]
	       + a5*phi_site[nop_*index0 + n]
	       + a6*phi_site[nop_*index1 + n]
	       + a7*phi_site[nop_*ADDR(icp2, jc, kc) + n]);
	  }
	}
	else {
	  for (n = 0; n < nop_; n++) {
	    fluxw[nop_*index0 + n] =
	      u*(a1*phi_site[nop_*ADDR(icp3, jc, kc) + n]
	       + a2*phi_site[nop_*ADDR(icp2, jc, kc) + n]
	       + a3*phi_site[nop_*index1 + n]
	       + a4*phi_site[nop_*index0 + n]
	       + a5*phi_site[nop_*ADDR(icm1, jc, kc) + n]
	       + a6*phi_site[nop_*ADDR(icm2, jc, kc) + n]
	       + a7*phi_site[nop_*ADDR(icm3, jc, kc) + n]);
	  }
	}

	/* y-direction */

	index1 = ADDR(ic, jc+1, kc);
	hydrodynamics_get_velocity(index1, u1);
	u = 0.5*(u0[Y] + u1[Y]);

	if (u < 0.0) {
	  for (n = 0; n < nop_; n++) {
	    fluxy[nop_*index0 + n] =
	      u*(a1*phi_site[nop_*ADDR(ic, jc+4, kc) + n]
	       + a2*phi_site[nop_*ADDR(ic, jc+3, kc) + n]
	       + a3*phi_site[nop_*ADDR(ic, jc+2, kc) + n]
	       + a4*phi_site[nop_*index1 + n]
	       + a5*phi_site[nop_*index0 + n]
	       + a6*phi_site[nop_*ADDR(ic, jc-1, kc) + n]
	       + a7*phi_site[nop_*ADDR(ic, jc-2, kc) + n]);
	  }
	}
	else {
	  for (n = 0; n < nop_; n++) {
	    fluxy[nop_*index0 + n] =
	      u*(a1*phi_site[nop_*ADDR(ic, jc-3, kc) + n]
	       + a2*phi_site[nop_*ADDR(ic, jc-2, kc) + n]
	       + a3*phi_site[nop_*ADDR(ic, jc-1, kc) + n]
	       + a4*phi_site[nop_*index0 + n]
	       + a5*phi_site[nop_*index1 + n]
	       + a6*phi_site[nop_*ADDR(ic, jc+2, kc) + n]
	       + a7*phi_site[nop_*ADDR(ic, jc+3, kc) + n]);
	  }
	}

	/* z-direction */

	index1 = ADDR(ic, jc, kc+1);
	hydrodynamics_get_velocity(index1, u1);
	u = 0.5*(u0[Z] + u1[Z]);

	if (u < 0.0) {
	  for (n = 0; n < nop_; n++) {
	    fluxz[nop_*index0 + n] =
	      u*(a1*phi_site[nop_*ADDR(ic, jc, kc+4) + n]
	       + a2*phi_site[nop_*ADDR(ic, jc, kc+3) + n]
	       + a3*phi_site[nop_*ADDR(ic, jc, kc+2) + n]
	       + a4*phi_site[nop_*index1 + n]
	       + a5*phi_site[nop_*index0 + n]
	       + a6*phi_site[nop_*ADDR(ic, jc, kc-1) + n]
	       + a7*phi_site[nop_*ADDR(ic, jc, kc-2) + n]);
	  }
	}
	else {
	  for (n = 0; n < nop_; n++) {
	    fluxz[nop_*index0 + n] =
	      u*(a1*phi_site[nop_*ADDR(ic, jc, kc-3) + n]
	       + a2*phi_site[nop_*ADDR(ic, jc, kc-2) + n]
	       + a3*phi_site[nop_*ADDR(ic, jc, kc-1) + n]
	       + a4*phi_site[nop_*index0 + n]
	       + a5*phi_site[nop_*index1 + n]
	       + a6*phi_site[nop_*ADDR(ic, jc, kc+2) + n]
	       + a7*phi_site[nop_*ADDR(ic, jc, kc+3) + n]);
	  }
	}

	/* Next interface */
      }
    }
  }

  return;
}