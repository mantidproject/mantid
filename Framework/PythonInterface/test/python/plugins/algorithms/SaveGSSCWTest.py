# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#pylint: disable=invalid-name,too-many-public-methods,too-many-arguments
import mantid
from mantid.api import AnalysisDataService
from mantid.simpleapi import CreateWorkspace, SaveGSSCW, LoadAscii
import numpy as np
import os
import unittest

runTests = True


class SaveGSSCWTest(unittest.TestCase):
    def saveFilePath(self, wkspname):
        dataDir = mantid.config.getString('defaultsave.directory')
        return os.path.join(dataDir, wkspname+'.gss')

    def cleanup(self, filename, wkspname):
        if os.path.exists(filename):
            os.remove(filename)
        if mantid.mtd.doesExist(wkspname):
            mantid.api.AnalysisDataService.remove(wkspname)

    def testSaveGSS2Theta(self):
        """ Test to Save one spectrum without and instrument
        """
        if not runTests:
            return

        ws_name = "SaveGSSCW_2theta"
        gsas_file_name = self.saveFilePath(ws_name)
        self._create_2theta_workspace(ws_name)
        print(f'GSAS file name: {gsas_file_name}')
        ws = AnalysisDataService[ws_name]
        assert ws
        print(f'Workspace {ws_name} of type {type(ws)}')

        try:
            SaveGSSCW(InputWorkspace=ws_name, OutputFilename=gsas_file_name)

            self.check(gsas_file_name)
        finally:
            self.cleanup(gsas_file_name, ws_name)

    def testSaveGSS_hb2a(self):
        """Test with real HB2B data including XYE and verified by GSAS

        Returns
        -------

        """
        # Test files were provided by Stuart
        source_xye_file = 'IPTS-2005_exp719_Scan52_MANTID.dat'
        ws_name = 'HB2AData'
        LoadAscii(OutputWorkspace=ws_name,
                  Filename=source_xye_file,
                  Unit="Degrees")

        spice_gss_file = 'IPTS-2005_exp719_Scan52_SPICE.gss'

        try:
            xye_file = open(source_xye_file, 'r')
            xye_file.close()
        except IOError:
            raise IOError(f'Unable to locate XYE file {source_xye_file}')

    def checkDataFields(self, nxitem, withInstrument):
        keys = nxitem.keys()
        for fieldname in ['data', 'errors', 'tof']:
            self.assertTrue(fieldname in keys)
        if withInstrument:
            for fieldname in ['Q', 'dspacing']:
                self.assertTrue(fieldname in keys)

    @staticmethod
    def check(filename):
        """Check output GSAS file

        Parameters
        ----------
        filename: str
            GSAS filename

        Returns
        -------

        """
        # File exists?
        assert os.path.exists(filename), f'Generated GSAS file {filename} cannot be found'

        # Contents

    @staticmethod
    def _create_2theta_workspace(ws_name):
        """ Create 2theta workspace
        """
        # From 15 degree to 105 degree with 1 degree space
        x = np.arange(15., 105., 1.)
        y = np.zeros((len(x)-1,), dtype='float') + 5 * np.exp(-(x[1:] - 50.)**2 / 4.)  # histogram
        e = np.sqrt(y)

        CreateWorkspace(OutputWorkspace=ws_name,
                        DataX=x, DataY=y, DataE=e, NSpec=1,
                        UnitX='Degrees', YUnitlabel="Counts",
                        WorkspaceTitle='Test SaveGSSCW')


if __name__ == '__main__':
    unittest.main()
