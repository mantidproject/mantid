# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from mantid.simpleapi import (
    AnalysisDataService as ADS,
    AlgorithmManager,
)
from testhelpers import run_algorithm
import numpy as np


class CreateGroupingByComponenetTest(unittest.TestCase):
    def setUp(self):
        self.default_kwargs = {"InstrumentName": "ENGINX", "OutputWorkspace": "test_group"}

    def tearDown(self):
        ADS.clear()

    def test_alg_with_default_parameters(self):
        alg = run_algorithm("CreateGroupingByComponent", **self.default_kwargs)
        self.assertTrue(alg.isExecuted())

        # by default, include is "" which all components satisfy, so there should be 1 unique group
        out_ws = ADS.retrieve(alg.getPropertyValue("OutputWorkspace"))
        self.assertEqual(len(np.unique(out_ws.extractY())), 1)

    def test_alg_with_no_search_term_splits_into_n_groups(self):
        alg = run_algorithm("CreateGroupingByComponent", **self.default_kwargs, GroupSubdivision=5)
        self.assertTrue(alg.isExecuted())

        # by default, include is "" which all components satisfy, so sub-dividing this into 5 gives 5 unique groups
        out_ws = ADS.retrieve(alg.getPropertyValue("OutputWorkspace"))
        self.assertEqual(len(np.unique(out_ws.extractY())), 5)

    def test_alg_with_include_term_splits_into_expected_groups(self):
        alg = run_algorithm("CreateGroupingByComponent", **self.default_kwargs, IncludeComponents="detector-block")
        self.assertTrue(alg.isExecuted())

        # ENGINX has structure banks -> modules -> blocks -> pixels
        # with 2 banks, each containing 5 modules (each module then made up of 9 blocks)

        # here we have requested that we group the detector-blocks together.
        # As this is done under each parent component we should have 10 groups + 1 for the null group

        out_ws = ADS.retrieve(alg.getPropertyValue("OutputWorkspace"))
        self.assertEqual(len(np.unique(out_ws.extractY())), 11)

    def test_alg_with_exclude_term_splits_into_expected_groups(self):
        alg = run_algorithm("CreateGroupingByComponent", **self.default_kwargs, IncludeComponents="pixel")
        self.assertTrue(alg.isExecuted())

        # ENGINX has diffraction structure: banks -> modules -> blocks -> pixels
        # with 2 banks, each containing 5 modules (each module then made up of 9 blocks)

        # AND forward transmission structure: bank -> columns -> pixels
        # with 1 banks, containing 10 columns (each module then made up of 10 pixels)

        # here we have requested that we group the pixel together.
        # As this is done under each parent component we should have 2*5*9 groups for the diffraction
        # and 10 for the transmission, so 100 total

        out_ws = ADS.retrieve(alg.getPropertyValue("OutputWorkspace"))
        self.assertEqual(len(np.unique(out_ws.extractY())), 100)

        # Now we run again and exclude any component which has 'transmission' in the name

        alg = run_algorithm("CreateGroupingByComponent", **self.default_kwargs, IncludeComponents="pixel", ExcludeComponents="transmission")
        self.assertTrue(alg.isExecuted())

        # Now only expect the 90 groups for the diffraction banks (+1 for the null group)

        out_ws = ADS.retrieve(alg.getPropertyValue("OutputWorkspace"))
        self.assertEqual(len(np.unique(out_ws.extractY())), 91)

    def test_alg_with_single_exclude_branches_splits_into_expected_groups(self):
        alg = run_algorithm("CreateGroupingByComponent", **self.default_kwargs, IncludeComponents="block", ExcludeBranches="NorthBank")
        self.assertTrue(alg.isExecuted())

        # ENGINX has diffraction structure: banks -> modules -> blocks -> pixels
        # with 2 banks (NorthBank and SouthBank), each containing 5 modules (each module then made up of 9 blocks)

        # here we have requested that we group the blocks together but exclude anything under NorthBank
        # so we expect the 5 groups +1 for null group

        out_ws = ADS.retrieve(alg.getPropertyValue("OutputWorkspace"))
        self.assertEqual(len(np.unique(out_ws.extractY())), 6)

    def test_alg_with_multiple_exclude_branches_splits_into_expected_groups(self):
        alg = run_algorithm(
            "CreateGroupingByComponent", **self.default_kwargs, IncludeComponents="pixel", ExcludeBranches="NorthBank, TransmissionBank"
        )
        self.assertTrue(alg.isExecuted())

        # ENGINX has diffraction structure: banks -> modules -> blocks -> pixels
        # with 2 banks (NorthBank and SouthBank), each containing 5 modules (each module then made up of 9 blocks)

        # AND forward transmission structure: bank -> columns -> pixels
        # with 1 banks (TransmissionBank), containing 10 columns (each module then made up of 10 pixels)

        # here we have requested that we group the pixels together but exclude anything under NorthBank or TransmissionBank
        # so we expect the 45 groups (South Bank 5*9) +1 for null group

        out_ws = ADS.retrieve(alg.getPropertyValue("OutputWorkspace"))
        self.assertEqual(len(np.unique(out_ws.extractY())), 46)

    def test_alg_with_bad_instrument_name_fails(self):
        alg = _init_alg(InstrumentName="FakeInstrument", OutputWorkspace="test")
        self.assertRaises(RuntimeError, alg.execute)


def _init_alg(**kwargs):
    alg = AlgorithmManager.create("CreateGroupingByComponent")
    alg.initialize()
    for prop, value in kwargs.items():
        alg.setProperty(prop, value)
    return alg


if __name__ == "__main__":
    unittest.main()
