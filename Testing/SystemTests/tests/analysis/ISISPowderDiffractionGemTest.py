# pylint: disable=no-init,attribute-defined-outside-init,too-many-public-methods

from mantid.api import AnalysisDataService, MatrixWorkspace, WorkspaceGroup, ITableWorkspace, FileFinder
from mantid.simpleapi import *
from mantid import config
import os.path
import stresstesting
import unittest

import cry_ini
import cry_focus

DIFF_PLACES = 8
DIRS = config['datasearch.directories'].split(';')


class ISISPowderDiffractionGem(stresstesting.MantidStressTest):
    def requiredFiles(self):
        return set(["GEM/GEM46487.raw", "GEM/GEM46488.raw",
                    "GEM/GEM46489.raw", "GEM/VanaPeaks.dat",
                    "GEM/test/GrpOff/offsets_2009_cycle094.cal",
                    "GEM/test/Cycle_09_5/Calibration/van_8mm_15x40_spline30-0.nxs",
                    "GEM/test/Cycle_09_5/Calibration/van_8mm_15x40_spline30-1.nxs",
                    "GEM/test/Cycle_09_5/Calibration/van_8mm_15x40_spline30-2.nxs",
                    "GEM/test/Cycle_09_5/Calibration/van_8mm_15x40_spline30-3.nxs",
                    "GEM/test/Cycle_09_5/Calibration/van_8mm_15x40_spline30-4.nxs",
                    "GEM/test/Cycle_09_5/Calibration/van_8mm_15x40_spline30-5.nxs",
                    "GEM/test/Cycle_09_5/mantid_tester/GEM_095_calibration.pref"])

        # note: VanaPeaks.dat is used only if provided in the directory

    def _clean_up_files(self, filenames, directories):
        try:
            for files in filenames:
                path = os.path.join(directories[0], files)
                os.remove(path)
        except OSError, ose:
            print 'could not delete generated file : ', ose.filename

    def runTest(self):
        self._success = False
        expt = cry_ini.Files('GEM', RawDir=(DIRS[0] + "GEM/"), Analysisdir='test',
                             forceRootDirFromScripts=False, inputInstDir=(DIRS[0]))
        expt.initialize('cycle_09_5', user='mantid_tester', prefFile='GEM_095_calibration.pref')
        expt.tell()
        cry_focus.focus_all(expt, "46489")

        # Custom code to create and run this single test suite
        # and then mark as success or failure
        suite = unittest.TestSuite()
        suite.addTest(unittest.makeSuite(LoadTests, "test"))
        runner = unittest.TextTestRunner()
        # Run using either runner
        res = runner.run(suite)
        if res.wasSuccessful():
            self._success = True
        else:
            self._success = False

    def validate(self):
        return self._success

    def cleanup(self):
        filenames = set(["GEM/test/Cycle_09_5/Calibration/offsets_2009_cycle094.cal",
                         "GEM/test/Cycle_09_5/mantid_tester/GEM46489.gss",
                         "GEM/test/Cycle_09_5/mantid_tester/GEM46489.nxs",
                         'GEM/test/Cycle_09_5/mantid_tester/GEM46489_b1_D.dat',
                         'GEM/test/Cycle_09_5/mantid_tester/GEM46489_b1_TOF.dat',
                         'GEM/test/Cycle_09_5/mantid_tester/GEM46489_b2_D.dat',
                         'GEM/test/Cycle_09_5/mantid_tester/GEM46489_b2_TOF.dat',
                         'GEM/test/Cycle_09_5/mantid_tester/GEM46489_b3_D.dat',
                         'GEM/test/Cycle_09_5/mantid_tester/GEM46489_b3_TOF.dat',
                         'GEM/test/Cycle_09_5/mantid_tester/GEM46489_b4_D.dat',
                         'GEM/test/Cycle_09_5/mantid_tester/GEM46489_b4_TOF.dat',
                         'GEM/test/Cycle_09_5/mantid_tester/GEM46489_b5_D.dat',
                         'GEM/test/Cycle_09_5/mantid_tester/GEM46489_b5_TOF.dat',
                         'GEM/test/Cycle_09_5/mantid_tester/GEM46489_b6_D.dat',
                         'GEM/test/Cycle_09_5/mantid_tester/GEM46489_b6_TOF.dat',
                         'GEM/test/Cycle_09_5/mantid_tester/offsets_2009_cycle094.cal'])

        self._clean_up_files(filenames, DIRS)


# ======================================================================
# work horse
class LoadTests(unittest.TestCase):
    wsname = "__LoadTest"
    cleanup_names = []

    def tearDown(self):
        self.cleanup_names.append(self.wsname)
        for name in self.cleanup_names:
            try:
                AnalysisDataService.remove(name)
            except KeyError:
                pass
        self.cleanup_names = []
