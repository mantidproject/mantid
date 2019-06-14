# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#pylint: disable=invalid-name,too-many-public-methods,too-many-arguments
from __future__ import (absolute_import, division, print_function)

import mantid
from mantid.api import AnalysisDataService
from mantid.simpleapi import CreateWorkspace, SaveNexusPD
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
        dataDir = mantid.config.getString('defaultsave.directory')
        return os.path.join(dataDir, wkspname+'.h5')

    def cleanup(self, wkspname, filename):
        if os.path.exists(filename):
            os.remove(filename)
        if mantid.mtd.doesExist(wkspname):
            mantid.api.AnalysisDataService.remove(wkspname)

    def testSaveOneSpectrumNoInstrument(self):
        """ Test to Save one spectrum without and instrument
        """
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
        """ Test to Save multiple spectra with instrument
        """
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

    def checkDataFields(self, nxitem, withInstrument):
        keys = nxitem.keys()
        for fieldname in ['data', 'errors', 'tof']:
            self.assertTrue(fieldname in keys)
        if withInstrument:
            for fieldname in ['Q', 'dspacing']:
                self.assertTrue(fieldname in keys)

    def checkDetectorFields(self, nxitem, withInstrument):
        if not withInstrument:
            return
        keys = nxitem.keys()
        for fieldname in ['distance',
                          'azimuthal_angle', 'polar_angle']:
            self.assertTrue(fieldname in keys)

    def check(self, filename, withInstrument):
        with h5py.File(filename, 'r') as handle:
            nxentry = handle[sorted(handle.keys())[0]]
            nxinstrument = nxentry['instrument']

            nxmoderator = nxinstrument['moderator']
            if withInstrument:
                self.assertLess(nxmoderator['distance'][0], 0.)

            for name in nxinstrument.keys():
                if name == 'moderator':
                    continue
                nxdetector = nxinstrument[name]
                self.checkDataFields(nxdetector, withInstrument)
                self.checkDetectorFields(nxdetector, withInstrument)

            for name in nxentry.keys():
                if name == 'instrument':
                    continue
                if name == 'proton_charge':
                    continue
                nxdata = nxentry[name]
                self.checkDataFields(nxdata, withInstrument)

    def _createOneSpectrum(self, wkspname):
        x = np.arange(300, 16667, 15.)
        y = np.random.random(len(x)-1)  # histogram
        e = np.sqrt(y)

        CreateWorkspace(OutputWorkspace=wkspname,
                        DataX=x, DataY=y, DataE=e, NSpec=1,
                        UnitX='TOF',
                        YUnitlabel="stuff")

    def _createMultiSpectra(self, wkspname):
        wksp = WCH.create2DWorkspaceWithFullInstrument(30, 5, False, False)
        AnalysisDataService.add(wkspname, wksp)

if __name__ == '__main__':
    unittest.main()
