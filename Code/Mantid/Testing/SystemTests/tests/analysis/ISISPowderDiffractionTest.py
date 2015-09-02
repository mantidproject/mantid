# pylint: disable=no-init

from mantid.api import FileFinder
from mantid import config
import os.path
import stresstesting
import sys
F_DIR = FileFinder.getFullPath("PowderISIS")
sys.path.append(F_DIR)
import cry_ini
import cry_focus


# ==============================================================================
def _cleanup_files(dirname, filenames):
    """
       Attempts to remove each filename from
       the given directory
    """
    for filename in filenames:
        path = os.path.join(dirname, filename)
        try:
            os.remove(path)
        except OSError:
            print 'Did not delete generated file: ' + filename
# ==============================================================================

class ISISPowderDiffraction(stresstesting.MantidStressTest):

    def requiredFiles(self):
        return {"HRP39191.raw", "HRP39187.raw", "HRP43022.raw", "hrpd/test/GrpOff/hrpd_new_072_01.cal",
                "hrpd/test/GrpOff/hrpd_new_072_01_corr.cal", "hrpd/test/cycle_09_2/Calibration/van_s1_old-0.nxs",
                "hrpd/test/cycle_09_2/Calibration/van_s1_old-1.nxs",
                "hrpd/test/cycle_09_2/Calibration/van_s1_old-2.nxs", "hrpd/test/cycle_09_2/tester/mtd.pref"}

    def runTest(self):
        dirs = config['datasearch.directories'].split(';')
        expt = cry_ini.Files('hrpd', RawDir=(dirs[0]), Analysisdir='test', forceRootDirFromScripts=False,
                             inputInstDir=dirs[0])
        expt.initialize('cycle_09_2', user='tester', prefFile='mtd.pref')
        expt.tell()
        cry_focus.focus_all(expt, "43022")

    def validate(self):
        return 'ResultTOFgrp','hrpd/test/cycle_09_2/tester/hrp43022_s1_old.nxs'

    def cleanup(self):
        filenames = {"hrpd/test/cycle_09_2/Calibration/hrpd_new_072_01_corr.cal",
                     "hrpd/test/cycle_09_2/tester/hrp43022_s1_old.gss",
                     "hrpd/test/cycle_09_2/tester/hrp43022_s1_old.nxs",
                     'hrpd/test/cycle_09_2/tester/hrp43022_s1_old_b1_D.dat',
                     'hrpd/test/cycle_09_2/tester/hrp43022_s1_old_b1_TOF.dat',
                     'hrpd/test/cycle_09_2/tester/hrp43022_s1_old_b2_D.dat',
                     'hrpd/test/cycle_09_2/tester/hrp43022_s1_old_b2_TOF.dat',
                     'hrpd/test/cycle_09_2/tester/hrp43022_s1_old_b3_D.dat',
                     'hrpd/test/cycle_09_2/tester/hrp43022_s1_old_b3_TOF.dat',
                     'hrpd/test/cycle_09_2/hrpd_new_072_01_corr.cal'}
        _cleanup_files(config['defaultsave.directory'], filenames)

