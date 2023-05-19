# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import os
from typing import Any, Dict, Mapping
from unittest import TestCase

import numpy as np
from numpy.testing import assert_allclose

from abins.atomsdata import _AtomData
from abins.kpointsdata import KpointData


# Module with helper functions used to create tests.
def find_file(filename=None):
    """
    Calculates path of filename with the testing data. Path is determined in the platform independent way.

    :param filename: name of file to find
    :returns: full path for the file with the testing data
    """
    from mantid.api import FileFinder

    return FileFinder.Instance().getFullPath(filename)


def remove_output_files(list_of_names=None):
    """Removes output files created during a test."""

    # import ConfigService here to avoid:
    # RuntimeError: Pickling of "mantid.kernel._kernel.ConfigServiceImpl"
    # instances is not enabled (http://www.boost.org/libs/python/doc/v2/pickle.html)

    from mantid.kernel import ConfigService

    if not isinstance(list_of_names, list):
        raise ValueError("List of names is expected.")
    if not all(isinstance(i, str) for i in list_of_names):
        raise ValueError("Each name should be a string.")

    save_dir_path = ConfigService.getString("defaultsave.directory")
    if save_dir_path != "":  # default save directory set
        all_files = os.listdir(save_dir_path)
    else:
        all_files = os.listdir(os.getcwd())

    for filename in all_files:
        for name in list_of_names:
            if name in filename:
                full_path = os.path.join(save_dir_path, filename)
                if os.path.isfile(full_path):
                    os.remove(full_path)
                break


def dict_arrays_to_lists(mydict: Mapping[str, Any]) -> Dict[str, Any]:
    """Recursively convert numpy arrays in a nested dict to lists (i.e. valid JSON)

    Returns a processed *copy* of the input dictionary: in-place values will not be altered."""
    clean_dict = {}
    for key, value in mydict.items():
        if isinstance(value, np.ndarray):
            if hasattr(value, "imag"):
                # Treat imaginary components as alternating columns in array
                clean_dict[key] = value.view(dtype=float).tolist()
            else:
                clean_dict[key] = value.tolist()
        elif isinstance(value, dict):
            clean_dict[key] = dict_arrays_to_lists(value)
        else:
            clean_dict[key] = value
    return clean_dict


def assert_atom_almost_equal(ref_atom: _AtomData, atom: _AtomData, tester: TestCase = None) -> None:
    """Compare two items from AtomsData, raise AssertionError if different"""

    if tester is None:
        tester = TestCase()

    tester.assertAlmostEqual(ref_atom["mass"], atom["mass"])
    tester.assertEqual(ref_atom["sort"], atom["sort"])
    tester.assertEqual(ref_atom["symbol"], atom["symbol"])
    assert_allclose(ref_atom["coord"], atom["coord"])


def assert_kpoint_almost_equal(ref_kpt: KpointData, test_kpt: KpointData) -> None:
    """Compare two items from KpointsData, raise AssertionError if different"""

    assert_allclose(ref_kpt.frequencies, test_kpt.frequencies)
    assert_allclose(ref_kpt.k, test_kpt.k)
    assert_allclose(ref_kpt.weight, test_kpt.weight)
    assert_allclose(ref_kpt.atomic_displacements, test_kpt.atomic_displacements)
