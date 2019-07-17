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

#include <sstream>
#include <mpi.h>

#include "random.h"
#include "math.h"
#include "walker_collection.h"
#include "simulation.h"

double trial_wavefunction(walker* w, walker_collection* walkers)
{
        double r02 = w->sq_distance_to_origin();
        //return exp(-r02/2.0); // Harmonic oscillator
        return exp(-sqrt(r02)); // Hydrogen
        //return exp(-r02/5.0);
        //double rs  = sqrt(r02) - 1;
        //return exp(-rs*rs/2);
        return 1.0;
        
        // Evaluate the trial wavefunction at the configuration
        // of the given walker
        double wfn  = 0;
        double norm = 0;
        for (int n=0; n<walkers->size(); ++n)
        {       
                walker* wn = (*walkers)[n];
                if (wn == w) continue;
                double r2    = w->sq_distance_to(wn);
                double decay = exp(-sqrt(r2)/(2*simulation.tau));
                wfn  += wn->weight * decay;
                norm += decay;
        }
        return wfn/norm;
}

double trial_local_kinetic(walker* w, walker_collection* walkers)
{
        // Evaluate the local energy of the trial wavefunction
        // at the configuration w

        // Evaluate the kinetic energy of psi_t
        // at my configuration using finite differences
        double laplacian = 0;
        double psi_x = trial_wavefunction(w, walkers);

        // Use central finite differences to
        // calculate the laplacian
        for (unsigned i=0; i<w->particles.size(); ++i)
                for (unsigned j=0; j<simulation.dimensions; ++j)
                {
                        // Set coordinate to + EPS and evaluate
                        w->particles[i]->coords[j] += FD_EPS;
                        double psi_xplus  = trial_wavefunction(w, walkers);

                        // Set coordinate to - EPS and evaluate
                        w->particles[i]->coords[j] -= 2*FD_EPS;
                        double psi_xminus = trial_wavefunction(w, walkers);

                        // Reset coorindate and accumulate laplacian
                        w->particles[i]->coords[j] += FD_EPS;
                        laplacian += (psi_xplus - 2*psi_x + psi_xminus)/(FD_EPS*FD_EPS);
                }

        // Local K.E = \psi^{-1}(w)(-laplacian(\psi(w))/2)
        return -laplacian/(2*psi_x);
}

double trial_local_energy(walker* w, walker_collection* walkers)
{
        // Get the local energy evaluated at the walker position w
        return trial_local_kinetic(w, walkers) + w->potential();
}

void apply_drift(walker* w, walker_collection* walkers)
{
        // Apply the drift velocity to the walker w
        // by taking w -> w + \tau*v_d

        double psi_w = trial_wavefunction(w, walkers);

        // Use finite differences to evaluate drift
        for (unsigned i=0; i<w->particles.size(); ++i)
                for (unsigned j=0; j<simulation.dimensions; ++j)
                {
                        // Evaluate drift velocity of i^th particle
                        // in the j^th direction as
                        // v_{ij} = psi_T(w)^{-1} d/dx_{ij} psi_T(w)
                        w->particles[i]->coords[j] += FD_EPS;
                        double psi_wplus = trial_wavefunction(w, walkers);
                        w->particles[i]->coords[j] -= FD_EPS;
                        double vij = (psi_wplus - psi_w)/(FD_EPS*psi_w);

                        // Apply drift
                        w->particles[i]->coords[j] += vij * simulation.tau;
                }
}

walker_collection :: walker_collection()
{
        simulation.progress_file << "Initializing walkers\n";
        simulation.progress_file << "    Metropolis sampling trial wavefunction... ";
        double time_start = simulation.time();
        
	// Reserve a reasonable amount of space to deal efficiently
	// with the fact that the population can fluctuate.
	walkers.reserve(simulation.target_population*4);

        // Sample the walkers using metropolis monte carlo on 
        // the trial wavefunction
        walker* current = new walker();

        double step_size    = 10.0/double(simulation.target_population);
        int accepted        = 0;
        int rejected        = 0;
        current->diffuse(1.0);

        for (int i=0; i<simulation.target_population; ++i)
        {
                // Evaluate wfn at current configuration
                double psi_before = trial_wavefunction(current, this);

                // Create a moved version and evaluate wfn there
                walker* moved = current->copy();
                moved->diffuse(step_size);
                double psi_after = trial_wavefunction(moved, this);

                double accept = fabs(psi_after)/fabs(psi_before);
                if (rand_uniform() < accept)
                {
                        // Accept the move
                        delete current;
                        current = moved;
                        ++ accepted;
                }
                else
                {
                        // Reject the move
                        delete moved;
                        ++ rejected;
                }

                walkers.push_back(current->copy());
        }
        double time = simulation.time() - time_start;
        double accept_ratio = double(accepted)/double(accepted+rejected);
        simulation.progress_file << "done in " << time << "s\n";
        simulation.progress_file << "    Acceptance ratio = " << accept_ratio*100  
                                 << "% (optimal = 23.4%)\n"
                                 << "    Final step size  = " << step_size << "\n";

        delete current;
}

walker_collection :: ~walker_collection()
{
	// Clean up memory
	for (int n=0; n<size(); ++n)
		delete (*this)[n];
}

double branching_greens_function(double local_e_before, double local_e_after)
{
	// Evaluate the potential-dependent part of the greens
	// function G_v(x,x',tau) = exp(-tau*(v(x)+v(x')-2E_T)/2)
        if (std::isinf(local_e_after))  return 0;
        if (std::isinf(local_e_before)) return 0;

        double av_energy = (local_e_before + local_e_after)/2;
        double exponent  = -simulation.tau*(av_energy - simulation.trial_energy);
	return exp(exponent);
}

int branch_from_weight(double weight)
{
	// Returns how many walkers should be produced
	// from one walker of the given weight
	int num = (int)floor(fabs(weight) + rand_uniform());
	return std::min(num, 3);
}

void expectation_values :: reset()
{
        // Reset the expectation values
        // to begin accumulation
        local_energy = 0;
}

void walker_collection :: diffuse_and_branch()
{
        // Carry out diffusion and branching of the walkers
	int nmax = size();
        this->expect_vals.reset();

	for (int n=0; n < nmax; ++n)
	{
		walker* w = (*this)[n];

                // Diffuse each walker and apply the drift velocity
		// (recording the local energy before/after)
                double local_energy_before = trial_local_energy(w, this);

		w->diffuse(simulation.tau);
                apply_drift(w, this);

                double local_energy_after  = trial_local_energy(w, this);

		// Multiply the weight by the potential-
		// dependent branching part of the greens function
		w->weight *= branching_greens_function(
                        local_energy_before,
                        local_energy_after);

		// Apply a branching step, adding branched
		// survivors to the end of the collection
		int surviving = branch_from_weight(w->weight);
		for (int s=0; s<surviving; ++s)
			walkers.push_back(w->branch_copy());

                // Accumulate expectation values
                this->expect_vals.local_energy += local_energy_after * surviving;
	}

	// Delete the previous iterations walkers
	for (int n=0; n < nmax; ++n)
	{
		// Replace the nth walker with the
		// last walker, allowing us to shorten
		// the array from the end (delete the
		// nth walker in the process)
		delete walkers[n];
		walkers[n] = walkers.back();
		walkers.pop_back();
	}

        // Normalize expectation values
        this->expect_vals.local_energy /= double(walkers.size());

	// Set the trial energy to control population
        double pop_ratio = 
                double(walkers.size())/
                double(simulation.target_population);
	double log_pop_ratio = log(pop_ratio);

	simulation.trial_energy = this->expect_vals.local_energy - log_pop_ratio;
}

double walker_collection :: sum_mod_weight()
{
	// Returns sum_i |w_i|
	// This is the effective population
	double sum = 0;
	for (int n=0; n<size(); ++n)
		sum += fabs((*this)[n]->weight);
	return sum;
}

double walker_collection :: average_mod_weight()
{
	// Returns (1/N) * sum_i |w_i|
	return sum_mod_weight()/double(size());
}

double walker_collection :: average_weight()
{
	// Returns (1/N) * sum_i w_i
	double av_weight = 0;
	for (int n=0; n<size(); ++n)
		av_weight += (*this)[n]->weight;
	av_weight /= double(size());
	return av_weight;
}

double walker_collection :: average_potential()
{
	// Returns (1/N) * sum_i |w_i|*v_i
	// Where v_i is the potential energy 
	// of the i^th walker.
	double energy = 0;
	for (int n=0; n<size(); ++n)
		energy += (*this)[n]->potential() * fabs((*this)[n]->weight);
	energy /= double(size());
	return energy;
}

void walker_collection :: make_exchange_moves()
{
	// Apply exchange moves to each of the walkers
	for (int n=0; n<size(); ++n)
		(*this)[n]->exchange();
}

void walker_collection :: apply_cancellations()
{
        // Record weights and cancellation probabilities
	double*  weights_before = new double [size()];
        for (int n=0; n<size(); ++n)
                weights_before[n] = (*this)[n]->weight;

        // Apply all pair-wise cancellations
	for (int n=0; n<size(); ++n)
        {
                walker* wn = (*this)[n];

                for (int m=0; m<n; ++m)
                {
                        walker* wm = (*this)[m];
                        double cp  = wn->cancel_prob(wm);
                        
                        // Cancel these walkers with probability cp
                        wn->weight *= 1.0 - cp;
                        wm->weight *= 1.0 - cp;
                }
        }

	// Evaluate a measure of the amount of cancellation
	// that occured
	cancellation_amount_last = 0;
	for (int n=0; n<size(); ++n)
	{
		double delta = (*this)[n]->weight - weights_before[n];
		cancellation_amount_last += fabs(delta);
	}

        // Renormalize walkers
        double av_mod_w = this->average_mod_weight();
        for (int  n=0; n<size(); ++n)
                (*this)[n]->weight /= av_mod_w;

        // Clean up memory
        delete[] weights_before;
}

void walker_collection :: write_output(int iter)
{
        // Sum up walkers across processes
        int population = this->size();
        int population_red;
        MPI_Reduce(&population, &population_red, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

        // Average trial energy across processes
        double triale = simulation.trial_energy;
        double triale_red;
        MPI_Reduce(&triale, &triale_red, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
        triale_red /= double(simulation.np);

        // Avarage <weight> across processes
        double av_weight = this->average_weight();
        double av_weight_red;
        MPI_Reduce(&av_weight, &av_weight_red, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
        av_weight_red /= double(simulation.np);

        // Average <|weight|> across processes
        double av_mod_weight = this->average_mod_weight();
        double av_mod_weight_red;
        MPI_Reduce(&av_mod_weight, &av_mod_weight_red, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
        av_mod_weight_red /= double(simulation.np);

        // Average local energy across processes
        double av_loc_e = this->expect_vals.local_energy;
        double av_loc_e_red;
        MPI_Reduce(&av_loc_e, &av_loc_e_red, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
        av_loc_e_red /= double(simulation.np);

	// Sum cancellation amount across processes
	double cancel = this->cancellation_amount_last;
	double cancel_red;
	MPI_Reduce(&cancel, &cancel_red, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);

	// Calculate timing information
	double time_per_iter     = simulation.time()/iter;
	double seconds_remaining = time_per_iter * (simulation.dmc_iterations - iter);
	double percent_complete  = double(100*iter)/simulation.dmc_iterations;

        // Output iteration information
        simulation.progress_file << "\nIteration " << iter << "/" << simulation.dmc_iterations
				 << " (" << percent_complete << "%)\n";
	simulation.progress_file << "    Time running    : " << simulation.time()
				 << "s (" << time_per_iter << "s/iter)\n";
	simulation.progress_file << "    ETA             : "
				 << seconds_remaining << "s \n";
        simulation.progress_file << "    Trial energy    : " << triale_red     << " Hartree\n";
        simulation.progress_file << "    <E_L>           : " << av_loc_e_red   << " Hartree\n";
        simulation.progress_file << "    Population      : " << population_red << "\n";
	simulation.progress_file << "    Canceled weight : " << cancel_red     << "\n";

        if (iter == 1)
        {
                // Before the first iteration, output names of the
                // evolution file columns
                simulation.evolution_file
                        << "Population,"
                        << "Trial energy (Hartree),"
                        << "<E_L> (Hartree),"
                        << "Average weight,"
                        << "Average |weight|,"
			<< "Cancelled weight\n";
        }

        // Output evolution information to file
        simulation.evolution_file
                        << population_red    << ","
                        << triale_red        << ","
                        << av_loc_e_red      << ","
                        << av_weight_red     << ","
                        << av_mod_weight_red << ","
			<< cancel_red        << "\n";

        // Write the wavefunction to file
        if (simulation.write_wavefunction)
        {
                simulation.wavefunction_file << "# Iteration " << iter << "\n";
		for (int n=0; n<size(); ++n)
		{
			(*this)[n]->write_wavefunction();
			simulation.wavefunction_file << "\n";
		}
        }

        // Flush output files after every call
        simulation.flush();
}
