from __future__ import (absolute_import, division, print_function)

import os.path
import stresstesting
import tempfile
from mantid.simpleapi import PowderDiffILLReduction, PowderDiffILLCalibration, \
    SaveNexusProcessed, SaveFocusedXYE
from mantid import config, mtd


class ILLPowderDiffReductionTest(stresstesting.MantidStressTest):

    _calib_run = '967076.nxs'
    _sample_runs = '967087,967088'

    def __init__(self):
        super(ILLPowderDiffReductionTest, self).__init__()
        self.setUp()

    def setUp(self):
        config['default.facility'] = 'ILL'
        config['default.instrument'] = 'D20'
        config.appendDataSearchSubDir('ILL/D20/')

    def requiredFiles(self):
        return ['967087.nxs', '967088.nxs', '967076.nxs']

    def tearDown(self):
        mtd.clear()
        os.remove(self._get_tmp_file_name('calib.nxs'))
        os.remove(self._get_tmp_file_name('fullprof-0.dat'))
        os.remove(self._get_tmp_file_name('fullprof-1.dat'))

    def runTest(self):
        self.test_calib_default_options()
        self.test_full_scenario()

    def test_calib_default_options(self):
        PowderDiffILLCalibration(CalibrationRun=self._calib_run,
                                 OutputWorkspace='calib',OutputResponseWorkspace='response')
        self.assertTrue(mtd['calib'])
        self.assertTrue(mtd['response'])
        self._check_calibration_output(mtd['calib'])
        self._check_combined_reponse_output(mtd['response'])

    def test_full_scenario(self):
        tmp_calib_file = self._get_tmp_file_name('calib.nxs')
        SaveNexusProcessed(InputWorkspace='calib',Filename=tmp_calib_file)
        self.assertTrue(os.path.exists(tmp_calib_file))
        PowderDiffILLReduction(Run=self._sample_runs,CalibrationFile=tmp_calib_file,
                               OutputWorkspace='calibrated')
        self._check_calibrated_sample(mtd['calibrated'])
        tmp_fullprof_export = self._get_tmp_file_name('fullprof.dat')
        SaveFocusedXYE(InputWorkspace='calibrated',Filename=tmp_fullprof_export)
        self.assertTrue(os.path.exists(self._get_tmp_file_name('fullprof-0.dat')))
        self.assertTrue(os.path.exists(self._get_tmp_file_name('fullprof-1.dat')))

    def _get_tmp_file_name(self, name):
        return os.path.join(tempfile.gettempdir(),tempfile.gettempprefix() + '_' + name)

    def _check_calibration_output(self, calib):
        self.assertEquals(calib.blocksize(),1)
        self.assertEquals(calib.getNumberHistograms(),3072)

    def _check_combined_reponse_output(self, response):
        self.assertEquals(response.blocksize(),3642)
        self.assertEquals(response.getNumberHistograms(),1)
        self.assertEquals(response.getAxis(0).getUnit().unitID(),'Degrees')
        theta_values = response.readX(0)
        self.assertAlmostEqual(theta_values[0], -34.2243, 4)
        self.assertAlmostEqual(theta_values[1], -34.1746, 4)
        self.assertAlmostEqual(theta_values[-2], 147.7254, 4)
        self.assertAlmostEqual(theta_values[-1], 147.7754, 4)

    def _check_calibrated_sample(self, calibrated):
        self.assertEquals(calibrated.blocksize(),3008)
        self.assertEquals(calibrated.getNumberHistograms(),2)
