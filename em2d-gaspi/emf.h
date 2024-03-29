/*
 *  emf.h
 *  zpic
 *
 *  Created by Ricardo Fonseca on 10/8/10.
 *  Copyright 2010 Centro de Física dos Plasmas. All rights reserved.
 *
 */

#ifndef __EMF__
#define __EMF__


#include "zpic.h"

#include "current.h"

enum emf_diag { EFLD, BFLD };

typedef struct {
	
	t_vfld *E;
	t_vfld *B;
	
	t_vfld *E_buff;
	t_vfld *B_buff;
	
	// Simulation box info
	int nx[NUM_DIMS];
	t_fld box[NUM_DIMS];
	t_fld dx[NUM_DIMS];

	// Number of local cells on each axis, excluding guard cells
	int nx_local[NUM_DIMS];

	int buff_offset;

	// Number of rows on each line of the global simulation space
	int nrow;
	// Number of rows on each line on this proc
	int nrow_local;

	// Time step
	float dt;

	// Iteration number
	int iter;

	// Moving window
	bool moving_window;
	int n_move;
	
} t_emf;

enum emf_laser_type{ PLANE, GAUSSIAN };

typedef struct {

	enum emf_laser_type type;		// Laser pulse type
	
	float start;	// Front edge of the laser pulse, in simulation units
	float fwhm;		// FWHM of the laser pulse duration, in simulation units
	float rise, flat, fall; // Rise, flat and fall time of the laser pulse, in simulation units 
	
	float a0;		// Normalized peak vector potential of the pulse
	float omega0;	// Laser frequency, normalized to the plasma frequency
	
	float polarization; 
	
	float W0;		// Gaussian beam waist, in simulation units
	float focus;	// Focal plane position, in simulation units
	float axis;     // Position of optical axis, in simulation units
	
} t_emf_laser;

void emf_get_energy( const t_emf *emf, double energy[] );

void emf_new( t_emf *emf, const int nx[], const int nx_local[], const t_fld box[], const float dt, const bool moving_window);
void emf_delete( t_emf *emf );
void emf_report( const t_emf *emf, const char field, const char fc );

void emf_add_laser( t_emf* const emf, t_emf_laser* laser );

void emf_advance( t_emf *emf, const t_current *current );

void emf_move_window( t_emf *emf );

void emf_update_gc( t_emf *emf );

void send_emf_gc(t_emf* emf, const bool moving_window_iter);
void wait_save_emf_gc(t_emf* emf, const bool moving_window_iter);

void yee_b(t_emf *emf);
void yee_e(t_emf *emf, const t_current *current);

void print_emf_e(t_emf* emf);
void print_emf_b(t_emf* emf);

#endif
