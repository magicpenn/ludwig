/*****************************************************************************
 *
 *  test_hydro.c
 *
 *  Unit test for hydrodynamics object. Tests for the Lees Edwards
 *  transformations are sadly lacking.
 *
 *****************************************************************************/

#include <assert.h>
#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "pe.h"
#include "coords.h"
#include "leesedwards.h"
#include "io_harness.h"
#include "hydro_s.h"

#include "test_coords_field.h"

static int do_test1(void);
static int do_test_halo1(int nhalo, int nhcomm);
static int do_test_io1(void);

/*****************************************************************************
 *
 *  main
 *
 *****************************************************************************/

int main(int argc, char ** argv) {

  MPI_Init(&argc, &argv);
  pe_init();

  do_test1();
  do_test_halo1(1, 1);
  do_test_halo1(2, 2);
  do_test_halo1(2, 1);
  do_test_io1();

  pe_finalise();
  MPI_Finalize();

  return 0;
}

/*****************************************************************************
 *
 *  do_test1
 *
 *****************************************************************************/

static int do_test1(void) {

  hydro_t * hydro = NULL;

  int index;
  const double force[3] = {1.0, 2.0, 3.0};
  const double u[3] = {-1.0, -2.0, -3.0};
  double check[3] = {0.0, 0.0, 0.0};

  coords_init();
  le_init();

  hydro_create(1, &hydro);
  assert(hydro);

  index = coords_index(1, 1, 1);
  hydro_f_local_set(hydro, index, force);
  hydro_f_local(hydro, index, check);
  assert(fabs(force[X] - check[X]) < DBL_EPSILON);
  assert(fabs(force[Y] - check[Y]) < DBL_EPSILON);
  assert(fabs(force[Z] - check[Z]) < DBL_EPSILON);

  hydro_u_set(hydro, index, u);
  hydro_u(hydro, index, check);
  assert(fabs(u[X] - check[X]) < DBL_EPSILON);
  assert(fabs(u[Y] - check[Y]) < DBL_EPSILON);
  assert(fabs(u[Z] - check[Z]) < DBL_EPSILON);

  hydro_free(hydro);

  le_finish();
  coords_finish();

  return 0;
}

/*****************************************************************************
 *
 *  do_test_halo1
 *
 *****************************************************************************/

static int do_test_halo1(int nhalo, int nhcomm) {

  hydro_t * hydro = NULL;

  coords_nhalo_set(nhalo);
  coords_init();
  le_init();

  hydro_create(nhcomm, &hydro);
  assert(hydro);

  test_coords_field_set(hydro->nf, hydro->u, MPI_DOUBLE, test_ref_double1);
  hydro_u_halo(hydro);
  test_coords_field_check(nhcomm, hydro->nf, hydro->u, MPI_DOUBLE,
			  test_ref_double1);

  hydro_free(hydro);

  le_finish();
  coords_finish();

  return 0;
}

/*****************************************************************************
 *
 *  do_test_io1
 *
 *****************************************************************************/

static int do_test_io1(void) {

  int grid[3] = {1, 1, 1};
  char * filename = "hydro-test-io";
  io_info_t * iohandler = NULL;
  hydro_t * hydro;

  coords_init();
  le_init();

  if (pe_size() == 8) {
    grid[X] = 2;
    grid[Y] = 2;
    grid[Z] = 2;
  }

  hydro_create(1, &hydro);
  assert(hydro);

  hydro_init_io_info(hydro, grid, IO_FORMAT_DEFAULT, IO_FORMAT_DEFAULT);
  test_coords_field_set(hydro->nf, hydro->u, MPI_DOUBLE, test_ref_double1);

  hydro_io_info(hydro, &iohandler);
  assert(iohandler);

  io_write_data(iohandler, filename, hydro);

  MPI_Barrier(pe_comm());
  io_remove(filename, iohandler);
  io_remove_metadata(iohandler, "vel");

  hydro_free(hydro);
  le_finish();
  coords_finish();


  return 0;
}