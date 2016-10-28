import math
"""
Parameters for instruments and ABINS
"""

###################################### Instruments constants  #############################
# These parameters can be changed by a user if necessary

fwhm = 3.0  # approximate value for the full width at half maximum for Gaussian experimental resolutions

# None instrument
delta_width = 0.0005  # width of narrow Gaussian which approximates Dirac delta
q_mesh = [10, 10, 10] # Q grid

# TOSCA instrument
#    TOSCA parameters for calculating Q^2
TOSCA_final_neutron_energy = 32  # Final energy on the crystal analyser cm-1
TOSCA_cos_scattering_angle = math.cos(2.356)  # Angle of the crystal analyser radians


#    TOSCA parameters for resolution function
# sigma = TOSCA_A * omega * omega + TOSCA_B * omega + TOSCA_C
# where sigma is width of Gaussian function
TOSCA_A = 0.0000001
TOSCA_B = 0.005
TOSCA_C = 2.5

###################################### Instruments constants end ##########################


###################################### ABINS internal parameters ##########################
# Parameters which can be changed by user if necessary

DFT_group = "PhononAB"  # name of the group in the hdf file in which extracted data from DFT phonon calculations are stored

Q_data_group = "Q_data"  # name of the group where Q data is stored

powder_data_group = "Powder"  # name of the group where PowderData is stored

crystal_data_group = "Crystal"  # name of the group where CrystalData is stored

S_data_group = "S"  # name of the group where dynamical factor is stored

pkt_per_peak = 50  # number of points for each peak broadened by the experimental resolution
bin_width = 1.0  # defines width of bins used in rebining of S
max_wavenumber = 4100  # maximum wavenumber in cm^1 taken into account while creating workspaces (exclusive)
min_wavenumber = 0.0 # minimal wavenumber in cm^-1 taken into account while creating workspaces (exclusive)
acoustic_phonon_threshold = 0.0 # frequencies below this value are treated as acoustic and neglected.
######################################ABINS internal parameters end ###########################


