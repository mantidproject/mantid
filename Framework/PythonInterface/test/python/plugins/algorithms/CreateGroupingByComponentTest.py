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


class CreateGroupingByComponentTest(unittest.TestCase):
    def setUp(self):
        self.default_kwargs = {"InstrumentName": "ENGINX", "OutputWorkspace": "test_group"}

    def tearDown(self):
        ADS.clear()

    def test_alg_with_default_parameters(self):
        alg = run_algorithm("CreateGroupingByComponent", **self.default_kwargs)
        self.assertTrue(alg.isExecuted())

        # by default, include is "" which all components satisfy, so there should be 1 unique group
        out_ws = ADS.retrieve(alg.getPropertyValue("OutputWorkspace"))
        y_dat = out_ws.extractY().reshape(-1)
        self.assertEqual(len(np.unique(y_dat)), 1)
        self.assertTrue(np.all(y_dat == np.ones(len(y_dat))))

    def test_alg_with_no_search_term_splits_into_n_groups(self):
        alg = run_algorithm("CreateGroupingByComponent", **self.default_kwargs, GroupSubdivision=5)
        self.assertTrue(alg.isExecuted())

        # by default, include is "" which all components satisfy, so sub-dividing this into 5 gives 5 unique groups
        out_ws = ADS.retrieve(alg.getPropertyValue("OutputWorkspace"))
        y_dat = out_ws.extractY().reshape(-1)
        y_test = create_target_grouping_from_step_inds(len(y_dat), [499, 1000, 1500, 2000])
        self.assertEqual(len(np.unique(y_dat)), 5)
        self.assertTrue(np.all(y_dat == y_test))

    def test_alg_with_include_term_splits_into_expected_groups(self):
        alg = run_algorithm("CreateGroupingByComponent", **self.default_kwargs, ComponentNameIncludes="detector-block")
        self.assertTrue(alg.isExecuted())

        # ENGINX has structure banks -> modules -> blocks -> pixels
        # with 2 banks, each containing 5 modules (each module then made up of 9 blocks)

        # here we have requested that we group the detector-blocks together.
        # As this is done under each parent component we should have 10 groups + 1 for the null group

        out_ws = ADS.retrieve(alg.getPropertyValue("OutputWorkspace"))
        y_dat = out_ws.extractY().reshape(-1)
        y_test = create_target_grouping_from_step_inds(len(y_dat), [240, 480, 720, 960, 1200, 1440, 1680, 1920, 2160, 2400])
        y_test[2400:] = 0.0  # this last group is null
        self.assertEqual(len(np.unique(y_dat)), 11)
        self.assertTrue(np.all(y_dat == y_test))

    def test_alg_with_exclude_term_splits_into_expected_groups(self):
        alg = run_algorithm("CreateGroupingByComponent", **self.default_kwargs, ComponentNameIncludes="pixel")
        self.assertTrue(alg.isExecuted())

        # ENGINX has diffraction structure: banks -> modules -> blocks -> pixels
        # with 2 banks, each containing 5 modules (each module then made up of 9 blocks)

        # AND forward transmission structure: bank -> columns -> pixels
        # with 1 banks, containing 10 columns (each module then made up of 10 pixels)

        # here we have requested that we group the pixel together.
        # As this is done under each parent component we should have 2*5*9 groups for the diffraction
        # and 10 for the transmission, so 100 total

        out_ws = ADS.retrieve(alg.getPropertyValue("OutputWorkspace"))

        steps = [
            26,
            53,
            80,
            107,
            134,
            161,
            188,
            215,
            240,
            266,
            293,
            320,
            347,
            374,
            401,
            428,
            455,
            480,
            506,
            533,
            560,
            587,
            614,
            641,
            668,
            695,
            720,
            746,
            773,
            800,
            827,
            854,
            881,
            908,
            935,
            960,
            986,
            1013,
            1040,
            1067,
            1094,
            1121,
            1148,
            1175,
            1200,
            1226,
            1253,
            1280,
            1307,
            1334,
            1361,
            1388,
            1415,
            1440,
            1466,
            1493,
            1520,
            1547,
            1574,
            1601,
            1628,
            1655,
            1680,
            1706,
            1733,
            1760,
            1787,
            1814,
            1841,
            1868,
            1895,
            1920,
            1946,
            1973,
            2000,
            2027,
            2054,
            2081,
            2108,
            2135,
            2160,
            2186,
            2213,
            2240,
            2267,
            2294,
            2321,
            2348,
            2375,
            2400,
            2410,
            2420,
            2430,
            2440,
            2450,
            2460,
            2470,
            2480,
            2490,
        ]

        y_dat = out_ws.extractY().reshape(-1)
        y_test = create_target_grouping_from_step_inds(len(y_dat), steps)

        self.assertEqual(len(np.unique(y_dat)), 100)
        self.assertTrue(np.all(y_dat == y_test))

        # Now we run again and exclude any component which has 'transmission' in the name

        alg = run_algorithm(
            "CreateGroupingByComponent", **self.default_kwargs, ComponentNameIncludes="pixel", ComponentNameExcludes="transmission"
        )

        self.assertTrue(alg.isExecuted())

        # Now only expect the 90 groups for the diffraction banks (+1 for the null group)

        y_test[2400:] = 0.0  # these last detectors are now in null group

        out_ws = ADS.retrieve(alg.getPropertyValue("OutputWorkspace"))
        y_dat = out_ws.extractY().reshape(-1)
        self.assertEqual(len(np.unique(y_dat)), 91)
        self.assertTrue(np.all(y_dat == y_test))

    def test_alg_with_single_exclude_branches_splits_into_expected_groups(self):
        alg = run_algorithm("CreateGroupingByComponent", **self.default_kwargs, ComponentNameIncludes="block", ExcludeBranches="SouthBank")
        self.assertTrue(alg.isExecuted())

        # ENGINX has diffraction structure: banks -> modules -> blocks -> pixels
        # with 2 banks (NorthBank and SouthBank), each containing 5 modules (each module then made up of 9 blocks)

        # here we have requested that we group the blocks together but exclude anything under SouthBank
        # so we expect the 5 groups +1 for null group

        out_ws = ADS.retrieve(alg.getPropertyValue("OutputWorkspace"))
        steps = [240, 480, 720, 960, 1200]

        y_dat = out_ws.extractY().reshape(-1)
        y_test = create_target_grouping_from_step_inds(len(y_dat), steps)
        y_test[1200:] = 0.0  # this last group is null
        self.assertEqual(len(np.unique(y_dat)), 6)
        self.assertTrue(np.all(y_dat == y_test))

    def test_alg_with_multiple_exclude_branches_splits_into_expected_groups(self):
        alg = run_algorithm(
            "CreateGroupingByComponent", **self.default_kwargs, ComponentNameIncludes="pixel", ExcludeBranches="SouthBank, TransmissionBank"
        )
        self.assertTrue(alg.isExecuted())

        # ENGINX has diffraction structure: banks -> modules -> blocks -> pixels
        # with 2 banks (NorthBank and SouthBank), each containing 5 modules (each module then made up of 9 blocks)

        # AND forward transmission structure: bank -> columns -> pixels
        # with 1 banks (TransmissionBank), containing 10 columns (each module then made up of 10 pixels)

        # here we have requested that we group the pixels together but exclude anything under NorthBank or TransmissionBank
        # so we expect the 45 groups (North Bank 5*9) +1 for null group

        out_ws = ADS.retrieve(alg.getPropertyValue("OutputWorkspace"))
        steps = [
            26,
            53,
            80,
            107,
            134,
            161,
            188,
            215,
            240,
            266,
            293,
            320,
            347,
            374,
            401,
            428,
            455,
            480,
            506,
            533,
            560,
            587,
            614,
            641,
            668,
            695,
            720,
            746,
            773,
            800,
            827,
            854,
            881,
            908,
            935,
            960,
            986,
            1013,
            1040,
            1067,
            1094,
            1121,
            1148,
            1175,
            1200,
        ]

        y_dat = out_ws.extractY().reshape(-1)
        y_test = create_target_grouping_from_step_inds(len(y_dat), steps)
        y_test[1200:] = 0.0  # this last group is null
        self.assertEqual(len(np.unique(y_dat)), 46)
        self.assertTrue(np.all(y_dat == y_test))

    def test_alg_with_bad_instrument_name_fails(self):
        alg = _init_alg(InstrumentName="FakeInstrument", OutputWorkspace="test")
        with self.assertRaisesRegex(RuntimeError, "Failed to find a matching instrument to the provided input:"):
            alg.execute()


def _init_alg(**kwargs):
    alg = AlgorithmManager.create("CreateGroupingByComponent")
    alg.initialize()
    for prop, value in kwargs.items():
        alg.setProperty(prop, value)
    return alg


def create_target_grouping_from_step_inds(n_dets, step_inds):
    # For these ENGINX tests we are fortunate as to how the IDF is set up that our expected groupings appear in order
    # this is just a helper function to save the lines in the file that are required for hard coding n x (2500,) arrays

    # in general, we are looking to generate arrays that have detector groups 1,1,...1,2,2,...,2,3,3,...,3 etc.
    # we have the indices of when the steps 1->2, 2->3 etc. occur

    # this is almost certainly not the best way to do this, but it gets it done
    testY = np.ones(n_dets) * (len(step_inds) + 1)
    for step in step_inds:
        testY[:step] -= 1.0
    return testY


if __name__ == "__main__":
    unittest.main()
