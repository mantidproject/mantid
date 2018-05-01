# pylint: disable=too-many-public-methods,invalid-name
from __future__ import (absolute_import, division, print_function)

import unittest
from mantid.simpleapi import *
from mantid.api import *


class OSIRISDiffractionReductionTest(unittest.TestCase):

    def _assert_valid_reduction_output(self, **kwargs):
        workspace = OSIRISDiffractionReduction(StoreInADS=False,
                                               OutputWorkspace="__temp",
                                               **kwargs)

        self.assertTrue(isinstance(workspace, MatrixWorkspace),
                        'Result workspace should be a matrix workspace.')
        self.assertEqual(workspace.getAxis(0).getUnit().unitID(), 'dSpacing')
        self.assertEqual(workspace.getNumberHistograms(), 1)

    def test_basic_reduction_completes(self):
        """
        Sanity test to ensure the most basic reduction actually completes.
        """
        self._assert_valid_reduction_output(Sample=['OSI89813.raw'],
                                            CalFile='osiris_041_RES10.cal',
                                            Vanadium=['osi89757.raw'],
                                            SpectraMin=3,
                                            SpectraMax=361)

    def test_reduction_with_can_subtraction(self):
        """
        Tests reduction after subtraction of an empty can.
        """

        self._assert_valid_reduction_output(Sample=['OSI10203.raw'],
                                            CalFile='osiris_041_RES10.cal',
                                            Vanadium=['OSI10156.raw'],
                                            Container=['OSI10241.raw'],
                                            SpectraMin=3,
                                            SpectraMax=361)

    def test_reduction_with_can_subtraction_and_scale(self):
        """
        Tests reduction after subtraction of an empty can.
        """

        self._assert_valid_reduction_output(Sample=['OSI10203.raw'],
                                            CalFile='osiris_041_RES10.cal',
                                            Vanadium=['OSI10156.raw'],
                                            Container=['OSI10241.raw'],
                                            ContainerScaleFactor=0.5,
                                            SpectraMin=3,
                                            SpectraMax=361)

    def test_reduction_with_multiple_runs(self):
        """
        Test reduction with multiple sample and vanadium runs
        """
        self._assert_valid_reduction_output(Sample=['OSI10203.raw', 'OSI10204.RAW'],
                                            CalFile='osiris_041_RES10.cal',
                                            Vanadium=['OSI10156.raw', 'OSI10157.RAW'],
                                            ContainerScaleFactor=0.5,
                                            SpectraMin=3,
                                            SpectraMax=361)

    def test_reduction_with_multiple_sample_and_multiple_can(self):
        """
        Test reduction with multiple sample, vanadium and container runs
        """
        self._assert_valid_reduction_output(Sample=['OSI10203.raw', 'OSI10204.RAW'],
                                            CalFile='osiris_041_RES10.cal',
                                            Vanadium=['OSI10156.raw', 'OSI10157.RAW'],
                                            Container=['OSI10241.raw', 'OSI10242.RAW'],
                                            ContainerScaleFactor=0.5,
                                            SpectraMin=3,
                                            SpectraMax=361)

    def test_greater_sample_runs_no_container(self):
        """
        Test when the number of sample runs is greater than the number of vanadium runs
        """
        self._assert_valid_reduction_output(Sample=['OSI10203.raw', 'OSI10204.RAW'],
                                            CalFile='osiris_041_RES10.cal',
                                            Vanadium=['OSI10156.raw'],
                                            SpectraMin=3,
                                            SpectraMax=361)

    def test_greater_sample_runs_with_container(self):
        """
        Test when the number of sample runs is greater than the number of vanadium runs and a container is used
        """
        self._assert_valid_reduction_output(Sample=['OSI10203.raw', 'OSI10204.RAW'],
                                            CalFile='osiris_041_RES10.cal',
                                            Vanadium=['OSI10156.raw'],
                                            Container=['OSI10241.raw'],
                                            SpectraMin=3,
                                            SpectraMax=361)

    def test_greater_vanadium_runs_no_container(self):
        """
        Test when the number of vanadium runs is greater than the number of sample runs
        """
        self._assert_valid_reduction_output(Sample=['OSI10203.raw'],
                                            CalFile='osiris_041_RES10.cal',
                                            Vanadium=['OSI10156.raw', 'OSI10157.RAW'],
                                            SpectraMin=3,
                                            SpectraMax=361)

    def test_greater_vanadium_runs_with_container(self):
        """
        Test when the number of vanadium runs is greater than the number of sample runs and a container is used
        """
        self._assert_valid_reduction_output(Sample=['OSI10203.raw'],
                                            CalFile='osiris_041_RES10.cal',
                                            Vanadium=['OSI10156.raw', 'OSI10157.RAW'],
                                            Container=['OSI10241.raw'],
                                            SpectraMin=3,
                                            SpectraMax=361)

    def test_greater_container_runs(self):
        """
        Test when the number of container runs is greater than the number of sample or vanadium runs.
        """
        self._assert_valid_reduction_output(Sample=['OSI10203.raw'],
                                            CalFile='osiris_041_RES10.cal',
                                            Vanadium=['OSI10156.raw'],
                                            Container=['OSI10241.raw', 'OSI10242.RAW'],
                                            ContainerScaleFactor=0.5,
                                            SpectraMin=3,
                                            SpectraMax=361)

    def test_mismatch_sample_container_d_ranges(self):
        """
        Test error handling when there is no overlap between the sample and container d-ranges
        """
        self.assertRaises(RuntimeError,
                          OSIRISDiffractionReduction,
                          Sample=['OSI89813.raw'],
                          CalFile='osiris_041_RES10.cal',
                          Vanadium=['osi89757.raw'],
                          Container=['OSI10241.raw'],
                          SpectraMin=3,
                          SpectraMax=361)

    def test_spectra_with_bad_detectors(self):
        """
        Tests reduction with a spectra range that includes no selected Detectors from the Cal file
        """

        self.assertRaises(RuntimeError,
                          OSIRISDiffractionReduction,
                          Sample=['OSI10203.raw'],
                          CalFile='osiris_041_RES10.cal',
                          Vanadium=['OSI10156.raw'],
                          Container=['OSI10241.raw'],
                          SpectraMin=16,
                          SpectraMax=17)

    def test_failure_no_vanadium(self):
        """
        Tests error handling when failed to obtain a vanadium run.
        """
        self.assertRaises(RuntimeError,
                          OSIRISDiffractionReduction,
                          Sample=['OSI89813.raw'],
                          CalFile='osiris_041_RES10.cal',
                          Vanadium=['OSI10156.raw'],
                          SpectraMin=3,
                          SpectraMax=361)

if __name__ == '__main__':
    unittest.main()
