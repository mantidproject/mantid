import AbinsParameters
import math
import numpy as np
from scipy import constants
import re
from platform import python_version
from mantid import config

# Parameters in this bloc shouldn't be changed by a user. They should be treated as constants.
# Changing these parameters may lead to non-functional ABINS.

# power expansion in terms of FUNDAMENTALS and overtones
# S(Q, n * omega) \simeq (Q^2 * U^2)^n / n! exp(-Q^2 * U^2)
# n = 1, 2, 3.....


FUNDAMENTALS = 1  # value of fundamental parameter  (n = 1)
FIRST_OVERTONE = 1 + FUNDAMENTALS  # value of first overtone (n = 2)
FIRST_OPTICAL_PHONON = 3  # index of the first optical phonon
FUNDAMENTALS_DIM = 1

# In Python first element starts at 0-th index. This is a shift to index which has to be included
# in array index calculation to write data in the proper position of array
PYTHON_INDEX_SHIFT = 1

# symbols of all elements
ALL_SYMBOLS = ["Ac", "Ag", "Al", "Am", "Ar", "As", "At", "Au", "B", "Ba", "Be", "Bh", "Bi", "Bk", "Br", "C", "Ca",
               "Cd", "Ce", "Cf", "Cl", "Cm", "Cn", "Co", "Cr", "Cs", "Cu", "Db", "Ds", "Dy", "Er", "Es", "Eu", "F",
               "Fe", "Fl", "Fm", "Fr", "Ga", "Gd", "Ge", "H", "He", "Hf", "Hg", "Ho", "Hs", "I", "In", "Ir", "K",
               "Kr", "La", "Li", "Lr", "Lu", "Lv", "Md", "Mg", "Mn", "Mo", "Mt", "N", "Na", "Nb", "Nd", "Ne", "Ni",
               "No", "Np", "O", "Os", "P", "Pa", "Pb", "Pd", "Pm", "Po", "Pr", "Pt", "Pu", "Ra", "Rb", "Re", "Rf",
               "Rg", "Rh", "Rn", "Ru", "S", "Sb", "Sc", "Se", "Sg", "Si", "Sm", "Sn", "Sr", "Ta", "Tb", "Tc", "Te",
               "Th", "Ti", "Tl", "Tm", "U", "Uuo", "Uup", "Uus", "Uut", "V", "W", "Xe", "Y", "Yb", "Zn", "Zr",
               ]

SMALL_K = 1.0e-7  # norm of k vector below this value is considered zero

K_2_HARTREE = constants.codata.value("kelvin-hartree relationship")  # K * K_2_HARTREE =  Hartree

# here we have to multiply by 100 because frequency is expressed in cm^-1
CM1_2_HARTREE = constants.codata.value("inverse meter-hartree relationship") * 100.0  # cm-1 * CM1_2_HARTREE =  Hartree

ATOMIC_LENGTH_2_ANGSTROM = constants.codata.value(
    "atomic unit of length") / constants.angstrom  # 1 a.u. = 0.52917721067 Angstrom

M_2_HARTREE = constants.codata.value("atomic mass unit-hartree relationship")  # amu * m2_hartree =  Hartree

ALL_INSTRUMENTS = ["TwoDMap", "TOSCA"]  # supported instruments

ALL_SAMPLE_FORMS = ["SingleCrystal", "Powder"]  # valid forms of samples

# keywords which define data structure of KpointsData
ALL_KEYWORDS_K_DATA = ["weights", "k_vectors", "frequencies", "atomic_displacements"]

# keywords which define data structure of AtomsData
ALL_KEYWORDS_ATOMS_DATA = ["symbol", "fract_coord", "atom", "sort", "mass"]

# keywords which define data structure for PowderData
ALL_KEYWORDS_POWDER_DATA = ["b_tensors", "a_tensors"]

# keywords which define data structure for SData
ALL_KEYWORDS_S_DATA = ["data"]
ALL_KEYWORDS_ATOMS_S_DATA = ["sort", "symbol", "s", "frequencies"]

FLOAT_ID = np.dtype(np.float64).num
FLOAT_TYPE = np.dtype(np.float64)

COMPLEX_ID = np.dtype(np.complex).num
COMPLEX_TYPE = np.dtype(np.complex)

INT_ID = np.dtype(np.uint16).num
INT_TYPE = np.dtype(np.uint16)

# maximum number of entries in the workspace
TOTAL_WORKSPACE_SIZE = int(round(AbinsParameters.max_wavenumber / float(AbinsParameters.bin_width), 0))
HIGHER_ORDER_QUANTUM_EVENTS = 3  # number of quantum order effects taken into account
HIGHER_ORDER_QUANTUM_EVENTS_DIM = HIGHER_ORDER_QUANTUM_EVENTS
MAX_ARRAY_SIZE = 1000000  # maximum size for storing frequencies for each quantum order


# constant to be used when iterating with range() over all considered quantum effects
# (range() is exclusive with respect to the last element)
S_LAST_INDEX = 1

# construction of aCLIMAX constant which is used to evaluate mean square displacement (u)
H_BAR = constants.codata.value("Planck constant over 2 pi")  # H_BAR =  1.0545718e-34 [J s] = [kg m^2 / s ]
H_BAR_DECOMPOSITION = math.frexp(H_BAR)

M2_TO_ANGSTROM2 = 1.0 / constants.angstrom ** 2  # m^2 = 10^20 A^2
M2_TO_ANGSTROM2_DECOMPOSITION = math.frexp(M2_TO_ANGSTROM2)

KG2AMU = constants.codata.value("kilogram-atomic mass unit relationship")  # kg = 6.022140857e+26 amu
KG2AMU_DECOMPOSITION = math.frexp(KG2AMU)

# here we divide by 100 because we need relation between hertz and inverse cm
HZ2INV_CM = constants.codata.value("hertz-inverse meter relationship") / 100  # Hz [s^1] = 3.33564095198152e-11 [cm^-1]
HZ2INV_CM_DECOMPOSITION = math.frexp(HZ2INV_CM)
#
# u = H_BAR [J s ]/ ( 2 m [kg] omega [s^-1]) = ACLIMAX_CONSTANT / ( m [amu] nu [cm^-1])
#
# omega -- angular frequency
# nu -- wavenumber
#
# the relation between omega and nu is as follows:
#
# omega = 2 pi nu
#

ACLIMAX_CONSTANT = H_BAR_DECOMPOSITION[0] * M2_TO_ANGSTROM2_DECOMPOSITION[0] * \
                   KG2AMU_DECOMPOSITION[0] * HZ2INV_CM_DECOMPOSITION[0] / math.pi
ACLIMAX_CONSTANT *= 2 ** (H_BAR_DECOMPOSITION[1] + M2_TO_ANGSTROM2_DECOMPOSITION[1] + KG2AMU_DECOMPOSITION[1] +
                          HZ2INV_CM_DECOMPOSITION[1] - 2)

ACLIMAX_CONSTANT_DECOMPOSITION = math.frexp(ACLIMAX_CONSTANT)
M_N_DECOMPOSITION = math.frexp(constants.m_n)

# constant used to evaluate Q^2 for TOSCA.
TOSCA_CONSTANT = M_N_DECOMPOSITION[0] * KG2AMU_DECOMPOSITION[0] / ACLIMAX_CONSTANT_DECOMPOSITION[0]
TOSCA_CONSTANT *= 2 ** (M_N_DECOMPOSITION[1] + KG2AMU_DECOMPOSITION[1] - ACLIMAX_CONSTANT_DECOMPOSITION[1])

# constants which represent quantum order effects
QUANTUM_ORDER_ONE = 1
QUANTUM_ORDER_TWO = 2
QUANTUM_ORDER_THREE = 3
QUANTUM_ORDER_FOUR = 4


# values of S below that are considered to be zero
S_THRESHOLD = 10e-8

NUMPY_VERSION_REQUIRED = "1.6.0"  # ABINS requires numpy 1.6.0 or higher


def tuple_version_representation(string=None):
    """
    Calculates as a tuple numerical representation of package version .

    :param string:  version of package in the format number1.number2.number3....numberN
    :return: numerical representation of package version in the form  of tuple (number1, number2 ...., numberN)
    """
    if not isinstance(string, str):
        raise ValueError("Version of package in the form of string is expected.")
    if "." not in string:
        raise ValueError("Invalid format of package version.")

    try:
        return tuple([int(i) for i in re.findall(r'\d+', string=string)])
    except:
        raise ValueError("Version of package couldn't be converted to number. (version=", string)


def is_numpy_valid(string=None):
    """

    :param string: version of numpy to be checked in the format number1.number2.number3
    :return: False if version of numpy  is valid otherwise True
    """

    return tuple_version_representation(string=string) < tuple_version_representation(NUMPY_VERSION_REQUIRED)


def old_python():
    """
    Checks if Python i not too old
    :return: True if it is too old otherwise False
    """
    return tuple([int(i) for i in re.findall(r'\d+', string=python_version().replace(".", " "))]) < (2, 7)


def get_core_folder():
    """
    Calculates folder in which testing data is stored. Folder is determined in the platform independent way. It is
    assumed that the path to the testing data includes keyword  "UnitTest".
    :return: path to the folder with testing data
    """
    folders = config.getDataSearchDirs()
    for folder in folders:
        if "UnitTest" in folder:
            return folder
    raise ValueError("Folder with testing data not found.")
