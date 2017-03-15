#pylint: disable=too-many-public-methods,invalid-name
from __future__ import (absolute_import, division, print_function)

import unittest
from mantid.simpleapi import *
from mantid.api import *


class OSIRISDiffractionReductionTest(unittest.TestCase):

    def test_basic_reduction_completes(self):
        """
        Sanity test to ensure the most basic reduction actually completes.
        """

        wks = OSIRISDiffractionReduction(Sample=['OSI89813.raw'],
                                         CalFile='osiris_041_RES10.cal',
                                         Vanadium=['osi89757.raw'],
                                         SpectraMin=3,
                                         SpectraMax=361)

        self.assertTrue(isinstance(wks, MatrixWorkspace), 'Result workspace should be a matrix workspace.')
        self.assertEqual(wks.getAxis(0).getUnit().unitID(), 'dSpacing')
        self.assertEqual(wks.getNumberHistograms(), 1)


    def test_reduction_with_manual_drange_completes(self):
        """
        Test to ensure reduction with manual dRange selection completes.
        The run here is for dRange 4.
        """

        wks = OSIRISDiffractionReduction(Sample=['OSI10203.raw'],
                                         CalFile='osiris_041_RES10.cal',
                                         Vanadium=['OSI10156.raw'],
                                         SpectraMin=3,
                                         SpectraMax=361,
                                         DetectDRange=False,
                                         DRange=4)

        self.assertTrue(isinstance(wks, MatrixWorkspace), 'Result workspace should be a matrix workspace.')
        self.assertEqual(wks.getAxis(0).getUnit().unitID(), 'dSpacing')
        self.assertEqual(wks.getNumberHistograms(), 1)


    def test_reduction_with_can_subtraction(self):
        """
        Tests reduction after subtraction of an empty can.
        """

        wks = OSIRISDiffractionReduction(Sample=['OSI10203.raw'],
                                         CalFile='osiris_041_RES10.cal',
                                         Vanadium=['OSI10156.raw'],
                                         Container=['OSI10241.raw'],
                                         SpectraMin=3,
                                         SpectraMax=361,
                                         DetectDRange=False,
                                         DRange=4)

        self.assertTrue(isinstance(wks, MatrixWorkspace), 'Result workspace should be a matrix workspace.')
        self.assertEqual(wks.getAxis(0).getUnit().unitID(), 'dSpacing')
        self.assertEqual(wks.getNumberHistograms(), 1)


    def test_reduction_with_can_subtraction_and_scale(self):
        """
        Tests reduction after subtraction of an empty can.
        """

        wks = OSIRISDiffractionReduction(Sample=['OSI10203.raw'],
                                         CalFile='osiris_041_RES10.cal',
                                         Vanadium=['OSI10156.raw'],
                                         Container=['OSI10241.raw'],
                                         ContainerScaleFactor=0.5,
                                         SpectraMin=3,
                                         SpectraMax=361,
                                         DetectDRange=False,
                                         DRange=4)

        self.assertTrue(isinstance(wks, MatrixWorkspace), 'Result workspace should be a matrix workspace.')
        self.assertEqual(wks.getAxis(0).getUnit().unitID(), 'dSpacing')
        self.assertEqual(wks.getNumberHistograms(), 1)


    def test_reduction_with_multiple_runs(self):
        """
        Test reduction with multiple sample and vanadium runs
        """
        wks = OSIRISDiffractionReduction(Sample=['OSI10203.raw','OSI10204.RAW'],
                                         CalFile='osiris_041_RES10.cal',
                                         Vanadium=['OSI10156.raw','OSI10157.RAW'],
                                         ContainerScaleFactor=0.5,
                                         SpectraMin=3,
                                         SpectraMax=361,
                                         DetectDRange=True)
        self.assertTrue(isinstance(wks, MatrixWorkspace), 'Result workspace should be a matrix workspace.')
        self.assertEqual(wks.getAxis(0).getUnit().unitID(), 'dSpacing')
        self.assertEqual(wks.getNumberHistograms(), 1)


    def test_reduction_with_multiple_sample_and_multiple_can(self):
        """
        Test reduction with multiple sample, vanadium and container runs
        """
        wks = OSIRISDiffractionReduction(Sample=['OSI10203.raw','OSI10204.RAW'],
                                         CalFile='osiris_041_RES10.cal',
                                         Vanadium=['OSI10156.raw','OSI10157.RAW'],
                                         Container=['OSI10241.raw','OSI10242.RAW'],
                                         ContainerScaleFactor=0.5,
                                         SpectraMin=3,
                                         SpectraMax=361,
                                         DetectDRange=True)
        self.assertTrue(isinstance(wks, MatrixWorkspace), 'Result workspace should be a matrix workspace.')
        self.assertEqual(wks.getAxis(0).getUnit().unitID(), 'dSpacing')
        self.assertEqual(wks.getNumberHistograms(), 1)

    def test_mismatch_sample_container_bins(self):
        """
        Test when the container bins are a different size to the sample bins.
        """
        wks = OSIRISDiffractionReduction(Sample=['OSI89813.raw'],
                                         CalFile='osiris_041_RES10.cal',
                                         Vanadium=['osi89757.raw'],
                                         Container=['OSI10241.raw'],
                                         SpectraMin=3,
                                         SpectraMax=361)
        self.assertTrue(isinstance(wks, MatrixWorkspace), 'Result workspace should be a matrix workspace.')
        self.assertEqual(wks.getAxis(0).getUnit().unitID(), 'dSpacing')
        self.assertEqual(wks.getNumberHistograms(), 1)

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


    def test_mismatch_sample_vanadium_numbers(self):
        """
        Test error handling when number of samples is not equal to number of vanadium
        """
        self.assertRaises(RuntimeError,
                          OSIRISDiffractionReduction,
                          Sample=['OSI89813.raw', 'OSI89814.RAW'],
                          CalFile='osiris_041_RES10.cal',
                          Vanadium=['OSI10156.raw'],
                          SpectraMin=3,
                          SpectraMax=361)


    def test_mismatch_sample_container_numbers(self):
        """
        Test error handling when number of samples is not equal to number of containers
        """
        self.assertRaises(RuntimeError,
                          OSIRISDiffractionReduction,
                          Sample=['OSI89813.raw', 'OSI89814.RAW'],
                          CalFile='osiris_041_RES10.cal',
                          Vanadium=['OSI10156.raw', 'OSI10157.RAW'],
                          Container=['OSI10241.raw'],
                          SpectraMin=3,
                          SpectraMax=361)


if __name__ == '__main__':
    unittest.main()
