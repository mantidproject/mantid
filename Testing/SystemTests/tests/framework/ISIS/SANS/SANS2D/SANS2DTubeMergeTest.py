# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

import systemtesting
from ISIS.SANS.isis_sans_system_test import ISISSansSystemTest
from sans_core.common.enums import SANSInstrument

from mantid.simpleapi import SANSTubeMerge


@ISISSansSystemTest(SANSInstrument.SANS2D)
class SANS2DTubeMergeTest(systemtesting.MantidSystemTest):
    _FRONT_CALIBRATION_FILE = "SANS2DTubeMerge_front.nxs"
    _REAR_CALIBRATION_FILE = "SANS2DTubeMerge_rear.nxs"
    _OUTPUT_WORKSPACE_NAME = "empty_instr"
    _REFERENCE_FILE = "SANS2DTubeMerge_empty_instr.nxs"

    def requiredFiles(self):
        return [self._FRONT_CALIBRATION_FILE, self._REAR_CALIBRATION_FILE, self._REFERENCE_FILE]

    def runTest(self):
        SANSTubeMerge(Front=self._FRONT_CALIBRATION_FILE, Rear=self._REAR_CALIBRATION_FILE)

    def validate(self):
        self.disableChecking.append("SpectraMap")
        self.disableChecking.append("Axes")

        return self._OUTPUT_WORKSPACE_NAME, self._REFERENCE_FILE
