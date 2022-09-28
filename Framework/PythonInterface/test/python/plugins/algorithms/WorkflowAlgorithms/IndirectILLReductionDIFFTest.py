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
    # Doppler run
    doppler_run = '276047'
    # Doppler, with alternating velocity profile and mirror sense
    doppler_alternating_runs = ['313719', '313720']

    @classmethod
    def setUpClass(cls):
        # cache the def instrument and data search dirs
        cls.facility = config['default.facility']
        cls.instrument = config['default.instrument']
        cls.data_dir = config['datasearch.directories']

        # set instrument and append datasearch directory
        config['default.facility'] = 'ILL'
        config['default.instrument'] = 'IN16B'
        config.appendDataSearchSubDir('ILL/IN16B/')

    @classmethod
    def tearDownClass(cls):
        # set cached facility and datasearch directory
        config['default.facility'] = cls.facility
        config['default.instrument'] = cls.instrument
        config['datasearch.directories'] = cls.data_dir

    def test_doppler(self):
        args = {'SampleRuns': self.doppler_run,
                'OutputWorkspace': 'out1'}
        alg_test = run_algorithm("IndirectILLReductionDIFF", **args)
        self.assertTrue(alg_test.isExecuted(), "IndirectILLReductionDIFF not executed")
        self.check_workspace_dims(mtd['out1'], 1896, 1)

    def test_doppler_alternating(self):
        args = {'SampleRuns': ','.join(self.doppler_alternating_runs),
                'OutputWorkspace': 'out2'}
        alg_test = run_algorithm("IndirectILLReductionDIFF", **args)
        self.assertTrue(alg_test.isExecuted(), "IndirectILLReductionDIFF not executed")
        self.check_workspace_dims(mtd['out2'], 1896, 2)

    def test_doppler_alternating_sum(self):
        # same as above, but with summing option, also masks a tube
        # note, this is not summing at raw level (due to different dimensions), but after reduction
        # hence input is still with a ,
        args = {'SampleRuns': ','.join(self.doppler_alternating_runs),
                'Sum': True,
                'ComponentsToMask': 'tube_8',
                'OutputWorkspace': 'out3'}
        alg_test = run_algorithm("IndirectILLReductionDIFF", **args)
        self.assertTrue(alg_test.isExecuted(), "IndirectILLReductionDIFF not executed")
        self.check_workspace_dims(mtd['out3'], 1659, 1)

    def check_workspace_dims(self, ws, bsize, nhist):
        name = ws.name()
        self.assertTrue(isinstance(ws, MatrixWorkspace), "{0} should be a matrix workspace".format(name))
        self.assertEqual(ws.getNumberHistograms(), nhist, "{0} should contain {1} spectra".format(name, nhist))
        self.assertEqual(ws.blocksize(), bsize, "{0} should contain {1} bins".format(name, bsize))
        self.assertTrue(ws.getSampleDetails(), "{0} should have sample logs".format(name))
        self.assertTrue(ws.getHistory().lastAlgorithm(), "{0} should have history".format(name))
        self.assertEqual(ws.getAxis(0).getUnit().unitID(), "MomentumTransfer")


if __name__ == '__main__':
    unittest.main()
