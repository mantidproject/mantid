from __future__ import (absolute_import, division, print_function)
import unittest
from mantid import logger
from mantid.simpleapi import mtd, LoadAclimax, Load, CompareWorkspaces, DeleteWorkspace
from AbinsModules import AbinsTestHelpers
import numpy as np


def old_modules():
    """" Check if there are proper versions of  Python and numpy."""
    is_python_old = AbinsTestHelpers.old_python()
    if is_python_old:
        logger.warning("Skipping AbinsBasicTest because Python is too old.")

    is_numpy_old = AbinsTestHelpers.is_numpy_valid(np.__version__)
    if is_numpy_old:
        logger.warning("Skipping AbinsBasicTest because numpy is too old.")

    return is_python_old or is_numpy_old


def skip_if(skipping_criteria):
    """
    Skip all tests if the supplied function returns true.
    Python unittest.skipIf is not available in 2.6 (RHEL6) so we'll roll our own.
    """
    def decorate(cls):
        if skipping_criteria():
            for attr in cls.__dict__.keys():
                if callable(getattr(cls, attr)) and 'test' in attr:
                    delattr(cls, attr)
        return cls
    return decorate


@skip_if(old_modules)
class LoadAclimaxTest(unittest.TestCase):

    _aclimax_csv_file = "benzene_LoadAclimax.csv"  # used in both tests for origin and full
    _aclimax_csv_file_ref_origin = "BenzeneCSVLoadAclimax.nxs"
    _aclimax_csv_file_ref_full = "BenzeneFullCSVLoadAclimax.nxs"


    def tearDown(self):
        AbinsTestHelpers.remove_output_files(list_of_names=["LoadAclimax"])

    def test_wrong_input(self):
        # no name for workspace
        self.assertRaises(RuntimeError, LoadAclimax, aClimaxCVSFile=self._aclimax_csv_file)

    def test_good_case(self):


        # only origin
        wrk = LoadAclimax(aClimaxCVSFile=self._aclimax_csv_file)
        wrk_ref = Load(self._aclimax_csv_file_ref_origin)
        (result, messages) = CompareWorkspaces(wrk_ref, wrk)

        self.assertEqual(result, True)

        DeleteWorkspace(wrk_ref)
        DeleteWorkspace(wrk)

        # full =  origin + phonon_wings
        wrk = LoadAclimax(aClimaxCVSFile=self._aclimax_csv_file, PhononWings=True)
        wrk_ref = Load(self._aclimax_csv_file_ref_full)
        (result, messages) = CompareWorkspaces(wrk_ref, wrk)

        self.assertEqual(result, True)


if __name__ == "__main__":
    unittest.main()