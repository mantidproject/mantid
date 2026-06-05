# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init

import systemtesting
from ISIS.SANS.isis_sans_system_test import ISISSansSystemTest
from sans.command_interface.ISISCommandInterface import AssignSample, Gravity, MaskFile, SANS2D, Set1D, SetDetectorOffsets, director
from sans.common.enums import SANSInstrument

tolerance = 1e-6


@ISISSansSystemTest(SANSInstrument.SANS2D)
class SANSQResolutionWithoutGravity(systemtesting.MantidSystemTest):
    def runTest(self):

        SANS2D()
        # MaskFile contains setting for Q Resolution
        MaskFile("MASKSANS2D_094i_RKH_QRes1.txt")
        SetDetectorOffsets("REAR", -16.0, 58.0, 0.0, 0.0, 0.0, 0.0)
        SetDetectorOffsets("FRONT", -44.0, -20.0, 47.0, 0.0, 1.0, 1.0)
        Gravity(False)
        Set1D()
        AssignSample("2500.nxs")
        state = director.process_commands()
        q = state.convert_to_q
        self.assertTrue(q.use_q_resolution)
        self.assertDelta(q.q_resolution_a1, 0.002, tolerance)
        self.assertDelta(q.q_resolution_a2, 0.003, tolerance)
        self.assertDelta(q.q_resolution_delta_r, 0.002, tolerance)
        self.assertDelta(q.q_resolution_collimation_length, 10.0, tolerance)
        self.assertEqual(q.moderator_file, "ModeratorStdDev_TS2_SANS_30Jul2015.txt")

    def validate(self):
        self.disableChecking.append("Instrument")
        return True


@ISISSansSystemTest(SANSInstrument.SANS2D)
class SANSQResolutionWithGravity(systemtesting.MantidSystemTest):
    def runTest(self):
        SANS2D()
        # MaskFile contains setting for Q Resolution
        MaskFile("MASKSANS2D_094i_RKH_QRes2.txt")
        SetDetectorOffsets("REAR", -16.0, 58.0, 0.0, 0.0, 0.0, 0.0)
        SetDetectorOffsets("FRONT", -44.0, -20.0, 47.0, 0.0, 1.0, 1.0)
        Gravity(flag=True, extra_length=10.0)
        Set1D()
        AssignSample("2500.nxs")
        state = director.process_commands()
        q = state.convert_to_q
        self.assertTrue(q.use_q_resolution)
        self.assertDelta(q.q_resolution_h1, 0.002, tolerance)
        self.assertDelta(q.q_resolution_h2, 0.004, tolerance)
        self.assertDelta(q.q_resolution_w1, 0.003, tolerance)
        self.assertDelta(q.q_resolution_w2, 0.005, tolerance)
        self.assertDelta(q.q_resolution_delta_r, 0.002, tolerance)
        self.assertDelta(q.q_resolution_collimation_length, 5.0, tolerance)
        self.assertEqual(q.moderator_file, "ModeratorStdDev_TS2_SANS_30Jul2015.txt")
        self.assertTrue(q.use_gravity)
        self.assertDelta(q.gravity_extra_length, 10.0, tolerance)

    def validate(self):
        self.disableChecking.append("Instrument")
        return True
