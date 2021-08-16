# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

# package imports
from mantid.api import AlgorithmManager
from mantid.simpleapi import LoadNexusProcessed, NOMADMedianDetectorTest

# standard imports
import os
import tempfile
import unittest


class NOMADMedianDetectorTestTest(unittest.TestCase):

    def test_initalize(self):
        alg = AlgorithmManager.create('NOMADMedianDetectorTest')
        alg.initialize()
        self.assertTrue(alg.isInitialized())

    def test_exec(self):
        _, file_mask = tempfile.mkstemp(suffix='.xml')
        LoadNexusProcessed(Filename='NOM_144974_SingleBin.nxs', OutputWorkspace='NOM_144974')
        NOMADMedianDetectorTest(InputWorkspace='NOM_144974',
                                ConfigurationFile='NOMAD_mask_gen_config.yml',
                                OutputMaskXML=file_mask)
        with open(file_mask) as f:
            contents = f.read()
        for segment in ['0-3122', '48847-48900', '69884-71046', '96376-96772']:  # test a few
            assert segment in contents
        os.remove(file_mask)


if __name__ == '__main__':
    unittest.main()
