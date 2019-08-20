# Will generate the params.h and params.cpp files from 
# params_template.h and params_template.cpp respectively
params = {}

params["pid"] = [
    "int",
    "pid",
    "0",
    "The MPI process id of this process. Will be in [0,np)."
]

params["np"] = [
    "int",
    "np",
    "1",
    "The number of MPI processes."
]

params["dimensions"] = [
    "int",
    "dimensions",
    "3",
    "The spatial dimensions of the system."
]

params["walkers"] = [
    "int",
    "target_population",
    "1000",
    ("The target population of DMC walkers. The actual number of "
     "dmc walkers will fluctuate during runtime, but will be bias "
     "towards this value.")
]

params["max_pop_ratio"] = [
    "double",
    "max_pop_ratio",
    "4.0",
    ("The maximum allowed population, expressed as a multiple of "
     "the target population.")
]

params["min_pop_ratio"] = [
    "double",
    "min_pop_ratio",
    "0.5",
    ("The minimum allowed population, expressed as a fraction of "
    "the target population.")
]

params["iterations"] = [
    "int",
    "dmc_iterations",
    "10000",
    ("The number of DMC iterations, each corresponding to a "
     "step of tau in imaginary time.")
]

params["tau"] = [
    "double",
    "tau",
    "0.01",
    "The DMC timestep in atomic units."
]

params["tau_c_ratio"] = [
    "double",
    "tau_c_ratio",
    "1.0",
    ("The ratio of tau_c:tau, where tau_c is the effective "
     "cancellation timestep and tau is the DMC timestep.")
]

params["trial_energy"] = [
    "double",
    "trial_energy",
    "0.0",
    ("The DMC trial energy in atomic units (Hartree). This "
     "value is used to control the DMC population and will "
     "fluctuate during runtime. After equilibriation, it will "
     "fluctuate around the ground state energy of the system.")
]

params["pre_diffusion"] = [
    "double",
    "pre_diffusion",
    "1.0",
    ("The amount of imaginary time that the walkers will diffuse for "
     "before the first full DMC iteration. Effectively, this is how "
     "spread out the initial wavefunction is.")
]  

params["write_wavefunction"] = [
   "bool",
   "write_wavefunction",
   "true",
   "True if wavefunction files are to be written."
]

params["exchange_moves"] = [
    "bool",
    "exchange_moves",
    "true",
    "True if exchange moves are to be made."
]

params["exchange_prob"] = [
    "double",
    "exchange_prob",
    "0.5",
    ("The probability of a walker making an exchange move in any "
     "given timestep. The actual exchange move made will be chosen at random. "
     "1 - this is the probability of simply diffusing, making no exchange moves.")
]

params["cancel_scheme"] = [
    "std::string",
    "cancel_scheme",
    '"voronoi"',
    "The cancellation scheme used."
]

params["correct_seperations"] = [
    "bool",
    "correct_seperations",
    "false",
    "True if seperation corrections are applied."
]

# Generate the params.h file from params_template.h
f = open("params_template.h")
lines = f.read().split("\n")
f.close()

f = open("../params.h","w")
for l in lines:
    if "PYTHON_GEN_PARAMS_HERE" in l:
        ws = l[0:l.find("P")]
        l  = ws+"// Generated by gen_params.py\n"
        for p in params:
            par = params[p]
            l  += ws+"extern {0} {1};\n".format(par[0], par[1])
        l += ws+"// End generated by gen_params.py"
    f.write(l+"\n")
f.close()

# Generate the params.cpp file from params_template.cpp
f = open("params_template.cpp")
lines = f.read().split("\n")
f.close()

f = open("../params.cpp","w")
for l in lines:

    if "PYTHON_GEN_PARAMS_HERE" in l:
        ws = l[0:l.find("P")]
        l  = ws+"// Generated by gen_params.py\n"
        for p in params:
            par = params[p]
            l   += ws+"{0} params::{1} = {2};\n".format(par[0],par[1],par[2])
        l += ws+"// End generated by gen_params.py"

    if "PYTHON_PARSE_PARAMS_HERE" in l:
        ws = l[0:l.find("P")]
        l  = ws+"// Generated by gen_params.py\n"
        for p in params:
            par = params[p]
            l += ws+"// Parse {0}\n".format(p)
            l += ws+'if (tag == "{0}")\n'.format(p)
            l += ws+"{\n"
            l += ws+"    std::stringstream ss(split[1]);\n"
            l += ws+"    ss >> params::{0};\n".format(par[1])
            l += ws+"}\n\n"
        l += ws+"// End generated by gen_params.py"

    if "PYTHON_OUTPUT_PARAMS_HERE" in l:
        ws = l[0:l.find("P")]
        l  = ws+"// Generated by gen_params.py\n"
        for p in params:
            par = params[p]
            l += ws+'progress_file << "    " << "{0}"'.format(p)
            l += " << params::{0}".format(par[1])
            l += r' << "\n";'+"\n"
        l += ws+"// End generated by gen_params.py"
        
    f.write(l+"\n")
f.close()

