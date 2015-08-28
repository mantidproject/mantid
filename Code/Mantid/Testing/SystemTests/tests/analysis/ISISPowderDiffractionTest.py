# pylint: disable=no-init
import stresstesting
import os
import sys

sys.path.append('C:\Users\RYQ25391\Documents\GitHub\mantid\Code\Mantid\scripts\PowderISIS')
import cry_ini
import cry_focus
from mantid.simpleapi import *
from mantid.api import FileFinder

from IndirectImport import is_supported_f2py_platform


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
            pass
            print(OSError, "Did not DELETE")
            #mantid.logger.warning("Unable to remove created file '%s'" % filename)
# ==============================================================================

    def cleanup(self):
        Files = ["hrpd/test/cycle_09_2/Calibration/hrpd_new_072_01_corr.cal",
                     "hrpd/test/cycle_09_2/tester/hrp43022_s1_old.gss",
                     "hrpd/test/cycle_09_2/tester/hrp43022_s1_old.nxs",
                     'hrpd/test/cycle_09_2/tester/hrp43022_s1_old_b1_D.dat',
                     'hrpd/test/cycle_09_2/tester/hrp43022_s1_old_b1_TOF.dat',
                     'hrpd/test/cycle_09_2/tester/hrp43022_s1_old_b2_D.dat',
                     'hrpd/test/cycle_09_2/tester/hrp43022_s1_old_b2_TOF.dat',
                     'hrpd/test/cycle_09_2/tester/hrp43022_s1_old_b3_D.dat',
                     'hrpd/test/cycle_09_2/tester/hrp43022_s1_old_b3_TOF.dat',
                     'hrpd/test/cycle_09_2/hrpd_new_072_01_corr.cal']
        for filename in Files:
            absfile = FileFinder.getFullPath(filename)
            if os.path.exists(absfile):
                os.remove(absfile)
        return True



# Simply tests that our LoadRaw and LoadISISNexus algorithms produce the same workspace
class ISISPowderDiffraction(stresstesting.MantidStressTest):
    def skipTests(self):
        if is_supported_f2py_platform():
            return False
        else:
            return True

    def requiredFiles(self):
        return {"HRP39191.raw", "HRP39187.raw", "HRP43022.raw", "hrpd/test/GrpOff/hrpd_new_072_01.cal",
                "hrpd/test/GrpOff/hrpd_new_072_01_corr.cal", "hrpd/test/cycle_09_2/Calibration/van_s1_old-0.nxs",
                "hrpd/test/cycle_09_2/Calibration/van_s1_old-1.nxs",
                'hrpd/test/cycle_09_2/Calibration/van_s1_old-1.nxs', "hrpd/test/cycle_09_2/tester/mtd.pref"}

    def runTest(self):

        dirs = config['datasearch.directories'].split(';')

        print "this is dirs: ", dirs
        expt = cry_ini.Files('hrpd', RawDir=(dirs[0]), Analysisdir='test', forceRootDirFromScripts=False,
                             inputInstDir=dirs[0])
        expt.initialize('cycle_09_2', user='tester', prefFile='mtd.pref')
        expt.tell()
        cry_focus.focus_all(expt, "43022")

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
