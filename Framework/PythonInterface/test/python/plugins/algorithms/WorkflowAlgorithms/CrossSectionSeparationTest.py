# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from mantid.api import WorkspaceGroup, MatrixWorkspace
from mantid.simpleapi import Load, config, mtd, CrossSectionSeparation

class CrossSectionSeparationTest(unittest.TestCase):

    _facility = None
    _instrument = None

    @classmethod
    def setUpClass(cls):
        config.appendDataSearchSubDir('ILL/D7/')
        Load('vanadium_uniaxial.nxs', OutputWorkspace='vanadium_uniaxial')
        Load('vanadium_xyz.nxs', OutputWorkspace='vanadium_xyz')
        Load('vanadium_10p.nxs', OutputWorkspace='vanadium_10p')

    def setUp(self):
        self._facility = config['default.facility']
        self._instrument = config['default.instrument']

        config['default.facility'] = 'ILL'
        config['default.instrument'] = 'D7'

    def tearDown(self):
        if self._facility:
            config['default.facility'] = self._facility
        if self._instrument:
            config['default.instrument'] = self._instrument

    @classmethod
    def tearDownClass(cls):
        mtd.clear()

    def test_uniaxial_separation(self):
        CrossSectionSeparation(InputWorkspace='vanadium_uniaxial', OutputWorkspace='uniaxial',
                               CrossSectionSeparationMethod='Uniaxial')
        self.assertTrue(mtd['uniaxial'].getNumberOfEntries() == 2)
        self._check_output('uniaxial', 1, 132)

    def test_xyz_separation(self):
        CrossSectionSeparation(InputWorkspace='vanadium_xyz', OutputWorkspace='xyz',
                               CrossSectionSeparationMethod='XYZ')
        self.assertTrue(mtd['xyz'].getNumberOfEntries() == 3)
        self._check_output('xyz', 1, 132)

    def test_10p_separation(self):
        CrossSectionSeparation(InputWorkspace='vanadium_10p', OutputWorkspace='10p',
                               CrossSectionSeparationMethod='10p', ThetaOffset=1.0)
        self.assertTrue(mtd['10p'].getNumberOfEntries() == 3)
        self._check_output('10p', 1, 132)

    def _check_output(self, ws, blocksize, spectra):
        self.assertTrue(mtd[ws])
        self.assertTrue(isinstance(mtd[ws], WorkspaceGroup))
        for entry in mtd[ws]:
            self.assertTrue(isinstance(entry, MatrixWorkspace))
            self.assertTrue(entry.isHistogramData())
            self.assertTrue(not entry.isDistribution())
            name = entry.name()
            name = name[name.rfind("_")+1:]
            self.assertTrue(name in ['Coherent', 'Incoherent', 'Magnetic'])
            self.assertEqual(entry.blocksize(), blocksize)
            self.assertEqual(entry.getNumberHistograms(), spectra)
            self.assertTrue(entry.getHistory())

if __name__ == '__main__':
    unittest.main()
