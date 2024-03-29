#include "gaspi_aux.h"

extern int sim_decomp;

extern int proc_block_low[NUM_DIMS];
extern int proc_block_high[NUM_DIMS];
extern int proc_block_size[NUM_DIMS];

extern int proc_coords[NUM_DIMS];
extern bool is_on_edge[2];
extern int dims[NUM_DIMS];

extern gaspi_rank_t neighbour_rank[NUM_ADJ];
extern unsigned int neighbour_nx[NUM_ADJ][NUM_DIMS];

extern gaspi_rank_t num_procs;
extern gaspi_rank_t proc_rank;

int cart_rank(int coords[NUM_DIMS])
{
	return coords[0] + (coords[1] * dims[0]);
}

// also sets is_on_edge
void cart_coords(int rank, int coords[NUM_DIMS])
{
	coords[0] = rank % dims[0];
	coords[1] = rank / dims[0];

	is_on_edge[0] = coords[0] == 0;
	is_on_edge[1] = coords[0] == dims[0] - 1;
}

void create_dims(const int num_procs, const int sim_decomp, const int nx[NUM_DIMS])
{
	int num_factors;
	int* factors = get_factors(num_procs, &num_factors);

	assign_factors(factors, num_factors, dims, sim_decomp, nx);
	free(factors);
}

int* get_factors(int num, int* num_factors)
{
	if(num < 2)
	{
		(*num_factors) = 0;
		return NULL;
	}

	int num_sqrt = ceil(sqrt(num));
	int size = ceil(log2(num));
	int* factors = (int *) malloc((unsigned) size * sizeof(int));

	int i = 0;

	// occurances of factor 2
	while((num % 2) == 0)
	{
		num /= 2;
		factors[i++] = 2;
	}

	// occurances of odd numbers up to sqrt(num)
	for(int d = 3; (num > 1) && (d <= num_sqrt); d += 2)
	{
		while((num % d) == 0)
		{
			num /= d;
			factors[i++] = d;
		}
	}

	//add last factor
	if(num != 1)
	{
		factors[i++] = num;
	}

	(*num_factors) = i;

	return factors;
}

void assign_factors(const int* factors, const int num_factors, int dims[NUM_DIMS], const int sim_decomp, const int nx[NUM_DIMS])
{
	// Max of divisions allowed on the y axis
	// Each proc must have at least 2 lines
	const int max_blocks_y = nx[1] / 2;

	dims[0] = 1;
	dims[1] = 1;

	// assign factors from highest to lowest
	for (int i = num_factors - 1; i >= 0; i--)
	{
		// assign to dimention with the lowest number of divisions
		// if using row decomposition, assign to y axis when possible
		if ( (sim_decomp == DECOMP_ROW && (dims[1] * factors[i] <= max_blocks_y)) || (sim_decomp == DECOMP_CHECKERBOARD && dims[1] <= dims[0]) )
			dims[1] *= factors[i];
		
		else
			dims[0] *= factors[i];
	}

	// make sure the y axis has more divisions
	if (dims[0] > dims[1] && dims[0] <= max_blocks_y)
	{
		int aux = dims[0];
		dims[0] = dims[1];
		dims[1] = aux;
	}
}

void assign_proc_blocks(const int nx[NUM_DIMS])
{
	static bool blocks_set = 0;

	if (!blocks_set)
	{
		//only need to assign the blocks once
		blocks_set = true;

		create_dims(num_procs, sim_decomp, nx);
		cart_coords(proc_rank, proc_coords);

		if (proc_rank == ROOT)
		{
			printf("Dims:[%d,%d] with %d procs\n", dims[0], dims[1], num_procs);
		}

		proc_block_low[0] = BLOCK_LOW(proc_coords[0], dims[0], nx[0]);
		proc_block_low[1] = BLOCK_LOW(proc_coords[1], dims[1], nx[1]);

		proc_block_high[0] = BLOCK_HIGH(proc_coords[0], dims[0], nx[0]);
		proc_block_high[1] = BLOCK_HIGH(proc_coords[1], dims[1], nx[1]);

		proc_block_size[0] = BLOCK_SIZE(proc_coords[0], dims[0], nx[0]);
		proc_block_size[1] = BLOCK_SIZE(proc_coords[1], dims[1], nx[1]);
	}
}

// periodic boundary enforcer. If a coord leaves its allowed range, enforce a periodic boundary
int periodic_coord(int coord, int coord_max)
{
	if(coord < 0)
		return coord + coord_max;

	if(coord >= coord_max)
		return coord - coord_max;

	return coord;
}

// populate the neighbour_rank array with the rank on each direction and save their simulation space size
void discover_neighbours(int proc_coords[NUM_DIMS], int dims[NUM_DIMS], int nx[NUM_DIMS])
{
	int dir = 0;

	for (int y = 1; y >= -1; y--)
	{
		for (int x = -1; x <= 1; x++)
		{
			if(y == 0 && x == 0)
				continue;

			int new_x = periodic_coord(x + proc_coords[0], dims[0]);
			int new_y = periodic_coord(y + proc_coords[1], dims[1]);

			int coords[NUM_DIMS] = {new_x, new_y};

			for (int dim = 0; dim < NUM_DIMS; dim++)
			{
				neighbour_nx[dir][dim] = BLOCK_SIZE(coords[dim], dims[dim], nx[dim]);
			}

			neighbour_rank[dir++] = cart_rank(coords);
		}
	}
}

// returns true if this proc can send/receive data to/from neighbour at direction dir, false otherwise
bool can_talk_to_dir(const bool moving_window, const int dir)
{
	// restrictions only apply to moving window simulations
	if ( !moving_window )
		return true;
	
	// if proc is on the left edge of the simulation space, dont send data to the left
	if ( is_on_edge[0] && (dir == UP_LEFT || dir == LEFT || dir == DOWN_LEFT) )
		return false;
	
	// if proc is on the right edge of the simulation space, dont send data to the right
	if ( is_on_edge[1] && (dir == UP_RIGHT || dir == RIGHT || dir == DOWN_RIGHT) )
		return false;
	
	return true;
}

// returns the number of dirs that this proc will receive notifs from
int get_num_incoming_notifs(const bool moving_window)
{
	// On static window simulations
	if ( !moving_window )
		// will receive cell data from every dir
		return NUM_ADJ;

	// If proc is on both the right and left edges of the simulation space
	if ( is_on_edge[0] && is_on_edge[1] )
		// will only receive cell data from UP and DOWN dirs
		return NUM_ADJ - 6;
	
	// If proc is either on the left or right edge of the simulation space
	if ( is_on_edge[0] || is_on_edge[1] )
		// will not receive data from the dirs on the edge of the simulation space
		return NUM_ADJ - 3;
	
	return NUM_ADJ;
}