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
        self.assertAlmostEqual(diff_consts[UnitParams.difc], 18422, delta=1)
        self.assertAlmostEqual(diff_consts[UnitParams.difa], -6.3, delta=0.3)
        self.assertAlmostEqual(diff_consts[UnitParams.tzero], -15.0, delta=0.5)
        # bank 2
        diff_consts = self._ws_foc.spectrumInfo().diffractometerConstants(1)
        self.assertAlmostEqual(diff_consts[UnitParams.difc], 18423.5, delta=1)
        self.assertAlmostEqual(diff_consts[UnitParams.difa], -6.6, delta=0.3)
        self.assertAlmostEqual(diff_consts[UnitParams.tzero], -17.3, delta=0.5)
        # assert foc data in TOF (conversion to d already tested by assertions on diff consts)

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
        # bank 1
        diff_consts = self._ws_foc.spectrumInfo().diffractometerConstants(0)
        self.assertAlmostEqual(diff_consts[UnitParams.difc], 18422, delta=1)
        self.assertAlmostEqual(diff_consts[UnitParams.difa], -6.3, delta=0.3)
        self.assertAlmostEqual(diff_consts[UnitParams.tzero], -15.0, delta=0.5)

    def cleanup(self):
        ADS.clear()
        _try_delete_cal_and_focus_dirs(CWDIR)

# class FocusTexture(systemtesting.MantidSystemTest):
#     def validate(self):
#         self.tolerance = 1e-3
#         outputlist = ["ENGINX_299080_236516_texture_{i}" for i in range(1, 21)]
#         filelist = ["engg_texture_bank_{}.nxs".format(i) for i in range(1, 11)]
#         validation_list = [x for t in zip(*[outputlist, filelist]) for x in t]
#         return validation_list


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
