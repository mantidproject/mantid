"""
Constants for instruments and ABINS
"""

# ABINS internal constants

DFT_group = "PhononAB" # name of the group in the hdf file in which extracted data from DFT phonon calculations are stored

Q_data_group = "Q_data" # name of the group where Q data is stored

DW_data_group = "DW_factors" # name of the group where Debye-Waller factors are stored

MSQ_data_group = "MSQ" # name of the group where mean square displacements are stored

S_data_group = "S" # name of the group where dynamical factor is stored


all_instruments = ["None", "TOSCA"] # supported instruments

all_sample_forms = ["SingleCrystal", "Powder"] # valid forms of samples

# keywords which define data structure of KpointsData
all_keywords_k_data = ["weight", "value", "frequencies", "atomic_displacements"]

# keywords which define data structure of AtomsData
all_keywords_atoms_data = ["symbol", "fract_coord", "atom", "sort", "mass"]

# symbols of all elements
all_symbols = ["Ac", "Ag", "Al", "Am", "Ar",  "As", "At" , "Au" , "B"  , "Ba", "Be", "Bh", "Bi", "Bk", "Br", "C" , "Ca" ,
               "Cd", "Ce", "Cf", "Cl", "Cm",  "Cn", "Co" , "Cr" , "Cs" , "Cu", "Db", "Ds", "Dy", "Er", "Es", "Eu", "F" ,
               "Fe", "Fl", "Fm", "Fr", "Ga",  "Gd", "Ge" , "H"  , "He" , "Hf", "Hg", "Ho", "Hs", "I" , "In", "Ir", "K" ,
               "Kr", "La", "Li", "Lr", "Lu",  "Lv", "Md" , "Mg" , "Mn" , "Mo", "Mt", "N" , "Na", "Nb", "Nd", "Ne", "Ni",
               "No", "Np", "O" , "Os", "P" ,  "Pa", "Pb" , "Pd" , "Pm" , "Po", "Pr", "Pt", "Pu", "Ra", "Rb", "Re", "Rf",
               "Rg", "Rh", "Rn", "Ru", "S" ,  "Sb", "Sc" , "Se" , "Sg" , "Si", "Sm", "Sn", "Sr", "Ta", "Tb", "Tc", "Te",
               "Th", "Ti", "Tl", "Tm", "U" , "Uuo", "Uup", "Uus", "Uut", "V" , "W" , "Xe", "Y" , "Yb", "Zn", "Zr",
               ]

# Instruments constants
TOSCA_constant = 1 / 16.0 # magic number for TOSCA...

