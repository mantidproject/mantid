# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import os
import shutil
import systemtesting

from mantid import config
from mantid.api import AnalysisDataService as ADS
from mantid.kernel import UnitParams
from Engineering.EnginX import EnginX
from Engineering.EnggUtils import GROUP

CWDIR = os.path.join(config['datasearch.directories'].split(';')[0], "ENGINX")
FULL_CALIB = os.path.join(CWDIR, "ENGINX_whole_inst_calib.nxs")

BANK_DIFF_CONSTS = {'North': {UnitParams.difc: 18422, UnitParams.difa: -6.6, UnitParams.tzero: -15},
                    'South': {UnitParams.difc: 18427, UnitParams.difa: -7.9, UnitParams.tzero: -19.4}}


class FocusBothBanks(systemtesting.MantidSystemTest):

    def runTest(self):
        enginx = EnginX(vanadium_run="ENGINX236516", focus_runs=["ENGINX299080"], save_dir=CWDIR,
                        full_inst_calib_path=FULL_CALIB, ceria_run="ENGINX193749", group=GROUP.BOTH)
        enginx.main(plot_cal=False, plot_foc=False)
        # store workspaces for validation
        self._ws_foc = ADS.retrieve("299080_engggui_focusing_output_ws_bank")

    def validate(self):
        # bank 1
        diff_consts = self._ws_foc.spectrumInfo().diffractometerConstants(0)
        self.assertAlmostEqual(diff_consts[UnitParams.difc], BANK_DIFF_CONSTS['North'][UnitParams.difc], delta=5)
        self.assertAlmostEqual(diff_consts[UnitParams.difa], BANK_DIFF_CONSTS['North'][UnitParams.difa], delta=1)
        self.assertAlmostEqual(diff_consts[UnitParams.tzero], BANK_DIFF_CONSTS['North'][UnitParams.tzero], delta=2)
        # bank 2
        diff_consts = self._ws_foc.spectrumInfo().diffractometerConstants(1)
        self.assertAlmostEqual(diff_consts[UnitParams.difc], BANK_DIFF_CONSTS['South'][UnitParams.difc], delta=5)
        self.assertAlmostEqual(diff_consts[UnitParams.difa], BANK_DIFF_CONSTS['South'][UnitParams.difa], delta=1)
        self.assertAlmostEqual(diff_consts[UnitParams.tzero], BANK_DIFF_CONSTS['South'][UnitParams.tzero], delta=2)
        # compare foc data in TOF (conversion to d already tested by assertions on diff consts)
        self.tolerance = 1e-6
        self.disableChecking.extend(['Instrument'])  # don't check
        return self._ws_foc.name(), "299080_engggui_focusing_output_ws_bank.nxs"

    def cleanup(self):
        ADS.clear()
        _try_delete_cal_and_focus_dirs(CWDIR)


class FocusCroppedSpectraSameDiffConstsAsBank(systemtesting.MantidSystemTest):

    def runTest(self):
        enginx = EnginX(vanadium_run="ENGINX236516", focus_runs=["ENGINX299080"], save_dir=CWDIR,
                        full_inst_calib_path=FULL_CALIB, ceria_run="ENGINX193749",
                        group=GROUP.CROPPED, spectrum_num="1-1200")  # North
        enginx.main(plot_cal=False, plot_foc=False)
        # store workspaces for validation
        self._ws_foc = ADS.retrieve("299080_engggui_focusing_output_ws_Cropped")

    def validate(self):
        # only assert diff constants (both banks tests normalisation etc.)
        diff_consts = self._ws_foc.spectrumInfo().diffractometerConstants(0)
        self.assertAlmostEqual(diff_consts[UnitParams.difc], BANK_DIFF_CONSTS['North'][UnitParams.difc], delta=5)
        self.assertAlmostEqual(diff_consts[UnitParams.difa], BANK_DIFF_CONSTS['North'][UnitParams.difa], delta=1)
        self.assertAlmostEqual(diff_consts[UnitParams.tzero], BANK_DIFF_CONSTS['North'][UnitParams.tzero], delta=2)

    def cleanup(self):
        ADS.clear()
        _try_delete_cal_and_focus_dirs(CWDIR)


class FocusTexture(systemtesting.MantidSystemTest):

    def runTest(self):
        enginx = EnginX(vanadium_run="ENGINX236516", focus_runs=["ENGINX299080"], save_dir=CWDIR,
                        full_inst_calib_path=FULL_CALIB, ceria_run="ENGINX193749", group=GROUP.TEXTURE20)
        enginx.main(plot_cal=False, plot_foc=False)
        # store workspaces for validation
        self._ws_foc = ADS.retrieve("299080_engggui_focusing_output_ws_Texture20")

    def validate(self):
        # assert correct number spectra
        self.assertEqual(self._ws_foc.getNumberHistograms(), 20)
        # don't assert diff constants of one group
        diff_consts = self._ws_foc.spectrumInfo().diffractometerConstants(10)
        self.assertAlmostEqual(diff_consts[UnitParams.difc], 17250, delta=5)
        self.assertAlmostEqual(diff_consts[UnitParams.difa], -8.5, delta=1)
        self.assertAlmostEqual(diff_consts[UnitParams.tzero], -20.3, delta=2)
        # compare TOF workspaces
        self.tolerance = 1e-6
        self.disableChecking.extend(['Instrument'])  # don't check
        return self._ws_foc.name(), "299080_engggui_focusing_output_ws_Texture.nxs"


class FocusTexture30(systemtesting.MantidSystemTest):

    def runTest(self):
        enginx = EnginX(vanadium_run="ENGINX236516", focus_runs=["ENGINX299080"], save_dir=CWDIR,
                        full_inst_calib_path=FULL_CALIB, ceria_run="ENGINX193749", group=GROUP.TEXTURE30)
        enginx.main(plot_cal=False, plot_foc=False)
        # store workspaces for validation
        self._ws_foc = ADS.retrieve("299080_engggui_focusing_output_ws_Texture30")

    def validate(self):
        # assert correct number spectra
        self.assertEqual(self._ws_foc.getNumberHistograms(), 30)
        # don't assert diff constants of one group
        diff_consts = self._ws_foc.spectrumInfo().diffractometerConstants(23)
        self.assertAlmostEqual(diff_consts[UnitParams.difc], 19898, delta=5)
        self.assertAlmostEqual(diff_consts[UnitParams.difa], -8.9, delta=1)
        self.assertAlmostEqual(diff_consts[UnitParams.tzero], -22.2, delta=2)
        # compare TOF workspaces
        self.tolerance = 1e-6
        self.disableChecking.extend(['Instrument'])  # don't check
        return self._ws_foc.name(), "299080_engggui_focusing_output_ws_Texture30.nxs"


def _try_delete_cal_and_focus_dirs(parent_dir):
    for folder in ['Calibration', 'Focus']:
        rm_dir = os.path.join(parent_dir, folder)
        try:
            if os.path.isdir(rm_dir):
                shutil.rmtree(rm_dir) # don't use os.remove as we could be passed a non-empty dir
            else:
                os.remove(rm_dir)
        except OSError:
            print("Could not delete output file at: ", rm_dir)
