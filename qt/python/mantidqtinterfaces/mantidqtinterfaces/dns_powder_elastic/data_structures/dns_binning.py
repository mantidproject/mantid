# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

import numpy as np

from mantidqtinterfaces.dns_powder_tof.data_structures.object_dict import ObjectDict


DEFAULT_TWO_THETA_MIN = 5.0
DEFAULT_TWO_THETA_RANGE = 115.0
DEFAULT_TWO_THETA_STEP = 5.0
DEFAULT_OMEGA_MIN = 5.0
DEFAULT_OMEGA_MAX = 5.0
DEFAULT_OMEGA_STEP = 0.0


def get_automatic_omega_binning(sample_data):
    """
    Determines automatic sample rotation binning parameters from selected sample data.
    """
    selected_omega = [-x["det_rot"] + x["sample_rot"] for x in sample_data]

    unique_omega = sorted(list(set(selected_omega)))
    # default for the case when files with same values of omega are selected
    omega_step = DEFAULT_OMEGA_STEP
    number_omega_bins = 0

    omega_min = min(unique_omega, default=DEFAULT_OMEGA_MIN)
    omega_max = max(unique_omega, default=DEFAULT_OMEGA_MAX)
    omega_steps = np.diff(unique_omega)
    # use non-default step when several omega angles are selected
    if omega_steps.size != 0:
        omega_values, omega_counts = np.unique(omega_steps, return_counts=True)
        omega_ind = np.argmax(omega_counts)
        most_freq_omega_step = omega_values[omega_ind]
        omega_step = most_freq_omega_step

    if omega_step != 0:
        omega_range = np.arange(omega_min, omega_max + omega_step, omega_step)
        number_omega_bins = omega_range.size

    omega_binning_dict = {"omega_min": omega_min, "omega_max": omega_max, "omega_bin_size": omega_step, "omega_nbins": number_omega_bins}

    return omega_binning_dict


def get_automatic_two_theta_binning(sample_data):
    """
    Determines automatic two theta binning parameters from selected sample data.
    """
    selected_det_rot = [-x["det_rot"] for x in sample_data]

    unique_det_rot = sorted(list(set(selected_det_rot)))
    # default for the case when files with same values of det_rot are selected
    two_theta_step = DEFAULT_TWO_THETA_STEP

    two_theta_min = min(unique_det_rot, default=DEFAULT_TWO_THETA_MIN)
    two_theta_max = max(unique_det_rot, default=DEFAULT_TWO_THETA_MIN) + DEFAULT_TWO_THETA_RANGE
    two_theta_steps = np.diff(unique_det_rot)
    # use non-default step when several det_rot angles are selected
    if two_theta_steps.size != 0:
        two_theta_step_values, two_theta_step_counts = np.unique(two_theta_steps, return_counts=True)
        two_theta_step_ind = np.argmax(two_theta_step_counts)
        most_freq_two_theta_step = two_theta_step_values[two_theta_step_ind]
        two_theta_step = most_freq_two_theta_step

    two_theta_range = np.arange(two_theta_min, two_theta_max + two_theta_step, two_theta_step)
    number_two_theta_bins = two_theta_range.size

    two_theta_binning_dict = {
        "two_theta_min": two_theta_min,
        "two_theta_max": two_theta_max,
        "two_theta_bin_size": two_theta_step,
        "nbins": number_two_theta_bins,
    }

    return two_theta_binning_dict


class DNSBinning(ObjectDict):
    """
    Stores DNS binning information. If the number of bins is not given,
    it is calculated from the step size, also defines bin_edges.
    """

    def __init__(self, xmin, xmax, step):
        super().__init__()
        self["min"] = xmin
        self["max"] = xmax
        self["step"] = step
        self["bin_edge_min"] = xmin - step / 2
        self["bin_edge_max"] = xmax + step / 2
        if step == 0:
            self["range"] = np.array([])
            self["bin_edge_range"] = np.array([])
        else:
            self["range"] = np.arange(xmin, xmax + step, step)
            self["bin_edge_range"] = np.arange(self["bin_edge_min"], self["bin_edge_max"] + self["step"], self["step"])
        self["nbins"] = self["range"].size
