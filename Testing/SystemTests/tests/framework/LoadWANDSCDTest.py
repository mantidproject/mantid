# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import systemtesting
import numpy as np
from mantid.simpleapi import AddSampleLog, SetGoniometer, LoadWANDSCD


class LoadWANDSCDTest(systemtesting.MantidSystemTest):
    def runTest(self):
        # create van data
        van = LoadWANDSCD("HB2C_7000.nxs.h5")
        van.setSignalArray(np.full_like(van.getSignalArray(), 25))
        van.setErrorSquaredArray(np.full_like(van.getSignalArray(), 25))
        AddSampleLog(van, LogName="duration", LogText="42", LogType="Number Series", NumberType="Double")
        AddSampleLog(van, LogName="monitor_count", LogText="420", LogType="Number Series", NumberType="Double")
        # NOTE: all reference values are copied from terminal after confirm the code is correct
        LoadWANDTest_ws = LoadWANDSCD(Filename="HB2C_7000.nxs.h5,HB2C_7001.nxs.h5", VanadiumWorkspace=van, NormalizedBy="Monitor")
        ref_val = 0.00012953253733973653
        self.assertAlmostEqual(LoadWANDTest_ws.getSignalArray().max(), ref_val, 5)
        self.assertAlmostEqual(LoadWANDTest_ws.getErrorSquaredArray().max(), 0.0, 5)
        #
        LoadWANDTest_ws = LoadWANDSCD(Filename="HB2C_7000.nxs.h5,HB2C_7001.nxs.h5", VanadiumWorkspace=van, NormalizedBy="Counts")
        ref_val = 0.0112
        self.assertAlmostEqual(LoadWANDTest_ws.getSignalArray().max(), ref_val, 5)
        ref_err = 2.29376e-05
        self.assertAlmostEqual(LoadWANDTest_ws.getErrorSquaredArray().max(), ref_err, 5)
        #
        LoadWANDTest_ws = LoadWANDSCD(Filename="HB2C_7000.nxs.h5,HB2C_7001.nxs.h5", VanadiumWorkspace=van, NormalizedBy="Time")
        ref_val = 0.29363296439511044
        self.assertAlmostEqual(LoadWANDTest_ws.getSignalArray().max(), ref_val, 5)
        ref_err = 0.0157660
        self.assertAlmostEqual(LoadWANDTest_ws.getErrorSquaredArray().max(), ref_err, 5)
        LoadWANDTest_ws = LoadWANDSCD(Filename="HB2C_7000.nxs.h5,HB2C_7001.nxs.h5", ApplyGoniometerTilt=True)
        LoadWANDGonTest_ws = LoadWANDSCD(Filename="HB2C_7000.nxs.h5,HB2C_7001.nxs.h5", ApplyGoniometerTilt=False)
        SetGoniometer(
            LoadWANDGonTest_ws, Axis0="HB2C:Mot:s1,0,1,0,1", Axis1="HB2C:Mot:sgl,1,0,0,-1", Axis2="HB2C:Mot:sgu,0,0,1,-1", Average=False
        )
        UB = LoadWANDTest_ws.getExperimentInfo(0).sample().getOrientedLattice().getUB().copy()
        UBGon = LoadWANDGonTest_ws.getExperimentInfo(0).sample().getOrientedLattice().getUB().copy()
        # NOTE: the outer to innermost is s1, sgl, sgu
        np.testing.assert_allclose(
            LoadWANDTest_ws.getExperimentInfo(0).run().getGoniometer(0).getR() @ UB,
            LoadWANDGonTest_ws.getExperimentInfo(0).run().getGoniometer(0).getR() @ UBGon,
            3,
        )
        np.testing.assert_allclose(
            LoadWANDTest_ws.getExperimentInfo(0).run().getGoniometer(1).getR() @ UB,
            LoadWANDGonTest_ws.getExperimentInfo(0).run().getGoniometer(1).getR() @ UBGon,
            3,
        )

        LoadWANDTest_ws.delete()
        LoadWANDGonTest_ws.delete()
        van.delete()
