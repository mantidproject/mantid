import numpy as np
from scipy import constants


"""
Constants for instruments and ABINS
"""

# ABINS internal constants

float_id = np.dtype(np.float64).num
float_type = np.dtype(np.float64)

complex_id = np.dtype(np.complex).num
complex_type = np.dtype(np.complex)

DFT_group = "PhononAB"  # name of the group in the hdf file in which extracted data from DFT phonon calculations are stored

Q_data_group = "Q_data"  # name of the group where Q data is stored

DW_data_group = "DW_factors"  # name of the group where Debye-Waller factors are stored

powder_data_group = "Powder"  # name of the group where PowderData is stored

crystal_data_group = "Crystal"  # name of the group where CrystalData is stored

S_data_group = "S"  # name of the group where dynamical factor is stored

overtones_num = 10  # number of overtones taken into account in powder case scenario

all_instruments = ["None", "TOSCA"]  # supported instruments

all_sample_forms = ["SingleCrystal", "Powder"]  # valid forms of samples

# keywords which define data structure of KpointsData
all_keywords_k_data = ["weights", "k_vectors", "frequencies", "atomic_displacements"]

# keywords which define data structure of AtomsData
all_keywords_atoms_data = ["symbol", "fract_coord", "atom", "sort", "mass"]

# keywords which define data structure for PowderData
all_keywords_powder_data = ["msd", "dw"]

# keywords which define data structure for SData
all_keywords_s_data = ["atoms_data", "convoluted_frequencies"]
all_keywords_s_sub_data = ["sort", "symbol", "value"]

# symbols of all elements
all_symbols = ["Ac", "Ag", "Al", "Am", "Ar",  "As", "At" , "Au" , "B"  , "Ba", "Be", "Bh", "Bi", "Bk", "Br", "C" , "Ca" ,
               "Cd", "Ce", "Cf", "Cl", "Cm",  "Cn", "Co" , "Cr" , "Cs" , "Cu", "Db", "Ds", "Dy", "Er", "Es", "Eu", "F" ,
               "Fe", "Fl", "Fm", "Fr", "Ga",  "Gd", "Ge" , "H"  , "He" , "Hf", "Hg", "Ho", "Hs", "I" , "In", "Ir", "K" ,
               "Kr", "La", "Li", "Lr", "Lu",  "Lv", "Md" , "Mg" , "Mn" , "Mo", "Mt", "N" , "Na", "Nb", "Nd", "Ne", "Ni",
               "No", "Np", "O" , "Os", "P" ,  "Pa", "Pb" , "Pd" , "Pm" , "Po", "Pr", "Pt", "Pu", "Ra", "Rb", "Re", "Rf",
               "Rg", "Rh", "Rn", "Ru", "S" ,  "Sb", "Sc" , "Se" , "Sg" , "Si", "Sm", "Sn", "Sr", "Ta", "Tb", "Tc", "Te",
               "Th", "Ti", "Tl", "Tm", "U" , "Uuo", "Uup", "Uus", "Uut", "V" , "W" , "Xe", "Y" , "Yb", "Zn", "Zr",
               ]
pkt_per_peak = 500  # number of points for each broadened peak
bin_width = 0.2  # defines width of bins used in rebining

small_k = 1.0e-6  # norm of k vector below this value is considered zero

k_2_hartree = constants.codata.value("kelvin-hartree relationship")  # K * k_2_hartree =  Hartree

# here we have to multiply by 100 because frequency is expressed in cm^-1
cm1_2_hartree = constants.codata.value("inverse meter-hartree relationship") * 100.0   # cm-1 * cm1_2_hartree =  Hartree

m_2_hartree = constants.codata.value("atomic mass unit-hartree relationship")  # amu * m2_hartree =  Hartree


# Instruments constants
fwhm = 3.0  # approximate value for the full width at half maximum for Gaussian experimental resolutions

# None instrument
delta_width = 0.0005  # width of narrow Gaussian which approximates Dirac delta

# TOSCA instrument
TOSCA_constant = 1 / 16.0  # magic number for TOSCA...
# sigma = TOSCA_A * omega * omega + TOSCA_B * omega + TOSCA_C
TOSCA_A = 0.0000001
TOSCA_B = 0.005
TOSCA_C = 2.5

