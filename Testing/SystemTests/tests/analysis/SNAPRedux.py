# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#pylint: disable=no-init,invalid-name,attribute-defined-outside-init
from __future__ import (absolute_import, division, print_function)
import systemtesting
from mantid.simpleapi import *
import os


def _skip_test():
    """Helper function to determine if we run the test"""
    import platform

    # Only runs on RHEL6 at the moment
    return "Linux" not in platform.platform()


def getSaveDir():
    """determine where to save - the current working directory"""
    return os.path.abspath(os.path.curdir)


def do_cleanup():
    Files = [os.path.join('d_spacing', 'SNAP_34172_2_4_Grouping.dat'),
             os.path.join('gsas', 'SNAP_34172_2_4_Grouping.gsa'),
             os.path.join('fullprof', 'SNAP_34172_2_4_Grouping-0.dat'),
             os.path.join('fullprof', 'SNAP_34172_2_4_Grouping-1.dat'),
             os.path.join('nexus', 'SNAP_34172_2_4_Grouping.nxs')]
    savedir = getSaveDir()
    for filename in Files:
        absfile = os.path.join(savedir, filename)
        if os.path.exists(absfile):
            os.remove(absfile)
    for direc in ['d_spacing', 'gsas', 'fullprof', 'nexus']:
        direc = os.path.join(savedir, direc)
        if os.listdir(direc) == []:
            os.rmdir(direc)

    return True


class SNAP_short(systemtesting.MantidSystemTest):
    def skipTests(self):
        return _skip_test()

    def cleanup(self):
        do_cleanup()
        return True

    def requiredFiles(self):
        files = []
        files.append("SNAP_34172_event.nxs")
        return files

    def runTest(self):
        # run the actual code
        SNAPReduce(RunNumbers='34172', Masking='Horizontal', Binning='0.7,-0.004,5.5',
                   Normalization='Extracted from Data', PeakClippingWindowSize=7,
                   SmoothingRange=5, GroupDetectorsBy='2_4 Grouping',
                   SaveData=True, OutputDirectory=getSaveDir())

    def validate(self):
        self.tolerance = 1.0e-2
        self.disableChecking.append('Instrument')  # doesn't validate correctly
        # default validation of workspace to processed nexus is right
        return ('SNAP_34172_2_4_Grouping_nor','SNAP_34172_2_4_Grouping_nor.nxs')


class SNAP_short_detcal(systemtesting.MantidSystemTest):
    def skipTests(self):
        return _skip_test()

    def cleanup(self):
        do_cleanup()
        return True

    def requiredFiles(self):
        files = []
        files.append("SNAP_34172_event.nxs")
        return files

    def runTest(self):
        # run the actual code
        SNAPReduce(RunNumbers='34172', Masking='Horizontal', Binning='0.7,-0.004,5.5',
                   Calibration='DetCal File', DetCalFilename='SNAP_34172.DetCal',
                   Normalization='Extracted from Data', PeakClippingWindowSize=7,
                   SmoothingRange=5, GroupDetectorsBy='2_4 Grouping',
                   SaveData=True, OutputDirectory=getSaveDir())

    def validate(self):
        self.tolerance = 1.0e-2
        self.disableChecking.append('Instrument')  # doesn't validate correctly
        # default validation of workspace to processed nexus is right
        return ('SNAP_34172_2_4_Grouping_nor','SNAP_34172_2_4_Grouping_nor.nxs')
