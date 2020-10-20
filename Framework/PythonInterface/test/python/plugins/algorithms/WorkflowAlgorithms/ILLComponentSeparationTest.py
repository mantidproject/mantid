# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from mantid.api import WorkspaceGroup
from mantid.simpleapi import config, mtd, ILLComponentSeparation

class ILLComponentSeparationTest(unittest.TestCase):

    _facility = None
    _instrument = None

    @classmethod
    def setUpClass(cls):
        config.appendDataSearchSubDir('ILL/D7/')

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
        mtd.clear()

    def test_uniaxial_separation(self):
        ILLComponentSeparation(Run='vanadium_uniaxial', OutputWorkspace='uniaxial')
        self._check_output('uniaxial', 1, 132)

    def test_xyz_separation(self):
        ILLComponentSeparation(Run='vanadium_xyz', OutputWorkspace='xyz')
        self._check_output('xyz', 1, 132)

    def test_10p_separation(self):
        ILLComponentSeparation(Run='vanadium_10p', OutputWorkspace='10p')
        self._check_output('10p', 1, 132)


    def _check_output(self, ws, blocksize, spectra):
        self.assertTrue(mtd['ws'])
        self.assertTrue(isinstance(mtd['ws'], WorkspaceGroup))
        for entry in mtd['ws']:
            self.assertTrue(isinstance(entry, MatrixWorkspace))
            self.assertTrue(entry.isHistogramData())
            self.assertTrue(not entry.isDistribution())
            name = entry.name()[
            name = name[name.rfind("_"):]            
            self.assertTrue(name in ['Coherent', 'Incoherent', 'Magnetic'])
            self.assertEqual(entry.blocksize(), blocksize)
            self.assertEqual(entry.getNumberHistograms(), spectra)
            self.assertTrue(entry.getHistory())

if __name__ == '__main__':
    unittest.main()
