# pylint: disable=no-init
import stresstesting
import os
from mantid.simpleapi import *


#==============================================================================

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

#==============================================================================

# Simply tests that our LoadRaw and LoadISISNexus algorithms produce the same workspace
class ISISPowderDiffractionTest(stresstesting.MantidStressTest):
    def requiredFiles(self):
        return ["hrpd_new_072_01.cal", "hrpd_new_072_01.cal_corr.cal", "van_s1_old-0.nxs", "van_s1_old-1.nxs",
                "van_s1_old-1.nxs", "mtd.pref"]

    def runTest(self):
        import cry_init as Main
        config['datasearch.searcharchive'] = 'On'
        expt = cry_ini.Files("hrpd", Analysisdir='test')
        expt.initialize('cycle_09_2', user='tester', prefFile='mtd.pref')
        expt.tell()
        cry_focus.focus_all(expt, "43022")

    def validate(self):

     def cleanup(self):
    filenames = ['hrp43022_s1_old.gss','hrp43022_s1_old.nxs','hrp43022_s1_old_b1_D.dat','hrp43022_s1_old_b1_TOF.dat',
                         'hrp43022_s1_old_b2_D.dat','hrp43022_s1_old_b2_TOF.dat',
                         'hrp43022_s1_old_b3_D.dat','hrp43022_s1_old_b3_TOF.dat',
                         'hrpd_new_072_01_corr.cal' ]
        _cleanup_files(config['defaultsave.directory'], filenames)


