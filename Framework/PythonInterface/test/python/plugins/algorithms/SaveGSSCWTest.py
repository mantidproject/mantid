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

Expected_GSS = """                                                                                
                   BANK 1 2438 488 CONST 1000.000    5.000 0 0 ESD                                 
                   287.51   16.96  281.38   13.52  279.80   11.83  282.77   11.89  279.73   13.43
                   270.69   16.45  270.27   13.23  271.90   11.66  275.57   11.74  286.05   13.66
                   303.35   17.42  295.64   13.86  292.17   12.09  292.92   12.10  288.03   13.63
                   277.50   16.66  278.01   13.42  274.65   11.72  267.41   11.56  266.29   13.15
                   271.27   16.47  273.06   13.29  272.29   11.67  268.96   11.60  260.08   12.93
                   245.63   15.67  250.87   12.73  258.06   11.36  267.18   11.56  272.39   13.29
                   273.68   16.54  269.92   13.23  266.37   11.54  263.04   11.47  259.18   12.94
                   254.80   15.96  258.56   12.93  261.36   11.43  263.21   11.47  262.35   13.02
                   258.78   16.09  254.71   12.86  257.64   11.35  267.57   11.57  265.69   13.07
                   251.99   15.87  235.43   12.40  230.35   10.73  236.73   10.88  242.60   12.55
                   247.96   15.75  238.47   12.46  237.80   10.90  246.67   10.96  262.79    9.69
                   255.80   10.58  247.97   10.13  244.45    9.64  243.10   10.88  255.51    9.56
                   257.28   10.63  255.45   10.28  255.02    9.85  257.07   11.19  254.40    9.55
                   256.29   10.61  254.03   10.26  252.45    9.80  257.38   11.20  259.48    9.64
                   255.86   10.59  248.93   10.16  242.40    9.59  239.86   10.81  252.22    9.50
                   242.14   10.29  241.52    9.98  244.76    9.64  238.51   10.78  244.85    9.36
                   251.14   10.51  247.02   10.12  245.47    9.67  256.67   11.19  244.10    9.36
                   240.22   10.27  242.41   10.01  245.44    9.66  246.65   10.96  256.36    9.58
                   251.33   10.49  252.20   10.20  252.50    9.78  242.63   10.87  245.65    9.38
                   253.65   10.56  253.98   10.26  250.80    9.76  251.09   11.07  243.15    9.34
                   242.25   10.31  243.95   10.04  240.75    9.55  230.83   10.60  237.20    9.21
                   238.40    8.91  241.68    8.46  245.13    7.83  242.16    7.78  240.49    7.75
                   242.77    8.47  234.46    8.35  224.29    7.49  235.14    7.67  234.93    7.66
                   236.81    8.36  234.26    8.34  230.92    7.60  237.71    7.71  230.89    7.60
                   236.36    8.36  241.12    9.73  240.78   10.97  246.59   10.77  232.34    8.96
                   236.26   10.40  245.12   11.07  240.23    9.74  235.58    8.86  236.04    8.87
                   236.26   10.40  245.82   11.09  240.63    9.75  241.24    8.97  252.87    9.18
                   240.37   10.47  257.21   11.34  252.94   10.00  241.20    8.97  234.21    8.84
                   235.92   10.39  229.43   10.71  233.05    9.57  238.17    8.91  240.04    8.95
                   236.77   10.40  247.15   11.12  242.52    9.79  238.47    8.92  241.73    8.98
                   232.09   10.29  233.01   10.79  236.99    9.66  237.26    8.89  234.89    8.85
                   242.99   10.55  231.89   10.77  230.57    9.53  229.63    8.75  223.14    8.62"""


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
