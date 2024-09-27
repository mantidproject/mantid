# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init,attribute-defined-outside-init

import systemtesting
from ISIS.SANS.isis_sans_system_test import ISISSansSystemTest

from mantid import config
from mantid.api import FileFinder
from mantid.simpleapi import Load, DeleteWorkspace, mtd
from sans_core.command_interface.ISISCommandInterface import (
    SANS2DTUBES,
    SANS2D,
    Set1D,
    Detector,
    MaskFile,
    Gravity,
    BatchReduce,
    UseCompatibilityMode,
)
import os.path

from sans_core.common.enums import SANSInstrument


@ISISSansSystemTest(SANSInstrument.SANS2D)
class SANS2DBatchTest_V2(systemtesting.MantidSystemTest):
    def runTest(self):
        UseCompatibilityMode()
        SANS2D()
        Set1D()
        Detector("rear-detector")
        MaskFile("MASKSANS2Doptions.091A")
        Gravity(True)

        csv_file = FileFinder.getFullPath("SANS2D_periodTests.csv")
        BatchReduce(csv_file, "nxs", plotresults=False, saveAlgs={"SaveCanSAS1D": "xml", "SaveNexus": "nxs"})
        os.remove(os.path.join(config["defaultsave.directory"], "5512p7_SANS2DBatch_p7rear_1D_2.0_14.0Phi-45.0_45.0.xml"))

    def validate(self):
        self.disableChecking.append("SpectraMap")
        self.disableChecking.append("Axes")
        self.disableChecking.append("Instrument")

        return "5512p7_SANS2DBatch_p7rear_1D_2.0_14.0Phi-45.0_45.0", "SANS2DBatch.nxs"


@ISISSansSystemTest(SANSInstrument.SANS2D)
class SANS2DNewSettingsCarriedAcrossInBatchModeTest_V2(systemtesting.MantidSystemTest):
    """
    We want to make sure that any settings saved in the PropertyManager objects
    are used across all iterations of the reduction in Batch mode.  The MASKFILE
    command uses this new way of storing settings in ISIS SANS, and so we'll
    see if the same masks get applied in the second iteration as they do in the
    first.
    """

    def runTest(self):
        UseCompatibilityMode()
        config["default.instrument"] = "SANS2D"
        SANS2D()
        Set1D()
        Detector("rear-detector")
        # This contains two MASKFILE commands, each resulting in a separate call to MaskDetectors.
        MaskFile("MaskSANS2DReductionGUI_MaskFiles.txt")
        Gravity(True)

        # This does 2 separate reductions of the same data, but saving the result of each to a different workspace.
        csv_file = FileFinder.getFullPath("SANS2D_mask_batch.csv")
        BatchReduce(csv_file, "nxs", plotresults=False)

        path1 = os.path.join(config["defaultsave.directory"], "iteration_1.xml")
        path2 = os.path.join(config["defaultsave.directory"], "iteration_2.xml")
        if os.path.exists(path1):
            os.remove(path1)
        if os.path.exists(path2):
            os.remove(path2)

    def validate(self):
        self.tolerance_is_rel_err = True
        self.tolerance = 1.0e-2
        self.disableChecking.append("Instrument")
        return "iteration_2_rear_1D_1.5_12.5", "SANS2DNewSettingsCarriedAcross.nxs"


@ISISSansSystemTest(SANSInstrument.SANS2D)
class SANS2DTUBESBatchWithZeroErrorCorrectionTest_V2(systemtesting.MantidSystemTest):
    """
    We want to make sure that the BatchMode can remove zero error values
    and replace them with a large default value.
    """

    def runTest(self):
        UseCompatibilityMode()
        config["default.instrument"] = "SANS2D"
        SANS2DTUBES()
        Set1D()
        Detector("rear-detector")
        # This contains two MASKFILE commands, each resulting in a separate call to MaskDetectors.
        MaskFile("SANS2DTube_ZerroErrorFreeTest.txt")

        # Saves a file which produces an output file which does not contain any zero errors
        csv_file = FileFinder.getFullPath("SANS2DTUBES_ZeroErrorFree_batch.csv")
        save_alg = {"SaveNexus": "nxs"}
        BatchReduce(csv_file, "nxs", saveAlgs=save_alg, plotresults=False, save_as_zero_error_free=True)
        DeleteWorkspace("zero_free_out_rear_1D_1.75_12.5")

        # The zero correction only occurs for the saved files. Stephen King mentioned that the
        # original workspaces should not be tampered with
        self._final_output = os.path.join(config["defaultsave.directory"], "zero_free_out_rear_1D_1.75_12.5.nxs")
        self._final_workspace = "ws"
        Load(Filename=self._final_output, OutputWorkspace=self._final_workspace)

    def validate(self):
        self.tolerance_is_rel_err = True
        self.tolerance = 1.0e-2
        self.disableChecking.append("SpectraMap")
        self.disableChecking.append("Instrument")
        self.disableChecking.append("Axes")
        return self._final_workspace, "SANS2DTube_withZeroErrorCorrections.nxs"

    def validateMethod(self):
        return "WorkspaceToNexus"

    def cleanup(self):
        # Delete the stored file
        os.remove(self._final_output)
        # Delete the workspace
        if self._final_workspace in mtd:
            DeleteWorkspace(self._final_workspace)
