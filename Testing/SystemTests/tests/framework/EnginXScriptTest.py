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
        self._diff_consts = ADS.retrieve("diffractometer_consts_table").toDict()

    def validate(self):
        print("########## DIFF_CONSTS", self._diff_consts )
        # assert diff const
        self.assertAlmostEqual(self._diff_consts['DIFC'][0], 18425.452, delta=1e-2)
        self.assertAlmostEqual(self._diff_consts['DIFC'][1], 18421.872, delta=1e-2)
        self.assertAlmostEqual(self._diff_consts['DIFA'][0], -7.650, delta=1e-2)
        self.assertAlmostEqual(self._diff_consts['DIFA'][1], -6.137, delta=1e-2)
        self.assertAlmostEqual(self._diff_consts['TZERO'][0], -17.501, delta=1e-2)
        self.assertAlmostEqual(self._diff_consts['TZERO'][1], -15.595, delta=1e-2)
        # assert foc data in TOF (conversion to d already tested by assertions on diff consts)
        return ("ENGINX_299080_236516_bank_1_TOF.nxs", "ref_1.nxs",
                "ENGINX_299080_236516_bank_2_TOF.nxs", "ref_1.nxs")

    def cleanup(self):
        ADS.clear()
        _try_delete(CWDIR)

# class FocusCroppedSpectraSameDiffConstsAsBank(systemtesting.MantidSystemTest):
#
# class FocusCustomCalSameDiffConstsAsBank(systemtesting.MantidSystemTest):
#
# class FocusTexture(systemtesting.MantidSystemTest):
#     def validate(self):
#         self.tolerance = 1e-3
#         outputlist = ["ENGINX_299080_236516_texture_{i}" for i in range(1, 21)]
#         filelist = ["engg_texture_bank_{}.nxs".format(i) for i in range(1, 11)]
#         validation_list = [x for t in zip(*[outputlist, filelist]) for x in t]
#         return validation_list


def _try_delete(path):
    try:
        # Use this instead of os.remove as we could be passed a non-empty dir
        if os.path.isdir(path):
            shutil.rmtree(path)
        else:
            os.remove(path)
    except OSError:
        print("Could not delete output file at: ", path)
