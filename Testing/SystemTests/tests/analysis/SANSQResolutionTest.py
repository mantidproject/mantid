# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#pylint: disable=no-init

from __future__ import (absolute_import, division, print_function)
import stresstesting
from mantid.simpleapi import *
from ISISCommandInterface import *


class SANSQResolutionWithoutGravity(stresstesting.MantidStressTest):
    def runTest(self):
        SANS2D()
        MaskFile('MASKSANS2D_094i_RKH.txt')
        SetDetectorOffsets('REAR', -16.0, 58.0, 0.0, 0.0, 0.0, 0.0)
        SetDetectorOffsets('FRONT', -44.0, -20.0, 47.0, 0.0, 1.0, 1.0)
        Gravity(False)
        Set1D()
        AssignSample('2500.nxs')

        # Provide settings for QResolution
        set_q_resolution_use(True)
        moderator_file_name = "ModeratorStdDev_TS2_SANS_30Jul2015.txt"
        set_q_resolution_a1(a1 = 2)
        set_q_resolution_a2(a2 = 3)
        set_q_resolution_delta_r(delta_r = 2)
        set_q_resolution_collimation_length(collimation_length=10)
        set_q_resolution_moderator(file_name = moderator_file_name)

        WavRangeReduction(4.6, 12.85, False)

    def validate(self):
        self.disableChecking.append('Instrument')
        return True


class SANSQResolutionWithGravity(stresstesting.MantidStressTest):
    def runTest(self):
        SANS2D()
        MaskFile('MASKSANS2D_094i_RKH.txt')
        SetDetectorOffsets('REAR', -16.0, 58.0, 0.0, 0.0, 0.0, 0.0)
        SetDetectorOffsets('FRONT', -44.0, -20.0, 47.0, 0.0, 1.0, 1.0)
        Gravity(flag = True, extra_length = 10.0)
        Set1D()
        AssignSample('2500.nxs')

        # Provide settings for QResolution
        set_q_resolution_use(True)
        moderator_file_name = "ModeratorStdDev_TS2_SANS_30Jul2015.txt"
        set_q_resolution_h1(h1 = 2)
        set_q_resolution_w1(w1 = 3)
        set_q_resolution_h2(h2 = 4)
        set_q_resolution_w2(w2 = 5)

        set_q_resolution_delta_r(delta_r = 2)
        set_q_resolution_collimation_length(collimation_length=5)
        set_q_resolution_moderator(file_name = moderator_file_name)

        WavRangeReduction(4.6, 12.85, False)

    def validate(self):
        self.disableChecking.append('Instrument')
        return True
