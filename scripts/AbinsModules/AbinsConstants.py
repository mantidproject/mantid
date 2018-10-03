# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)
import math
import numpy as np
from scipy import constants

# Parameters in this bloc shouldn't be changed by a user. They should be treated as constants.
# Changing these parameters may lead to non-functional Abins.

# power expansion in terms of FUNDAMENTALS and overtones
# S(Q, n * omega) \simeq (Q^2 * U^2)^n / n! exp(-Q^2 * U^2)
# n = 1, 2, 3.....


FUNDAMENTALS = 1  # value of fundamental parameter  (n = 1)
FIRST_OVERTONE = 1 + FUNDAMENTALS  # value of first overtone (n = 2)
FIRST_OPTICAL_PHONON = 3  # index of the first optical phonon
FIRST_MOLECULAR_VIBRATION = 0  # index of the first vibration for molecule
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

SMALL_K = 1.0e-1  # norm of k vector below this value is considered zero

K_2_HARTREE = constants.codata.value("kelvin-hartree relationship")  # K * K_2_HARTREE =  Hartree

# here we have to multiply by 100 because frequency is expressed in cm^-1
CM1_2_HARTREE = constants.codata.value("inverse meter-hartree relationship") * 100.0  # cm-1 * CM1_2_HARTREE =  Hartree

ATOMIC_LENGTH_2_ANGSTROM = constants.codata.value(
    "atomic unit of length") / constants.angstrom  # 1 a.u. = 0.52917721067 Angstrom

M_2_HARTREE = constants.codata.value("atomic mass unit-hartree relationship")  # amu * m2_hartree =  Hartree

ALL_INSTRUMENTS = ["TOSCA"]  # supported instruments

# ALL_SAMPLE_FORMS = ["SingleCrystal", "Powder"]  # valid forms of samples
ALL_SAMPLE_FORMS = ["Powder"]  # valid forms of samples

# keywords which define data structure of KpointsData
ALL_KEYWORDS_K_DATA = ["weights", "k_vectors", "frequencies", "atomic_displacements", "unit_cell"]

# keywords which define data structure of AtomsData
ALL_KEYWORDS_ATOMS_DATA = ["symbol", "coord", "sort", "mass"]
# keywords which define data structure for PowderData
ALL_KEYWORDS_POWDER_DATA = ["b_tensors", "a_tensors"]

# keywords which define data structure for SData
ALL_KEYWORDS_S_DATA = ["data"]
ALL_KEYWORDS_ATOMS_S_DATA = ["s"]
S_LABEL = "s"
ATOM_LABEL = "atom"

FLOAT_ID = np.dtype(np.float64).num
FLOAT_TYPE = np.dtype(np.float64)

COMPLEX_ID = np.dtype(np.complex).num
COMPLEX_TYPE = np.dtype(np.complex)

INT_ID = np.dtype(np.uint32).num
INT_TYPE = np.dtype(np.uint32)

HIGHER_ORDER_QUANTUM_EVENTS = 3  # number of quantum order effects taken into account
HIGHER_ORDER_QUANTUM_EVENTS_DIM = HIGHER_ORDER_QUANTUM_EVENTS

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
# u = H_BAR [J s ]/ ( 2 m [kg] omega [s^-1]) = CONSTANT / ( m [amu] nu [cm^-1])
#
# omega -- angular frequency
# nu -- wavenumber
#
# the relation between omega and nu is as follows:
#
# omega = 2 pi nu
#

CONSTANT = H_BAR_DECOMPOSITION[0] * M2_TO_ANGSTROM2_DECOMPOSITION[0] * \
    KG2AMU_DECOMPOSITION[0] * HZ2INV_CM_DECOMPOSITION[0] / math.pi
CONSTANT *= 2 ** (H_BAR_DECOMPOSITION[1] + M2_TO_ANGSTROM2_DECOMPOSITION[1] + KG2AMU_DECOMPOSITION[1] +
                  HZ2INV_CM_DECOMPOSITION[1] - 2)

CONSTANT_DECOMPOSITION = math.frexp(CONSTANT)
M_N_DECOMPOSITION = math.frexp(constants.m_n)

# constant used to evaluate Q^2 in 1/A from energy in cm^-1.
WAVENUMBER_TO_INVERSE_A = M_N_DECOMPOSITION[0] * KG2AMU_DECOMPOSITION[0] / CONSTANT_DECOMPOSITION[0]
WAVENUMBER_TO_INVERSE_A *= 2 ** (M_N_DECOMPOSITION[1] + KG2AMU_DECOMPOSITION[1] - CONSTANT_DECOMPOSITION[1])

# constants which represent quantum order effects
QUANTUM_ORDER_ONE = 1
QUANTUM_ORDER_TWO = 2
QUANTUM_ORDER_THREE = 3
QUANTUM_ORDER_FOUR = 4

MIN_SIZE = 2  # minimal size of an array

# values of S below that are considered to be zero
THRESHOLD = 10e-15
NUM_ZERO = 10e-15

MAX_ORDER = 4  # max quantum order event

ALL_SUPPORTED_AB_INITIO_PROGRAMS = ["CRYSTAL", "CASTEP", "DMOL3", "GAUSSIAN"]

ONE_DIMENSIONAL_INSTRUMENTS = ["TOSCA"]
ONE_DIMENSIONAL_SPECTRUM = 1

FIRST_BIN_INDEX = 1

GAMMA_POINT = "0"
BUF = 65536

CRYSTAL = False

# definition of momentum transfer range
ACOUSTIC_PHONON_THRESHOLD = 10.0  # acoustic threshold in cm^-1

# indentations in messages
INCIDENT_ENERGY_MESSAGE_INDENTATION = "  "
K_POINT_MESSAGE_INDENTATION = 2 * INCIDENT_ENERGY_MESSAGE_INDENTATION
ANGLE_MESSAGE_INDENTATION = 3 * INCIDENT_ENERGY_MESSAGE_INDENTATION
S_FOR_ATOM_MESSAGE = 4 * INCIDENT_ENERGY_MESSAGE_INDENTATION
S_THRESHOLD_CHANGE_INDENTATION = 5 * INCIDENT_ENERGY_MESSAGE_INDENTATION

MAX_BIN_WIDTH = 10.0  # max bin width in cm^-1
MIN_BIN_WIDTH = 0.01  # min bin width in cm^-1

MAX_WIDTH = 10.0  # max width of the resolution function in cm^-1
MIN_WIDTH = 0.1  # min width of the resolution function in cm^-1

MAX_DIRECT_RESOLUTION = 0.1  # max Gaussian width for direct instruments defined as max percentage of incident energy
MIN_DIRECT_RESOLUTION = 0.0001

MIN_WAVENUMBER = 0.0  # in cm^-1
MAX_WAVENUMBER = 5000.0  # in cm^-1

MAX_POINTS_PER_PEAK = 1000
MIN_POINTS_PER_PEAK = 1

SMALL_S = 1e-6
MAX_THRESHOLD = 0.3

ONE_CHARACTER = 1
EOF = b""

ROTATIONS_AND_TRANSLATIONS = 6

# This constant is used to check whether for the given atom mass averaged over all isotopes or mass of the
# specific isotope is used.
MASS_EPS = 1e-2  # in amu units.

# this constant is used to check if in a system for the given symbol of an element all atoms with this symbol have
# the same mass
ONLY_ONE_MASS = 1
