# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init

import systemtesting
from ISIS.SANS.isis_sans_system_test import ISISSansSystemTest
from sans.command_interface.ISISCommandInterface import (
    AssignCan,
    AssignSample,
    Detector,
    Gravity,
    LimitsWav,
    LOQ,
    MaskFile,
    Set2D,
    TransFit,
    TransmissionCan,
    TransmissionSample,
    WavRangeReduction,
)
from sans.common.enums import SANSInstrument


@ISISSansSystemTest(SANSInstrument.LOQ)
class SANSLOQTransFitWorkspace2D(systemtesting.MantidSystemTest):
    """
    Tests the SANS interface commands TransFit() and TransWorkspace(). Also tests
    a LOQ reduction in 2D with can and transmission files
    """

    def runTest(self):
        # Testing transmission fitting on LOQ Workspace 2D
        self.setup()

        # test TransFit()
        TransmissionSample("54435.raw", "54433.raw")
        TransmissionCan("54434.raw", "54433.raw")
        TransFit("LOG", 3.0, 8.0)
        # run the reduction
        WavRangeReduction(3, 4, False, "_suff")

    def setup(self):
        LOQ()
        MaskFile("MASK.094AA")
        Gravity(False)
        Set2D()
        Detector("main-detector-bank")
        AssignSample("54431.raw")
        AssignCan("54432.raw")
        LimitsWav(3, 4, 0.2, "LIN")

    def validate(self):
        self.disableChecking.append("SpectraMap")
        # when comparing LOQ files you seem to need the following
        self.disableChecking.append("Axes")
        self.disableChecking.append("Instrument")
        return "54431_main_2D_3.0_4.0_suff", "LOQTransFitWorkspace2D.nxs"
