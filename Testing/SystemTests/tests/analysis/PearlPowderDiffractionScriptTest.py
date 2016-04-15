from mantid.simpleapi import *
from mantid import config
import os.pathimport

shutil
import stresstesting

DIRS = config['datasearch.directories'].split(';')


class PearlPowderDiffractionScriptTest(stresstesting.MantidStressTest):
    def requiredFiles(self):
        filenames = []

        # existing calibration files
        filenames.extend(('PEARL/Calibration/pearl_group_12_1_TT70.cal', 'PEARL/Calibration/pearl_offset_15_3.cal',
                          'PEARL/Calibration/van_spline_TT70_cycle_15_4.nxs',
                          'PEARL/Attentuation/PRL112_DC25_10MM_FF.OUT'))
        # raw files / run numbers 92476-92479
        for i in range(6, 10):
            filenames.append('PEARL/RawFiles/PEARL0009247' + str(i))

        return filenames

    def _clean_up_files(self, filenames, directories):
        try:
            for files in filenames:
                path = os.path.join(directories[0], filenames)
                os.remove(path)
        except OSError, ose:
            print 'could not delete the generated file: ', ose.filename
