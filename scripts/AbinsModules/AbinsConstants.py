import AbinsParameters
import numpy as np
from scipy import constants

####################################### Constants ##########################################
# Parameters in this bloc shouldn't be changed by a user. They should be treated as constants.
# Changing these parameters may lead to non-functional ABINS.

# power expansion in terms of fundamentals and overtones
# S(Q, n * omega) \simeq (Q^2 * U^2)^n / n! exp(-Q^2 * U^2)
# n = 1, 2, 3.....

first_overtone = 2 # value of first overtone (n = 2)
fundamentals = 1 # value of fundamental parameter  (n = 1)
fundamentals_dim = 1

python_index_shift = 1  # in Python first element starts at 0-th index. This is a shift to index which has to be included
# in array index calculation to write data in the proper position of array

# symbols of all elements
all_symbols = ["Ac", "Ag", "Al", "Am", "Ar",  "As", "At" , "Au" , "B"  , "Ba", "Be", "Bh", "Bi", "Bk", "Br", "C" , "Ca" ,
               "Cd", "Ce", "Cf", "Cl", "Cm",  "Cn", "Co" , "Cr" , "Cs" , "Cu", "Db", "Ds", "Dy", "Er", "Es", "Eu", "F" ,
               "Fe", "Fl", "Fm", "Fr", "Ga",  "Gd", "Ge" , "H"  , "He" , "Hf", "Hg", "Ho", "Hs", "I" , "In", "Ir", "K" ,
               "Kr", "La", "Li", "Lr", "Lu",  "Lv", "Md" , "Mg" , "Mn" , "Mo", "Mt", "N" , "Na", "Nb", "Nd", "Ne", "Ni",
               "No", "Np", "O" , "Os", "P" ,  "Pa", "Pb" , "Pd" , "Pm" , "Po", "Pr", "Pt", "Pu", "Ra", "Rb", "Re", "Rf",
               "Rg", "Rh", "Rn", "Ru", "S" ,  "Sb", "Sc" , "Se" , "Sg" , "Si", "Sm", "Sn", "Sr", "Ta", "Tb", "Tc", "Te",
               "Th", "Ti", "Tl", "Tm", "U" , "Uuo", "Uup", "Uus", "Uut", "V" , "W" , "Xe", "Y" , "Yb", "Zn", "Zr",
               ]

small_k = 1.0e-7  # norm of k vector below this value is considered zero

k_2_hartree = constants.codata.value("kelvin-hartree relationship")  # K * k_2_hartree =  Hartree

# here we have to multiply by 100 because frequency is expressed in cm^-1
cm1_2_hartree = constants.codata.value("inverse meter-hartree relationship") * 100.0   # cm-1 * cm1_2_hartree =  Hartree

atomic_length_2_angstrom = constants.codata.value("atomic unit of length") / constants.angstrom # 1 a.u. = 0.52917721067 Angstrom

m_2_hartree = constants.codata.value("atomic mass unit-hartree relationship")  # amu * m2_hartree =  Hartree

all_instruments = ["None", "TOSCA"]  # supported instruments

all_sample_forms = ["SingleCrystal", "Powder"]  # valid forms of samples

# keywords which define data structure of KpointsData
all_keywords_k_data = ["weights", "k_vectors", "frequencies", "atomic_displacements"]

# keywords which define data structure of AtomsData
all_keywords_atoms_data = ["symbol", "fract_coord", "atom", "sort", "mass"]

# keywords which define data structure for PowderData
all_keywords_powder_data = ["msd", "dw"]

# keywords which define data structure for SData
all_keywords_s_data = ["atoms_data", "frequencies"]
all_keywords_atoms_s_data = ["sort", "symbol", "s"]

float_id = np.dtype(np.float64).num
float_type = np.dtype(np.float64)

complex_id = np.dtype(np.complex).num
complex_type = np.dtype(np.complex)

total_workspace_size = int(round(AbinsParameters.max_wavenumber / float(AbinsParameters.bin_width), 0)) # maximum number of entries in the workspace
higher_order_quantum_effects_dim = AbinsParameters.higher_order_quantum_effects
max_quantum_order_array_size = 100000  # maximum size for storing frequencies for each quantum order
s_last_index = 1 # constant to be used when iterating with range() over all considered quantum effects (range() is exclusive with respect to the last element)
q_last_index = 1 # constant to be used when iterating with range() over all considered quantum effects  (range() is exclusive with respect to the last element)
####################################### Constants end #################################