# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""
Parameters for instruments and Abins

These values are somewhat 'global' across Abins. In previous versions it was
recommended that they could be changed by editing this source file.
This is still possible, but in general it is recommended to edit the
value dictionaries through the Python API if possible. e.g.::

    import abins
    abins.parameters.instruments['TOSCA']['final_neutron_energy'] = 24.0
"""

import math
from abins.constants import MILLI_EV_TO_WAVENUMBER

# Instruments constants  #############################
instruments = {
    "fwhm": 3.0,  # approximate value for the full width at half maximum for Gaussian experimental resolutions
    "Ideal2D": {
        "resolution": 0.01,  # Width of broadening function as a fraction of incident energy
        "n_energy_bins": 1000,  # Number of energy bins (from zero to e_init)
        "q_size": 200,  # Number of q slices in output plot
        "q_range": (0, 20),  # Lower and upper limits of measurement q sampling
        "e_init": 4100.0,  # Incident energies in cm-1
        "settings_default": "",
        "settings": {"": {"chopper": ""}},
        "min_wavenumber": 0.0,
        "max_wavenumber": 4100.0,
    },
    "PANTHER": {
        "q_size": 100,
        "e_init": 150 * MILLI_EV_TO_WAVENUMBER,
        "settings_default": "",
        "settings": {"": {"chopper": ""}},
        "angle_range": (5, 140),
        "chopper_frequency_default": 400,
        "chopper_allowed_frequencies": list(range(50, 601, 50)),
        "min_wavenumber": 0.0,
        "max_wavenumber": 150 * MILLI_EV_TO_WAVENUMBER,
        # Resolution function fitted to incident energy and energy transfer:
        # sigma = polyval(abs_meV, ɛ) + polyval(ei_dependence, E_i) + polyval(ei_energy_product, E_i × ɛ)
        # (Here a quartic polynomial in ɛ, plus quadratic on Ei and cubic on ɛ×Ei)
        "resolution": {
            "abs_meV": [1.4819832651359686e-06, 0.0006472263718719711, 0.023616411911882462, 9.776165626074981],
            "ei_dependence": [0.0010075660656377956, -0.15243694390151533, 0],
            "ei_energy_product": [-2.3477397983667877e-13, -8.56972067941707e-09, -0.000790753342146401, 0],
        },
    },
    "MAPS": {
        "resolution": "pychop",
        "q_size": 100,
        "e_init": 500 * MILLI_EV_TO_WAVENUMBER,
        "max_wavenumber": 2000 * MILLI_EV_TO_WAVENUMBER,
        "settings_default": "A",
        "settings": {
            "A": {"chopper": "A"},
            "S": {"chopper": "S"},
        },
        "chopper_frequency_default": 400,
        "chopper_allowed_frequencies": list(range(50, 601, 50)),
    },
    "MARI": {
        "resolution": "pychop",
        "q_size": 100,
        "e_init": 400 * MILLI_EV_TO_WAVENUMBER,
        "settings_default": "A",
        "settings": {
            "A": {"chopper": "A"},
            "G": {"chopper": "G"},
            "R": {"chopper": "R"},
            "S": {"chopper": "S"},
        },
        "chopper_frequency_default": 400,
        "chopper_allowed_frequencies": list(range(50, 601, 50)),
    },
    "MERLIN": {
        "resolution": "pychop",
        "q_size": 100,
        "e_init": 400 * MILLI_EV_TO_WAVENUMBER,
        "settings_default": "G",
        "settings": {
            "G": {"chopper": "G"},
            "S": {"chopper": "S"},
        },
        "chopper_frequency_default": 400,
        "chopper_allowed_frequencies": list(range(50, 601, 50)),
    },
    "TOSCA": {
        # TOSCA parameters for calculating Q^2
        "final_neutron_energy": 32.0,  # Final energy on the crystal analyser in cm-1
        "cos_scattering_angle": math.cos(2.356),  # Angle of the crystal analyser radians (NO LONGER USED)
        "energy_bin_width": 1.0,  # Default bin width in wavenumber
        # The forward detector angle is rather specific as test-data was based on truncated value in radians
        "settings": {
            "Backward (TOSCA)": {"angles": [134.98885653282196]},
            "Forward (TOSCA)": {"angles": [45.0]},
            "All detectors (TOSCA)": {"angles": [45.0, 134.98885653282196]},
        },
        "settings_default": "Backward (TOSCA)",
        # TOSCA parameters for resolution function
        # sigma = tosca_a * omega * omega + tosca_b * omega + tosca_c
        # where sigma is width of Gaussian function
        "a": 0.0000001,
        "b": 0.005,
        "c": 2.5,
    },
    "Lagrange": {
        # Lagrange parameters for calculating Q^2
        "final_neutron_energy": 4.5 * MILLI_EV_TO_WAVENUMBER,  # Final energy on the crystal analyser in cm-1 (converted from eV)
        "scattering_angle_range": [10, 90],
        "angles_per_detector": 5,
        "energy_bin_width": 1.0,  # Default bin width in wavenumber
        "settings_default": "Cu(220) (Lagrange)",
        "settings": {
            "Cu(220) (Lagrange)": {
                "Ei_range_meV": [26, 500],
                "abs_resolution_meV": [7.6987e-5, 2.156e-2, -3.5961e-2],
                "low_energy_cutoff_meV": 25,
                "low_energy_resolution_meV": 0.8,
            },
            "Cu(331) (Lagrange)": {
                "Ei_range_meV": [67, 500],
                "abs_resolution_meV": [-8.60511597e-08, 1.11911095e-04, 5.15925386e-04, 6.58362118e-01],
            },
            "Si(311) (Lagrange)": {"Ei_range_meV": [16.5, 60], "abs_resolution_meV": 0.8},
            "Si(111) (Lagrange)": {"Ei_range_meV": [4.5, 20], "abs_resolution_meV": 0.8},
        },
    },
}

# Names of groups in HDF5 cache/output file ################
hdf_groups = {
    "ab_initio_data": "ABData",  # AbinsData (i.e. input from phonon calculation)
    "powder_data": "Powder",  # PowderData
    "crystal_data": "SingleCrystal",  # SingleCrystalData
    "s_data": "S",  # dynamical structure factor
}

# Parameters related to sampling density and numerical cutoffs ##########################
sampling = {
    "max_wavenumber": 4100.0,  # maximum wavenumber in cm^-1 taken into account while creating workspaces (exclusive)
    "min_wavenumber": 0.0,  # minimal wavenumber in cm^-1 taken into account while creating workspaces (exclusive)
    "bin_width": None,  # Step size for bins in wavenumber. This is modified at runtime by the Abins algorithm.
    # If set, it takes priority over the instrument n_energy_bins or energy_bin_width.
    "default_n_energy_bins": 500,  # Use if no bin_width, n_energy_bins or energy_bin_width is available.
    "frequencies_threshold": 0.0,  # minimum included frequency
    "s_relative_threshold": 0.01,  # low cutoff for S intensity (fraction of maximum S)
    "s_absolute_threshold": 1e-7,  # low cutoff for S intensity (absolute value)
    "broadening_scheme": "auto",
    "broadening_range": 3,  # N*SIGMA cutoff for broadening kernels
    "force_constants": {  # Parameters related to imported Force Constants data (e.g. Phonopy .yaml)
        "qpt_cutoff": 15.0
    },  # Distance in Angstrom determining q-point sampling mesh
}

# Parameters related to estimated of spectra of high quantum orders by repeated convolution with fundamentals
autoconvolution = {
    "fine_bin_factor": 10,  # Bin size reduction during autoconvolution
    "max_order": 10,  # Highest quantum order accessed by autoconvolution
    "min_order": 3,  # Lowest quantum order accessed by autoconvolution. (Abins v1 disregards this.)
}

performance = {
    "optimal_size": 5000000,  # this is used to create optimal size of chunk energies for which S is calculated
    "threads": 4,  # number of threads used in parallel calculations
}

# Experimental / debug features
development = {"isotropic_fundamentals": False}

all_parameters = {
    "instruments": instruments,
    "hdf_groups": hdf_groups,
    "sampling": sampling,
    "performance": performance,
    "development": development,
}

non_performance_parameters = {"instruments": instruments, "hdf_groups": hdf_groups, "sampling": sampling}
