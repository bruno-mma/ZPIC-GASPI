/*
 *  current.h
 *  zpic
 *
 *  Created by Ricardo Fonseca on 12/8/10.
 *  Copyright 2010 Centro de Física dos Plasmas. All rights reserved.
 *
 */

#ifndef __CURRENT__
#define __CURRENT__

#include "zpic.h"

enum smooth_type { NONE, BINOMIAL, COMPENSATED };

typedef struct {
	enum smooth_type xtype, ytype;
	int xlevel, ylevel;
} t_smooth;

typedef struct {
	
	t_vfld *J;
	
	t_vfld *J_buff;

	// J_buff size in bytes
	size_t J_buff_size;
	
	// Grid parameters
	int nx[NUM_DIMS];

	//number of local cells on each axis, excluding guard cells
	int nx_local[NUM_DIMS];

	// Number of rows on each line of the global simulation space
	int nrow;
	
	// Number of rows on each line on this proc
	int nrow_local;

	// Moving window
	bool moving_window;
	
	// Box size
	t_fld box[NUM_DIMS];
	
	// Cell size
	t_fld dx[NUM_DIMS];

	// Current smoothing
	t_smooth smooth;

	// Time step
	float dt;

	// Iteration number
	int iter;

} t_current;

typedef struct {
	
	t_vfld* J_buff;
	size_t J_buff_size; 			// in bytes
	unsigned int J_buff_num_cells;

	t_vfld* J; 						// Points to cell [0][0]

} t_current_reduce;


void send_current(t_current* current);

void current_new(t_current *current, const int nx[NUM_DIMS], const int nx_local[NUM_DIMS], const t_fld box[NUM_DIMS], const float dt, const bool moving_window);
void current_delete( t_current *current );
void current_zero( t_current *current );
void current_update( t_current *current );
void current_report( const t_current *current, const char jc );
void current_smooth( t_current* const current );
void curr_set_smooth(t_current* current, t_smooth* smooth);

void wait_save_update_current(t_current* current);

void set_current_reduce_out(t_current* current);
void alloc_private_current_reduce(t_current* current);
t_current_reduce reset_thread_current();
void add_thread_current(t_current_reduce current_in, t_current_reduce current_out);

void print_local_current(t_current* current);

#endif
