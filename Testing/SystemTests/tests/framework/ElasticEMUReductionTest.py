# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

import os
import zipfile
import tempfile
import systemtesting
from mantid.simpleapi import mtd
from mantid.api import FileFinder
from mantid.simpleapi import ElasticEMUauReduction


class ElasticEMUauReductionAxisTest(systemtesting.MantidSystemTest):
    def __init__(self):
        systemtesting.MantidSystemTest.__init__(self)
        self.tolerance = 1e-6

    def runTest(self):
        # find the zip file
        zpath = FileFinder.getFullPath("EMUauElasticTestData.zip")
        zfile = zipfile.ZipFile(zpath)

        # expand to tempory folder and perform the reduction
        with tempfile.TemporaryDirectory() as tempdir:
            zfile.extractall(tempdir)
            prevdir = os.getcwd()
            os.chdir(tempdir)
            ElasticEMUauReduction(SampleRuns="19862:0-1", SpectrumAxis="Q", OutputWorkspace="test")
            os.chdir(prevdir)

        self.assertTrue("test_Q_bm" in mtd, "Expected output workspace group in ADS")
        wg = mtd["test_Q_bm"]
        index = dict([(tag, i) for i, tag in enumerate(wg.getNames())])
        ws = wg.getItem(index["test_Q_bm_2DT"])
        xv = ws.getAxis(0).extractValues()
        self.assertDelta(xv[0], 0.0, 0.01, "Unexpected minimum energy transfer value")
        self.assertDelta(xv[-1], 2.0, 0.01, "Unexpected minimum energy transfer value")
        yv = ws.getAxis(1).extractValues()
        self.assertDelta(yv[0], 90.672, 0.01, "Unexpected minimum Q value")
        self.assertDelta(yv[-1], 272.015, 0.01, "Unexpected minimum Q value")


class ElasticEMUauReductionSingleScanAxisTest(systemtesting.MantidSystemTest):
    def __init__(self):
        systemtesting.MantidSystemTest.__init__(self)
        self.tolerance = 1e-6

    def runTest(self):
        # find the zip file
        zpath = FileFinder.getFullPath("EMUauElasticTestData.zip")
        zfile = zipfile.ZipFile(zpath)

        # expand to tempory folder and perform the reduction
        with tempfile.TemporaryDirectory() as tempdir:
            zfile.extractall(tempdir)
            prevdir = os.getcwd()
            os.chdir(tempdir)
            ElasticEMUauReduction(SampleRuns="19862:0", SpectrumAxis="Q", OutputWorkspace="test")
            os.chdir(prevdir)

        self.assertTrue("test_Q_bm" in mtd, "Expected output workspace group in ADS")
        wg = mtd["test_Q_bm"]
        index = dict([(tag, i) for i, tag in enumerate(wg.getNames())])
        ws = wg.getItem(index["test_Q_bm_2DT"])
        xv = ws.getAxis(0).extractValues()
        self.assertDelta(xv[0], 0.0, 0.01, "Unexpected minimum energy transfer value")
        self.assertDelta(xv[-1], 2.0, 0.01, "Unexpected minimum energy transfer value")
        yv = ws.getAxis(1).extractValues()
        self.assertEqual(len(yv), 1)
        self.assertDelta(yv[0], 90.34, 0.01, "Unexpected Q value")
        # note that the time period is not the same because of the gap
        # in the between scan 0 and scan 1 when compared against the previous test


class ElasticEMUauReductionWSTest(systemtesting.MantidSystemTest):
    def __init__(self):
        systemtesting.MantidSystemTest.__init__(self)
        self.tolerance = 1e-6

    def runTest(self):
        # find the zip file
        zpath = FileFinder.getFullPath("EMUauElasticTestData.zip")
        zfile = zipfile.ZipFile(zpath)

        # expand to tempory folder and perform the reduction
        with tempfile.TemporaryDirectory() as tempdir:
            zfile.extractall(tempdir)
            prevdir = os.getcwd()
            os.chdir(tempdir)
            ElasticEMUauReduction(SampleRuns="19862:0-1", SpectrumAxis="Q", OutputWorkspace="test")
            os.chdir(prevdir)

    def validate(self):
        self.disableChecking.append("Instrument")
        return "test_Q_bm_I_Scan", "EMUauElasticTestScan.nxs"
