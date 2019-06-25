# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

import unittest
from testhelpers import run_algorithm
from mantid.api import AnalysisDataService
from math import pi
import os
from mantid.simpleapi import LoadDNSLegacy


class LoadDNSLegacyTest(unittest.TestCase):
    def setUp(self):
        self.curtable = 'dns_coil_currents.txt'
        self._createCurrentsTable(self.curtable)

    def tearDown(self):
        os.remove(self.curtable)

    def test_LoadValidData(self):
        outputWorkspaceName = "LoadDNSLegacyTest_Test1"
        filename = "dn134011vana.d_dat"
        alg_test = run_algorithm("LoadDNSLegacy", Filename=filename, Normalization='no',
                                 OutputWorkspace=outputWorkspaceName, CoilCurrentsTable=self.curtable)
        self.assertTrue(alg_test.isExecuted())

        # Verify some values
        ws = AnalysisDataService.retrieve(outputWorkspaceName)
        # dimensions
        self.assertEqual(24, ws.getNumberHistograms())
        self.assertEqual(2,  ws.getNumDims())
        # data array
        self.assertEqual(31461, ws.readY(1))
        self.assertEqual(13340, ws.readY(23))
        # sample logs
        run = ws.getRun()
        self.assertEqual(-8.54, run.getProperty('deterota').value)
        self.assertEqual(8332872, run.getProperty('mon_sum').value)
        self.assertEqual('z', run.getProperty('polarisation').value)
        self.assertEqual('7', str(run.getProperty('polarisation_comment').value))
        self.assertEqual('no', run.getProperty('normalized').value)
        # check whether detector bank is rotated
        det = ws.getDetector(0)
        self.assertAlmostEqual(8.54, ws.detectorSignedTwoTheta(det)*180/pi)
        run_algorithm("DeleteWorkspace", Workspace=outputWorkspaceName)
        return

    def _createIncompleteFile(self, filename):
        """
        creates an incomplete data file
        """
        with open(filename, "w") as f:
            f.write("# DNS Data userid=sa,exp=961,file=988,sample=run2")
            f.write("#--------------------------------------------------------------------------")
            f.write("# 9")
            f.write("# User: Some User")
            f.close()
        return

    def _createCurrentsTable(self, filename):
        """
        creates a text file with the coil currents table
        """
        with open(filename, "w") as f:
            f.write("polarisation\t comment\t C_a\t C_b\t C_c\t C_z\n")
            f.write("x\t 7\t 0\t -2\t -0.77\t -2.21\n")
            f.write("y\t 7\t 0\t 1.60\t -2.77\t -2.21\n")
            f.write("z\t 7\t 0\t 0.11\t -0.5\t 0\n")
            f.close()
        return

    def test_LoadInvalidData(self):
        outputWorkspaceName = "LoadDNSLegacyTest_Test2"
        filename = "dns-incomplete.d_dat"
        self._createIncompleteFile(filename)
        self.assertRaises(RuntimeError, LoadDNSLegacy,  Filename=filename,
                          OutputWorkspace=outputWorkspaceName, CoilCurrentsTable=self.curtable)
        os.remove(filename)

    def test_LoadNormalizeToDuration(self):
        outputWorkspaceName = "LoadDNSLegacyTest_Test1"
        filename = "dn134011vana.d_dat"
        alg_test = run_algorithm("LoadDNSLegacy", Filename=filename, Normalization='duration',
                                 OutputWorkspace=outputWorkspaceName, CoilCurrentsTable=self.curtable)
        self.assertTrue(alg_test.isExecuted())

        # Verify some values
        ws = AnalysisDataService.retrieve(outputWorkspaceName)
        # dimensions
        self.assertEqual(24, ws.getNumberHistograms())
        self.assertEqual(2,  ws.getNumDims())
        # data array
        self.assertAlmostEqual(31461.0/600.0, ws.readY(1))
        self.assertAlmostEqual(13340.0/600.0, ws.readY(23))
        # sample logs
        run = ws.getRun()
        self.assertEqual(-8.54, run.getProperty('deterota').value)
        self.assertEqual(8332872, run.getProperty('mon_sum').value)
        self.assertEqual('duration', run.getProperty('normalized').value)
        # check whether detector bank is rotated
        det = ws.getDetector(0)
        self.assertAlmostEqual(8.54, ws.detectorSignedTwoTheta(det)*180/pi)
        run_algorithm("DeleteWorkspace", Workspace=outputWorkspaceName)
        return

    def test_LoadNormalizeToMonitor(self):
        outputWorkspaceName = "LoadDNSLegacyTest_Test5"
        filename = "dn134011vana.d_dat"
        alg_test = run_algorithm("LoadDNSLegacy", Filename=filename, Normalization='monitor',
                                 OutputWorkspace=outputWorkspaceName, CoilCurrentsTable=self.curtable)
        self.assertTrue(alg_test.isExecuted())

        # Verify some values
        ws = AnalysisDataService.retrieve(outputWorkspaceName)
        # dimensions
        self.assertEqual(24, ws.getNumberHistograms())
        self.assertEqual(2,  ws.getNumDims())
        # data array
        self.assertAlmostEqual(31461.0/8332872.0, ws.readY(1))
        self.assertAlmostEqual(13340.0/8332872.0, ws.readY(23))
        # sample logs
        run = ws.getRun()
        self.assertEqual(-8.54, run.getProperty('deterota').value)
        self.assertEqual(8332872, run.getProperty('mon_sum').value)
        self.assertEqual('monitor', run.getProperty('normalized').value)
        # check whether detector bank is rotated
        det = ws.getDetector(0)
        self.assertAlmostEqual(8.54, ws.detectorSignedTwoTheta(det)*180/pi)
        run_algorithm("DeleteWorkspace", Workspace=outputWorkspaceName)
        return

    def test_LoadNoCurtable(self):
        outputWorkspaceName = "LoadDNSLegacyTest_Test6"
        filename = "dn134011vana.d_dat"
        alg_test = run_algorithm("LoadDNSLegacy", Filename=filename, Normalization='no',
                                 OutputWorkspace=outputWorkspaceName)
        self.assertTrue(alg_test.isExecuted())

        # Verify some values
        ws = AnalysisDataService.retrieve(outputWorkspaceName)
        # dimensions
        self.assertEqual(24, ws.getNumberHistograms())
        self.assertEqual(2,  ws.getNumDims())
        # data array
        self.assertEqual(31461, ws.readY(1))
        self.assertEqual(13340, ws.readY(23))
        # sample logs
        run = ws.getRun()
        self.assertEqual(-8.54, run.getProperty('deterota').value)
        self.assertEqual(8332872, run.getProperty('mon_sum').value)
        self.assertEqual('z', run.getProperty('polarisation').value)
        self.assertEqual('7', str(run.getProperty('polarisation_comment').value))
        self.assertEqual('no', run.getProperty('normalized').value)
        # check whether detector bank is rotated
        det = ws.getDetector(0)
        self.assertAlmostEqual(8.54, ws.detectorSignedTwoTheta(det)*180/pi)
        run_algorithm("DeleteWorkspace", Workspace=outputWorkspaceName)
        return

    def test_LoadTOF(self):
        outputWorkspaceName = "LoadDNSLegacyTest_Test7"
        filename = "dnstof.d_dat"
        tof1 = 424.668     # must be changed if L1 will change
        alg_test = run_algorithm("LoadDNSLegacy", Filename=filename, Normalization='no',
                                 ElasticChannel=65, OutputWorkspace=outputWorkspaceName)
        self.assertTrue(alg_test.isExecuted())

        # Verify some values
        ws = AnalysisDataService.retrieve(outputWorkspaceName)
        # dimensions
        self.assertEqual(24, ws.getNumberHistograms())
        self.assertEqual(100,  ws.getNumberBins())
        # data array
        self.assertEqual(8, ws.readY(19)[23])
        self.assertAlmostEqual(tof1, ws.readX(0)[0], 3)
        self.assertAlmostEqual(tof1+40.1*100, ws.readX(0)[100], 3)
        # sample logs
        run = ws.getRun()
        self.assertEqual(-7.5, run.getProperty('deterota').value)
        self.assertEqual(100, run.getProperty('tof_channels').value)
        self.assertEqual(51428, run.getProperty('mon_sum').value)
        self.assertEqual('z', run.getProperty('polarisation').value)
        self.assertEqual(65, run.getProperty('EPP').value)  # check that EPP is not taken from file
        self.assertEqual('7', str(run.getProperty('polarisation_comment').value))
        self.assertEqual('no', run.getProperty('normalized').value)
        # check whether detector bank is rotated
        det = ws.getDetector(0)
        self.assertAlmostEqual(7.5, ws.detectorSignedTwoTheta(det)*180/pi)
        run_algorithm("DeleteWorkspace", Workspace=outputWorkspaceName)
        return

    def test_LoadWavelength(self):
        outputWorkspaceName = "LoadDNSLegacyTest_Test8"
        filename = "dn134011vana.d_dat"
        alg_test = run_algorithm("LoadDNSLegacy", Filename=filename, Normalization='no',
                                 OutputWorkspace=outputWorkspaceName, CoilCurrentsTable=self.curtable,
                                 Wavelength=5.7)

        self.assertTrue(alg_test.isExecuted())

        # Verify some values
        ws = AnalysisDataService.retrieve(outputWorkspaceName)
        # dimensions
        self.assertEqual(24, ws.getNumberHistograms())
        self.assertEqual(2,  ws.getNumDims())
        # data array
        self.assertEqual(31461, ws.readY(1))
        self.assertEqual(13340, ws.readY(23))
        self.assertAlmostEqual(5.7, ws.readX(1)[0], 3)
        self.assertAlmostEqual(5.7, ws.readX(23)[0], 3)
        # sample logs
        run = ws.getRun()
        self.assertEqual(5.7, run.getProperty('wavelength').value)
        self.assertAlmostEqual(2.51782, run.getProperty('Ei').value, 3)
        run_algorithm("DeleteWorkspace", Workspace=outputWorkspaceName)
        return

if __name__ == '__main__':
    unittest.main()
