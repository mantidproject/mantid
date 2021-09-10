# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""
Parameters for instruments and Abins

These values are somewhat 'global' across Abins. In previous versions it was
recommended that they could be changed by editing the AbinsParameters.py file.
This is still possible, but in general it is recommended to edit the
value dictionaries through the Python API if possible. e.g.::

    import abins
    abins.parameters.instruments['TOSCA']['final_neutron_energy'] = 24.0
"""

import math
from abins.constants import MILLI_EV_TO_WAVENUMBER

# Instruments constants  #############################
instruments = {
    'fwhm': 3.0,  # approximate value for the full width at half maximum for Gaussian experimental resolutions
    'TOSCA': {
        # TOSCA parameters for calculating Q^2
        'final_neutron_energy': 32.0,  # Final energy on the crystal analyser in cm-1
        'cos_scattering_angle': math.cos(2.356),  # Angle of the crystal analyser radians (NO LONGER USED)
        # The forward detector angle is rather specific as test-data was based on truncated value in radians
        'settings': {'Forward (TOSCA)': {'angles': [134.98885653282196]},
                     'Backward (TOSCA)': {'angles': [45.]},
                     'All detectors (TOSCA)': {'angles': [45., 134.98885653282196]}},
        'settings_default': 'Forward (TOSCA)',
        # TOSCA parameters for resolution function
        # sigma = tosca_a * omega * omega + tosca_b * omega + tosca_c
        # where sigma is width of Gaussian function
        'a': 0.0000001,
        'b': 0.005,
        'c': 2.5,},
    'Lagrange': {
        # Lagrange parameters for calculating Q^2
        'final_neutron_energy': 4.5 * MILLI_EV_TO_WAVENUMBER,  # Final energy on the crystal analyser in cm-1 (converted from eV)
        'scattering_angle_range': [10, 90],
        'angles_per_detector': 5,
        'settings_default': 'Cu(220) (Lagrange)',
        'settings': {'Cu(220) (Lagrange)': {'Ei_range_meV': [26, 500],
                                            'abs_resolution_meV': [7.6987e-5, 2.156e-2, -3.5961e-2],
                                            'low_energy_cutoff_meV': 25, 'low_energy_resolution_meV': 0.8},
                     'Cu(331) (Lagrange)': {'Ei_range_meV': [67, 500],
                                            'abs_resolution_meV': [-8.60511597e-08,  1.11911095e-04,
                                                                   5.15925386e-04,  6.58362118e-01]},
                     'Si(311) (Lagrange)': {'Ei_range_meV': [16.5, 60], 'abs_resolution_meV': 0.8},
                     'Si(111) (Lagrange)': {'Ei_range_meV': [4.5, 20], 'abs_resolution_meV': 0.8}}}
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

# Parameters related to estimated of spectra of high quantum orders by repeated convolution with fundamentals
autoconvolution = {
    'max_order': 10, # Highest quantum order accessed by autoconvolution
    'scale': 1.0,    # Scale factor applied to normalised convolution kernel
    'fine_bin_factor': 10
    }

performance = {
    'optimal_size': 5000000,  # this is used to create optimal size of chunk energies for which S is calculated
    'threads': 4  # number of threads used in parallel calculations
    }

# Experimental / debug features
development = {'isotropic_fundamentals': False}

all_parameters = {'instruments': instruments,
                  'hdf_groups': hdf_groups,
                  'sampling': sampling,
                  'performance': performance,
                  'development': development}

non_performance_parameters = {'instruments': instruments,
                              'hdf_groups': hdf_groups,
                              'sampling': sampling}
