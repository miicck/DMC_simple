/*
    DMCPP
    Copyright (C) 2019 Michael Hutcheon (email mjh261@cam.ac.uk)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    For a copy of the GNU General Public License see <https://www.gnu.org/licenses/>.
*/

#include "params.h"
#include "particle.h"
#include "walker_collection.h"
#include "random.h"
#include "constants.h"

// Run the DMC calculation
void run_dmc()
{
    // Our DMC walkers
    params::progress_file << "Initializing walkers\n";
    walker_collection walkers;
    
    // Run our DMC iterations
    params::progress_file << "Starting DMC simulation\n";
    params::progress_file << "    Total setup time: " << params::time() << "s\n";
    params::dmc_start_clock = clock();

    for (params::dmc_iteration = 1;
         params::dmc_iteration <= params::dmc_iterations;
         params::dmc_iteration ++)
    {
        // Apply propagation of walkers
        walkers.propagate();
        walkers.write_output();
    }

    // Output success message
    params::progress_file << "\nDone, total time: " << params::time() << "s.\n";
}

// Program entrypoint
int main(int argc, char** argv)
{
    // Used for timing info
    params::start_clock = clock();

    // Read input files, ready output files, initialize MPI etc.
    params::load(argc, argv);

    // Run the DMC simulation
    run_dmc();

    // Free memory used in the simulation specification
    params::free_memory();
}
