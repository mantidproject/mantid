# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name,too-many-public-methods,too-many-arguments
import mantid
from mantid.api import AnalysisDataService
from mantid.simpleapi import CreateWorkspace, MoveInstrumentComponent, SaveNexusPD
import numpy as np
import os
from testhelpers import WorkspaceCreationHelper as WCH
import unittest

runTests = True
try:
    import h5py
except ImportError:
    runTests = False


class SaveNexusPDTest(unittest.TestCase):
    def saveFilePath(self, wkspname):
        dataDir = mantid.config.getString("defaultsave.directory")
        return os.path.join(dataDir, wkspname + ".h5")

    def cleanup(self, filename, wkspname):
        if os.path.exists(filename):
            os.remove(filename)
        if mantid.mtd.doesExist(wkspname):
            mantid.api.AnalysisDataService.remove(wkspname)

    def testSaveOneSpectrumNoInstrument(self):
        """Test to Save one spectrum without and instrument"""
        if not runTests:
            return

        wkspname = "SaveNexusPDTest_onespectrumnoinstr"
        filename = self.saveFilePath(wkspname)
        self._createOneSpectrum(wkspname)

        try:
            SaveNexusPD(InputWorkspace=wkspname, OutputFilename=filename)

            self.check(filename, False)
        finally:
            self.cleanup(filename, wkspname)

    def testSaveMultiSpectra(self):
        """Test to Save multiple spectra with instrument"""
        if not runTests:
            return

        wkspname = "SaveNexusPDTest_multispectra"
        filename = self.saveFilePath(wkspname)
        self._createMultiSpectra(wkspname)

        try:
            SaveNexusPD(InputWorkspace=wkspname, OutputFilename=filename)

            self.check(filename, True)
        finally:
            self.cleanup(filename, wkspname)

    def testSaveWithOffsetSample(self):
        """
        Regression test for possible bug, if sample is not lcated at origin.
        - _determineSourceSample stored (samplePos - sourcePos), the beam direction vector,
          in self._sourcePos instead of the absolute source position. _createInstrument then
          computed L1 = self._sourcePos.distance(self._samplePos), which algebraically reduces
          to |sourcePos| (distance from the coordinate origin to the source) rather than the
          true source-to-sample distance. This was invisible whenever the sample happened to
          sit at the origin, which is why it went unnoticed for years.
        """
        if not runTests:
            return

        wkspname = "SaveNexusPDTest_offsetsample"
        filename = self.saveFilePath(wkspname)
        self._createMultiSpectra(wkspname)

        # move the sample away from the origin so a wrong L1 formula would show up numerically
        sample_offset = np.array([0.5, 0.3, 1.2])
        MoveInstrumentComponent(
            Workspace=wkspname,
            ComponentName="samplePos",
            RelativePosition=False,
            X=sample_offset[0],
            Y=sample_offset[1],
            Z=sample_offset[2],
        )

        component_info = mantid.mtd[wkspname].componentInfo()
        expected_l1 = component_info.sourcePosition().distance(component_info.samplePosition())
        # the pre-fix formula computed this (wrong) value instead
        buggy_l1 = np.linalg.norm(np.array(list(component_info.sourcePosition())))
        self.assertGreater(abs(expected_l1 - buggy_l1), 1e-6, "Sample offset displacement not considered in calculation of L1")
        try:
            SaveNexusPD(InputWorkspace=wkspname, OutputFilename=filename)
            with h5py.File(filename, "r") as handle:
                nxentry = handle[min(handle.keys())]
                saved_l1 = abs(nxentry["instrument"]["moderator"]["distance"][0])
            self.assertAlmostEqual(saved_l1, expected_l1, places=5)
        finally:
            self.cleanup(filename, wkspname)

    def testSaveWithMonitors(self):
        """
        Regression test for possible bug if the workspace contains monitors
        - _writeDetectorPos called spectrum_info.twoTheta()/l2() unconditionally; those throw
          (twoTheta) or silently use a different formula (l2) for monitor spectra, whereas the
          legacy IDetector.getTwoTheta()/getDistance() computed the raw geometry unconditionally.
        """
        if not runTests:
            return

        wkspname = "SaveNexusPDTest_monitors"
        filename = self.saveFilePath(wkspname)
        # create a workspace with monitors (the True argument)
        wksp = WCH.create2DWorkspaceWithFullInstrument(5, 5, True, False)
        AnalysisDataService.add(wkspname, wksp)

        try:
            # must not raise: a monitor spectrum in the loop previously crashed twoTheta()
            SaveNexusPD(InputWorkspace=wkspname, OutputFilename=filename)
        except Exception as e:
            self.fail(f"SaveNexusPD raised an exception with monitors in the workspace: {e}")
        finally:
            self.cleanup(filename, wkspname)

    def checkDataFields(self, nxitem, withInstrument):
        keys = nxitem.keys()
        for fieldname in ["data", "errors", "tof"]:
            self.assertTrue(fieldname in keys)
        if withInstrument:
            for fieldname in ["Q", "dspacing"]:
                self.assertTrue(fieldname in keys)

    def checkDetectorFields(self, nxitem, withInstrument):
        if not withInstrument:
            return
        keys = nxitem.keys()
        for fieldname in ["distance", "azimuthal_angle", "polar_angle"]:
            self.assertTrue(fieldname in keys)

    def check(self, filename, withInstrument):
        with h5py.File(filename, "r") as handle:
            nxentry = handle[sorted(handle.keys())[0]]
            nxinstrument = nxentry["instrument"]

            nxmoderator = nxinstrument["moderator"]
            if withInstrument:
                self.assertLess(nxmoderator["distance"][0], 0.0)

            for name in nxinstrument.keys():
                if name == "moderator":
                    continue
                nxdetector = nxinstrument[name]
                self.checkDataFields(nxdetector, withInstrument)
                self.checkDetectorFields(nxdetector, withInstrument)

            for name in nxentry.keys():
                if name == "instrument":
                    continue
                if name == "proton_charge":
                    continue
                nxdata = nxentry[name]
                self.checkDataFields(nxdata, withInstrument)

    def _createOneSpectrum(self, wkspname):
        x = np.arange(300, 16667, 15.0)
        y = np.random.random(len(x) - 1)  # histogram
        e = np.sqrt(y)

        CreateWorkspace(OutputWorkspace=wkspname, DataX=x, DataY=y, DataE=e, NSpec=1, UnitX="TOF", YUnitlabel="stuff")

    def _createMultiSpectra(self, wkspname):
        wksp = WCH.create2DWorkspaceWithFullInstrument(30, 5, False, False)
        AnalysisDataService.add(wkspname, wksp)


if __name__ == "__main__":
    unittest.main()
