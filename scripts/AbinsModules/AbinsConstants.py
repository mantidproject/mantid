import AbinsParameters
import math
import numpy as np
from scipy import constants

# Parameters in this bloc shouldn't be changed by a user. They should be treated as constants.
# Changing these parameters may lead to non-functional ABINS.

# power expansion in terms of fundamentals and overtones
# S(Q, n * omega) \simeq (Q^2 * U^2)^n / n! exp(-Q^2 * U^2)
# n = 1, 2, 3.....


fundamentals = 1  # value of fundamental parameter  (n = 1)
first_overtone = 1 + fundamentals  # value of first overtone (n = 2)
first_optical_phonon = 3  # index of the first optical phonon
fundamentals_dim = 1

#  in Python first element starts at 0-th index. This is a shift to index which has to be included
# in array index calculation to write data in the proper position of array
python_index_shift = 1

# symbols of all elements
all_symbols = ["Ac", "Ag", "Al", "Am", "Ar", "As", "At", "Au", "B", "Ba", "Be", "Bh", "Bi", "Bk", "Br", "C", "Ca",
               "Cd", "Ce", "Cf", "Cl", "Cm", "Cn", "Co", "Cr", "Cs", "Cu", "Db", "Ds", "Dy", "Er", "Es", "Eu", "F",
               "Fe", "Fl", "Fm", "Fr", "Ga", "Gd", "Ge", "H", "He", "Hf", "Hg", "Ho", "Hs", "I", "In", "Ir", "K",
               "Kr", "La", "Li", "Lr", "Lu", "Lv", "Md", "Mg", "Mn", "Mo", "Mt", "N", "Na", "Nb", "Nd", "Ne", "Ni",
               "No", "Np", "O", "Os", "P", "Pa", "Pb", "Pd", "Pm", "Po", "Pr", "Pt", "Pu", "Ra", "Rb", "Re", "Rf",
               "Rg", "Rh", "Rn", "Ru", "S", "Sb", "Sc", "Se", "Sg", "Si", "Sm", "Sn", "Sr", "Ta", "Tb", "Tc", "Te",
               "Th", "Ti", "Tl", "Tm", "U", "Uuo", "Uup", "Uus", "Uut", "V", "W", "Xe", "Y", "Yb", "Zn", "Zr",
               ]

small_k = 1.0e-7  # norm of k vector below this value is considered zero

k_2_hartree = constants.codata.value("kelvin-hartree relationship")  # K * k_2_hartree =  Hartree

# here we have to multiply by 100 because frequency is expressed in cm^-1
cm1_2_hartree = constants.codata.value("inverse meter-hartree relationship") * 100.0  # cm-1 * cm1_2_hartree =  Hartree

atomic_length_2_angstrom = constants.codata.value(
    "atomic unit of length") / constants.angstrom  # 1 a.u. = 0.52917721067 Angstrom

m_2_hartree = constants.codata.value("atomic mass unit-hartree relationship")  # amu * m2_hartree =  Hartree

all_instruments = ["None", "TOSCA"]  # supported instruments

all_sample_forms = ["SingleCrystal", "Powder"]  # valid forms of samples

# keywords which define data structure of KpointsData
all_keywords_k_data = ["weights", "k_vectors", "frequencies", "atomic_displacements"]

# keywords which define data structure of AtomsData
all_keywords_atoms_data = ["symbol", "fract_coord", "atom", "sort", "mass"]

# keywords which define data structure for PowderData
all_keywords_powder_data = ["b_tensors", "a_tensors"]

# keywords which define data structure for SData
all_keywords_s_data = ["atoms_data", "frequencies"]
all_keywords_atoms_s_data = ["sort", "symbol", "s"]

float_id = np.dtype(np.float64).num
float_type = np.dtype(np.float64)

complex_id = np.dtype(np.complex).num
complex_type = np.dtype(np.complex)

int_id = np.dtype(np.uint8).num
int_type = np.dtype(np.uint8)

# maximum number of entries in the workspace
total_workspace_size = int(round(AbinsParameters.max_wavenumber / float(AbinsParameters.bin_width), 0))
higher_order_quantum_effects = 3  # number of quantum order effects taken into account
higher_order_quantum_effects_dim = higher_order_quantum_effects
max_array_size = 100000  # maximum size for storing frequencies for each quantum order


# constant to be used when iterating with range() over all considered quantum effects
# (range() is exclusive with respect to the last element)
s_last_index = 1

# constant to be used when iterating with range() over all considered quantum effects
#  (range() is exclusive with respect to the last element)
q_last_index = 1


# construction of aCLIMAX constant which is used to evaluate mean square displacement (u)
h_bar = constants.codata.value("Planck constant over 2 pi")  # h_bar =  1.0545718e-34 [J s] = [kg m^2 / s ]
h_bar_decomposition = math.frexp(h_bar)

m2_to_angstrom2 = 1.0 / constants.angstrom**2  # m^2 = 10^20 A^2
m2_to_angstrom2_decomposition = math.frexp(m2_to_angstrom2)

kg2amu = constants.codata.value("kilogram-atomic mass unit relationship")  # kg = 6.022140857e+26 amu
kg2amu_decomposition = math.frexp(kg2amu)

# here we divide by 100 because we need relation between hertz and inverse cm
hz2inv_cm = constants.codata.value("hertz-inverse meter relationship") / 100  # Hz [s^1] = 3.33564095198152e-11 [cm^-1]
hz2inv_cm_decomposition = math.frexp(hz2inv_cm)
#
# u = h_bar [J s ]/ ( 2 m [kg] omega [s^-1]) = aCLIMAX_constant / ( m [amu] nu [cm^-1])
#
# omega -- angular frequency
# nu -- wavenumber
#
# the relation between omega and nu is as follows:
#
# omega = 2 pi nu
#

aCLIMAX_constant = h_bar_decomposition[0] * m2_to_angstrom2_decomposition[0] * \
                   kg2amu_decomposition[0] * hz2inv_cm_decomposition[0] / math.pi
aCLIMAX_constant *= 2 ** (h_bar_decomposition[1] + m2_to_angstrom2_decomposition[1] + kg2amu_decomposition[1] +
                          hz2inv_cm_decomposition[1] - 2)


aCLIMAX_constant_decomposition = math.frexp(aCLIMAX_constant)
m_n_decomposition = math.frexp(constants.m_n)

# constant used to evaluate Q^2 for TOSCA.
TOSCA_constant = m_n_decomposition[0] * kg2amu_decomposition[0] / aCLIMAX_constant_decomposition[0]
TOSCA_constant *= 2 ** (m_n_decomposition[1] + kg2amu_decomposition[1] - aCLIMAX_constant_decomposition[1])
