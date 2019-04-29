# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, print_function)

import numpy as np
import math
import unittest
from mantid.simpleapi import *
from mantid.api import *


class MuonMaxEntTest(unittest.TestCase):

    def genData(self):
        x_data = np.linspace(0, 30., 100)
        e_data = np.cos(0.0 * x_data)
        y_data = np.sin(2.3 * x_data + 0.1) * np.exp(-x_data / 2.19703)
        inputData = CreateWorkspace(
            DataX=x_data,
            DataY=y_data,
            DataE=e_data,
            UnitX='Time')
        return inputData

    def genData2(self):
        x_data = np.linspace(0, 30., 100)
        e_data = []
        y_data = []
        for xx in x_data:
            y_data.append(np.sin(2.3 * xx + 0.1) * np.exp(-xx / 2.19703))
            e_data.append(np.cos(0.2 * xx))
        for xx in x_data:
            y_data.append(np.sin(4.3 * xx + 0.2) * np.exp(-xx / 2.19703))
            e_data.append(np.cos(0.2 * xx))

        inputData = CreateWorkspace(
            DataX=x_data,
            DataY=y_data,
            DataE=e_data,
            NSpec=2,
            UnitX='Time')
        return inputData

    def genDataWithDeadDetectors(self):
        inputData = CreateSimulationWorkspace("MUSR", "0,1,32")
        xData = (inputData.dataX(0)[1:] + inputData.dataX(0)[:-1]) / 2.
        for j in range(inputData.getNumberHistograms()):
            if j == 42 or j == 24:
                inputData.dataY(j)[:] = np.zeros(len(xData))
                inputData.dataE(j)[:] = np.cos(.3 * xData)
            else:
                phi = 2. * math.pi * \
                    (j + 1) / (inputData.getNumberHistograms() + 1)
                inputData.dataY(j)[:] = np.sin(2.3 * xData + phi)
                inputData.dataE(j)[:] = np.cos(.3 * xData)
        return inputData

    def cleanUp(self):
        DeleteWorkspace("InputData")
        DeleteWorkspace("freq")
        DeleteWorkspace("time")
        DeleteWorkspace("phase")

    def test_executes(self):
        inputData = self.genData()
        MuonMaxent(
            InputWorkspace=inputData,
            Npts=32768,
            FitDeaDTime=False,
            FixPhases=True,
            OuterIterations=1,
            InnerIterations=1,
            OutputWorkspace='freq',
            ReconstructedSpectra='time',
            OutputPhaseTable="phase")
        freq = AnalysisDataService.retrieve("freq")
        time = AnalysisDataService.retrieve("time")
        phase = AnalysisDataService.retrieve("phase")
        self.assertEqual(freq.getNumberHistograms(), 1)
        self.assertEqual(time.getNumberHistograms(), 1)
        self.assertEqual(phase.rowCount(), 1)
        self.cleanUp()

    def test_deadDetectors(self):
        inputData = self.genDataWithDeadDetectors()
        # check input data has 2 dead detectors
        for k in range(inputData.getNumberHistograms()):
            if k == 24 or k == 42:
                self.assertEqual(np.count_nonzero(inputData.readY(k)), 0)
            else:
                self.assertNotEqual(np.count_nonzero(inputData.readY(k)), 0)
        MuonMaxent(
            InputWorkspace=inputData,
            Npts=32768,
            FitDeaDTime=False,
            FixPhases=False,
            OuterIterations=5,
            InnerIterations=5,
            OutputWorkspace='freq',
            ReconstructedSpectra='time',
            OutputPhaseTable="phase",
            PhaseConvergenceTable="con")
        freq = AnalysisDataService.retrieve("freq")
        time = AnalysisDataService.retrieve("time")
        phase = AnalysisDataService.retrieve("phase")
        con = AnalysisDataService.retrieve("con")
        self.assertEqual(freq.getNumberHistograms(), 1)
        for j in range(time.getNumberHistograms()):
            if j == 42 or j == 24:
                self.assertEqual(np.count_nonzero(time.readY(j)), 0)
                self.assertEqual(np.count_nonzero(con.readY(j)), 0)
                self.assertEqual(phase.cell(j, 2), 0.0)
                self.assertEqual(phase.cell(j, 1), 999.)
            else:
                self.assertNotEqual(np.count_nonzero(time.readY(j)), 0)
                self.assertNotEqual(np.count_nonzero(con.readY(j)), 0)
                self.assertNotEqual(phase.cell(j, 1), 999.)

        self.assertEqual(phase.rowCount(), 64)
        self.cleanUp()

    def test_exitOnNAN(self):
        inputData = self.genData()
        try:
            MuonMaxent(
                InputWorkspace=inputData,
                Npts=32768,
                FitDeaDTime=False,
                FixPhases=True,
                OuterIterations=10,
                InnerIterations=10,
                OutputWorkspace='freq',
                ReconstructedSpectra='time',
                OutputPhaseTable="phase")
        except RuntimeError:
            pass
        else:
            self.fail(
                "should throw an error as it will get stuck in a loop due to NAN values")
        finally:
            DeleteWorkspace("InputData")

    def test_multipleSpec(self):
        inputData = self.genData2()
        MuonMaxent(
            InputWorkspace=inputData,
            Npts=32768,
            FitDeaDTime=False,
            FixPhases=True,
            OuterIterations=1,
            InnerIterations=1,
            OutputWorkspace='freq',
            ReconstructedSpectra='time',
            OutputPhaseTable="phase")
        freq = AnalysisDataService.retrieve("freq")
        time = AnalysisDataService.retrieve("time")
        phase = AnalysisDataService.retrieve("phase")
        self.assertEqual(freq.getNumberHistograms(), 1)
        self.assertEqual(time.getNumberHistograms(), 2)
        self.assertEqual(phase.rowCount(), 2)
        self.cleanUp()

    def test_badRange(self):
        inputData = self.genData2()
        try:
            MuonMaxent(
                InputWorkspace=inputData,
                FirstGoodData = 10.0,
                LastGoodData = 1.0,
                Npts=32768,
                FitDeaDTime=False,
                FixPhases=True,
                OuterIterations=1,
                InnerIterations=1,
                OutputWorkspace='freq',
                ReconstructedSpectra='time',
                OutputPhaseTable="phase")
        except:
           pass
        else:
           self.fail("should have failed as start > end")
        finally:
            DeleteWorkspace("InputData")

    def test_badStart(self):
        inputData = self.genData2()
        try:
            MuonMaxent(
                InputWorkspace=inputData,
                FirstGoodData = -10.0,
                Npts=32768,
                FitDeaDTime=False,
                FixPhases=True,
                OuterIterations=1,
                InnerIterations=1,
                OutputWorkspace='freq',
                ReconstructedSpectra='time',
                OutputPhaseTable="phase")
        except:
           pass
        else:
           self.fail("should have failed as start < 0.0")
        finally:
            DeleteWorkspace("InputData")

    def test_badInner(self):
        inputData = self.genData2()
        try:
            MuonMaxent(
                InputWorkspace=inputData,
                Npts=32768,
                FitDeaDTime=False,
                FixPhases=True,
                OuterIterations=1,
                InnerIterations=0,
                OutputWorkspace='freq',
                ReconstructedSpectra='time',
                OutputPhaseTable="phase")
        except:
           pass
        else:
           self.fail("should have failed as InnerIterations <= 0.0")
        finally:
            DeleteWorkspace("InputData")

    def test_badOuter(self):
        inputData = self.genData2()
        try:
            MuonMaxent(
                InputWorkspace=inputData,
                Npts=32768,
                FitDeaDTime=False,
                FixPhases=True,
                OuterIterations=0,
                InnerIterations=1,
                OutputWorkspace='freq',
                ReconstructedSpectra='time',
                OutputPhaseTable="phase")
        except:
           pass
        else:
           self.fail("should have failed as OuterIterations <= 0.0")
        finally:
            DeleteWorkspace("InputData")

    def test_badField(self):
        inputData = self.genData2()
        try:
            MuonMaxent(
                InputWorkspace=inputData,
                Npts=32768,
                MaxField = -10.,
                FitDeaDTime=False,
                FixPhases=True,
                OuterIterations=1,
                InnerIterations=1,
                OutputWorkspace='freq',
                ReconstructedSpectra='time',
                OutputPhaseTable="phase")
        except:
           pass
        else:
           self.fail("should have failed as MaxField <= 0.0")
        finally:
            DeleteWorkspace("InputData")

    def test_badFactor(self):
        inputData = self.genData2()
        try:
            MuonMaxent(
                InputWorkspace=inputData,
                Npts=32768,
                Factor = -10.,
                FitDeaDTime=False,
                FixPhases=True,
                OuterIterations=1,
                InnerIterations=1,
                OutputWorkspace='freq',
                ReconstructedSpectra='time',
                OutputPhaseTable="phase")
        except:
           pass
        else:
           self.fail("should have failed as Factor <= 0.0")
        finally:
            DeleteWorkspace("InputData")

    def test_badDefault(self):
        inputData = self.genData2()
        try:
            MuonMaxent(
                InputWorkspace=inputData,
                Npts=32768,
                DefaultLevel = -10.,
                FitDeaDTime=False,
                FixPhases=True,
                OuterIterations=1,
                InnerIterations=1,
                OutputWorkspace='freq',
                ReconstructedSpectra='time',
                OutputPhaseTable="phase")
        except:
           pass
        else:
           self.fail("should have failed as DefaultLevel <= 0.0")
        finally:
            DeleteWorkspace("InputData")

if __name__ == '__main__':
    unittest.main()
