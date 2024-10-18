# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init,invalid-name
import systemtesting
from mantid.simpleapi import Load
from isis_reflectometry import quick


class ReflectometryQuickMultiDetector(systemtesting.MantidSystemTest):
    """
    This is a system test for the top-level quick routines. Quick is the name given to the
    ISIS reflectometry reduction scripts.

    This test uses the multidetector functionality within the script.
    No transmission runs are passed, so it uses correction algorithms instead.
    """

    def runTest(self):
        workspace_name = "POLREF4699"
        workspace_nexus_file = workspace_name + ".nxs"
        ws = Load(workspace_nexus_file, OutputWorkspace=workspace_name)

        first_ws = ws[0]

        quick.quick_explicit(
            first_ws,
            i0_monitor_index=0,
            lambda_min=0.8,
            lambda_max=14.5,
            background_min=0.8,
            background_max=14.5,
            int_min=0.8,
            int_max=14.5,
            point_detector_start=0,
            point_detector_stop=245,
            multi_detector_start=1,
            theta=0,
            pointdet=False,
            roi=[74, 74],
        )

    def validate(self):
        self.nanEqual = True
        self.disableChecking.append("Instrument")
        return "4699_IvsQ", "4699_IvsQ_Result.nxs"
