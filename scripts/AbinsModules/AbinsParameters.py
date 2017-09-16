import math
import numpy as np
"""
Parameters for instruments and Abins
"""


# Instruments constants  #############################
# These parameters can be changed by a user if necessary

fwhm = 3.0  # approximate value for the full width at half maximum for Gaussian experimental resolutions

# ***TwoDMap instrument***
# width of narrow Gaussian in terms of incident energy which approximates Dirac delta
direct_instrument_resolution = 0.01
q_size = 60  # how many q_slices
e_init = [4100.0]  # list of incident energies with energies in cm^-1
angles = np.arange(3.0, 140.0, 5.0)  # angle detectors in degrees
interpolation = "lanczos"  # interpolation used by matplotlib.axes.Axes.imshow
colormap = "hot"  # colormap used by matplotlib.axes.Axes.imshow
figure_format = "pdf"  # format of figure to which 2D S is saved

# TOSCA instrument
#    TOSCA parameters for calculating Q^2
tosca_final_neutron_energy = 32.0  # Final energy on the crystal analyser in cm-1
tosca_cos_scattering_angle = math.cos(2.356)  # Angle of the crystal analyser radians

# TOSCA parameters for resolution function
# sigma = tosca_a * omega * omega + tosca_b * omega + tosca_c
# where sigma is width of Gaussian function
tosca_a = 0.0000001
tosca_b = 0.005
tosca_c = 2.5

# Instruments constants end ##########################


# Abins internal parameters ##########################
# Parameters which can be changed by a user if necessary

# name of the group in the hdf file in which extracted  data from ab initio vibrational calculations are stored
ab_initio_group = "VibrationalData"

powder_data_group = "Powder"  # name of the group where PowderData is stored

crystal_data_group = "SingleCrystal"  # name of the group where SingleCrystalData is stored

s_data_group = "S"  # name of the group where dynamical factor is stored

pkt_per_peak = 50  # number of points for each peak broadened by the experimental resolution
bin_width = 1.0  # defines width of bins used in rebinning of S
max_wavenumber = 4100.0  # maximum wavenumber in cm^-1 taken into account while creating workspaces (exclusive)
min_wavenumber = 0.0  # minimal wavenumber in cm^-1 taken into account while creating workspaces (exclusive)
acoustic_phonon_threshold = 0.0  # frequencies below this value are treated as acoustic and neglected.

# threshold expressed as a fraction of max S intensity below which S values are treated as zero
s_relative_threshold = 0.01

# values of S below that value are considered to be zero (to be use in case threshold calculated from
# s_relative_threshold is larger than s_absolute_threshold)
s_absolute_threshold = 10e-8

optimal_size = 5000000  # this is used to create optimal size of chunk energies for which S is calculated
# Actual chunk of energies < optimal_size

threads = 3  # number of threads used in parallel calculations
# Abins internal parameters end ###########################
