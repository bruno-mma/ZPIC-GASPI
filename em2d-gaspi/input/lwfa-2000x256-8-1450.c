/**
 * ZPIC - em2d
 *
 * Laser Wakefield Acceleration
 */

#include <stdlib.h>
#include <math.h>

#include "../simulation.h"

void sim_init( t_simulation* sim ){

	// Time step
	float dt = 0.014;
	float tmax = 20.314;

	// Simulation box
	int   nx[2]  = { 2000, 256 };
	float box[2] = { 40.0, 51.2 };

	// Diagnostic frequency
	int ndump = 10;

    // Initialize particles
	const int n_species = 1;

	// Use 4x2 particles per cell
	int ppc[] = {4,2};

	// Density profile
	t_density density = { .type = STEP, .start = 20.0 };

	t_species* species = (t_species *) malloc( n_species * sizeof( t_species ));
	spec_new( &species[0], "electrons", -1.0, ppc, NULL, NULL, nx, box, dt, NULL);//&density );

	// Initialize Simulation data
	sim_new( sim, nx, box, dt, tmax, ndump, species, n_species, MOVING_WINDOW);

	// Add laser pulse (this must come after sim_new)
	t_emf_laser laser = {
		.type = GAUSSIAN,
		.start = 17.0,
		.fwhm  = 2.0,
		.a0 = 2.0,
		.omega0 = 10.0,
		.W0 = 4.0,
		.focus = 20.0,
		.axis = 12.8,
		.polarization = M_PI_2
    };
	sim_add_laser( sim, &laser );

	// Set current smoothing (this must come after sim_new)
	t_smooth smooth = {
		.xtype = COMPENSATED,
		.xlevel = 4
	};

	sim_set_smooth( sim, &smooth );
}


void sim_report( t_simulation* sim )
{
	// Jx, Jy, Jz
	current_report( &sim->current, 0 );
	current_report( &sim->current, 1 );
	current_report( &sim->current, 2 );

	// Bx, By, Bz
	emf_report( &sim->emf, BFLD, 0 );
	emf_report( &sim->emf, BFLD, 1 );
	emf_report( &sim->emf, BFLD, 2 );

	// Ex, Ey, Ez
	emf_report( &sim->emf, EFLD, 0 );
	emf_report( &sim->emf, EFLD, 1 );
	emf_report( &sim->emf, EFLD, 2 );
}
