# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

import unittest
from mantid.simpleapi import mtd
from mantid import config
from testhelpers import run_algorithm
from mantid.api import MatrixWorkspace


class IndirectILLReductionDIFF(unittest.TestCase):
    # cache the def instrument and data search dirs
    facility = config['default.facility']
    instrument = config['default.instrument']
    data_dir = config['datasearch.directories']

    # Doppler
    doppler_run = '276047'

    def setUp(self):
        # set instrument and append datasearch directory
        config['default.facility'] = 'ILL'
        config['default.instrument'] = 'IN16B'
        config.appendDataSearchSubDir('ILL/IN16B/')

        self.tolerance = 1e-2

    def tearDown(self):
        # set cached facility and datasearch directory
        config['default.facility'] = self.facility
        config['default.instrument'] = self.instrument
        config['datasearch.directories'] = self.data_dir

    def test_doppler(self):
        args = {'SampleRuns': self.doppler_run,
                'OutputWorkspace': 'out'}
        alg_test = run_algorithm("IndirectILLReductionDIFF", **args)

        self.assertTrue(alg_test.isExecuted(), "IndirectILLReductionDIFF not executed")

        ws = mtd["out"]
        name = ws.name()

        self.assertTrue(isinstance(ws, MatrixWorkspace), "{0} should be a matrix workspace".format(name))
        self.assertEqual(ws.getNumberHistograms(), 1, "{0} should contain {1} spectra".format(name, 1))

        self.assertEqual(ws.blocksize(), 1896, "{0} should contain {1} bins".format(name, 1896))

        self.assertTrue(ws.getSampleDetails(), "{0} should have sample logs".format(name))

        self.assertTrue(ws.getHistory().lastAlgorithm(), "{0} should have history".format(name))


if __name__ == '__main__':
    unittest.main()
