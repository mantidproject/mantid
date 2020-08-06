# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import math
import numpy as np
"""
Parameters for instruments and Abins

These values are somewhat 'global' across Abins. In previous versions it was
recommended that they could be changed by editing the AbinsParameters.py file.
This is still possible, but in general it is recommended to edit the
value dictionaries through the Python API if possible. e.g.::

    import abins
    abins.parameters.instruments['TOSCA']['final_neutron_energy'] = 24.0
"""

# Instruments constants  #############################
instruments = {
    'fwhm': 3.0,  # approximate value for the full width at half maximum for Gaussian experimental resolutions
    'TwoDMap': {
        'resolution': 0.1,  # Width of broadening function
        'q_size': 200,  # Number of q slices
        'e_init': [4100.0],  # Incident energies in cm-1
        'angles': np.arange(3.0, 140.0, 1).tolist(), # All measurement angles for direct sweeps
        },
    'TOSCA': {
        #    TOSCA parameters for calculating Q^2
        'final_neutron_energy': 32.0,  # Final energy on the crystal analyser in cm-1
        'cos_scattering_angle': math.cos(2.356),  # Angle of the crystal analyser radians
        # TOSCA parameters for resolution function
        # sigma = tosca_a * omega * omega + tosca_b * omega + tosca_c
        # where sigma is width of Gaussian function
        'a': 0.0000001,
        'b': 0.005,
        'c': 2.5,}
    }

# Names of groups in HDF5 cache/output file ################
hdf_groups = {
    'ab_initio_data': 'ABData',  # AbinsData (i.e. input from phonon calculation)
    'powder_data': 'Powder',  # PowderData
    'crystal_data': 'SingleCrystal',  # SingleCrystalData
    's_data': 'S'  # dynamical structure factor
    }

# Parameters related to sampling density and numerical cutoffs ##########################
sampling = {
    'max_wavenumber': 4100.0,  # maximum wavenumber in cm^-1 taken into account while creating workspaces (exclusive)
    'min_wavenumber': 0.0,  # minimal wavenumber in cm^-1 taken into account while creating workspaces (exclusive)
    'bin_width': 1.0,  # Step size for bins in wavenumber. This is modified at runtime by the Abins(2D) algorithm.
    'frequencies_threshold': 0.0,  # minimum included frequency
    's_relative_threshold': 0.01,  # low cutoff for S intensity (fraction of maximum S)
    's_absolute_threshold': 1e-7,  # low cutoff for S intensity (absolute value)
    'broadening_scheme': 'auto',
    }

# Parameters related to performance optimisation that do NOT impact calculation results
performance = {
    'optimal_size': 5000000,  # this is used to create optimal size of chunk energies for which S is calculated
    'threads': 4  # number of threads used in parallel calculations
    }

all_parameters = {'instruments': instruments,
                  'hdf_groups': hdf_groups,
                  'sampling': sampling,
                  'performance': performance}

non_performance_parameters = {'instruments': instruments,
                              'hdf_groups': hdf_groups,
                              'sampling': sampling}
