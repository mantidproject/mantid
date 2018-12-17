# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#pylint: disable=no-init
from __future__ import (absolute_import, division, print_function)
import systemtesting
from mantid.simpleapi import *


from mantid import config
from ISISCommandInterface import *


class SANS2DMultiPeriodAddFiles(systemtesting.MantidSystemTest):

    def requiredMemoryMB(self):
        """Requires 2.5Gb"""
        return 2500

    def runTest(self):
        pass
        SANS2D()
        Set1D()
        Detector("rear-detector")
        MaskFile('MASKSANS2Doptions.091A')
        Gravity(True)
        add_runs( ('5512', '5512') ,'SANS2D', 'nxs', lowMem=True)

    #one period of a multi-period Nexus file
        AssignSample('5512-add.nxs', period=7)

        WavRangeReduction(2, 4, DefaultTrans)

        paths = [os.path.join(config['defaultsave.directory'],'SANS2D00005512-add.nxs'),
                 os.path.join(config['defaultsave.directory'],'SANS2D00005512.log')]
        for path in paths:
            if os.path.exists(path):
                os.remove(path)

    def validate(self):
    # Need to disable checking of the Spectra-Detector map because it isn't
    # fully saved out to the nexus file (it's limited to the spectra that
    # are actually present in the saved workspace).
        self.disableChecking.append('SpectraMap')
        self.disableChecking.append('Instrument')
        self.disableChecking.append('Axes')

        return '5512p7rear_1D_2.0_4.0Phi-45.0_45.0','SANS2DMultiPeriodAddFiles.nxs'


class LARMORMultiPeriodAddEventFiles(systemtesting.MantidSystemTest):
    def requiredMemoryMB(self):
        """Requires 2.5Gb"""
        return 2500

    def runTest(self):
        LARMOR()
        Set1D()
        Detector("DetectorBench")
        MaskFile('USER_LARMOR_151B_LarmorTeam_80tubes_BenchRot1p4_M4_r3699.txt')
        Gravity(True)
        add_runs( ('13065', '13065') ,'LARMOR', 'nxs', lowMem=True)

        AssignSample('13065-add.nxs')
        WavRangeReduction(2, 4, DefaultTrans)

        # Clean up
        to_clean = ["13065_sans_nxs",
                    "13065p1rear_1D_2.0_4.0_incident_monitor",
                    "13065p2rear_1D_2.0_4.0_incident_monitor",
                    "13065p3rear_1D_2.0_4.0_incident_monitor",
                    "13065p4rear_1D_2.0_4.0_incident_monitor",
                    "80tubeCalibration_1-05-2015_r3157-3160"]
        for workspace in to_clean:
            DeleteWorkspace(workspace)

        paths = [os.path.join(config['defaultsave.directory'],'LARMOR00013065-add.nxs'),
                 os.path.join(config['defaultsave.directory'],'SANS2D00013065.log')]  # noqa
        for path in paths:
            if os.path.exists(path):
                os.remove(path)

    def validate(self):
    # Need to disable checking of the Spectra-Detector map because it isn't
    # fully saved out to the nexus file (it's limited to the spectra that
    # are actually present in the saved workspace).
        self.disableChecking.append('SpectraMap')
        self.disableChecking.append('Instrument')
        self.disableChecking.append('Axes')

        return "13065p1rear_1D_2.0_4.0" , "LARMORMultiPeriodAddEventFiles.nxs"
