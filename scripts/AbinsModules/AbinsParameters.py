import math
"""
Parameters for instruments and ABINS
"""

# Instruments constants  #############################
# These parameters can be changed by a user if necessary

fwhm = 3.0  # approximate value for the full width at half maximum for Gaussian experimental resolutions

# None instrument
delta_width = 0.0005  # width of narrow Gaussian which approximates Dirac delta
q_mesh = [10, 10, 10]  # Q grid

# TOSCA instrument
#    TOSCA parameters for calculating Q^2
tosca_final_neutron_energy = 32.0  # Final energy on the crystal analyser cm-1
tosca_cos_scattering_angle = math.cos(2.356)  # Angle of the crystal analyser radians


#    TOSCA parameters for resolution function
# sigma = tosca_a * omega * omega + tosca_b * omega + tosca_c
# where sigma is width of Gaussian function
tosca_a = 0.0000001
tosca_b = 0.005
tosca_c = 2.5

# Instruments constants end ##########################


# ABINS internal parameters ##########################
# Parameters which can be changed by user if necessary

# name of the group in the hdf file in which extracted  data from DFT phonon calculations are stored
dft_group = "PhononAB"

q_data_group = "Q_data"  # name of the group where Q data is stored

powder_data_group = "Powder"  # name of the group where PowderData is stored

crystal_data_group = "Crystal"  # name of the group where CrystalData is stored

s_data_group = "S"  # name of the group where dynamical factor is stored

pkt_per_peak = 100  # number of points for each peak broadened by the experimental resolution
bin_width = 1.0  # defines width of bins used in rebining of S
max_wavenumber = 4100  # maximum wavenumber in cm^1 taken into account while creating workspaces (exclusive)
min_wavenumber = 0.0  # minimal wavenumber in cm^-1 taken into account while creating workspaces (exclusive)
acoustic_phonon_threshold = 0.0  # frequencies below this value are treated as acoustic and neglected.
# ABINS internal parameters end ###########################
