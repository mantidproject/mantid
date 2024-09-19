# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init,attribute-defined-outside-init

import systemtesting
from ISIS.SANS.isis_sans_system_test import ISISSansSystemTest
from mantid.simpleapi import *
from ISISCommandInterface import *
from mantid import config
from SANSBatchMode import *
import os.path

# test batch mode with sans2d and selecting a period in batch mode
from SANS.sans.common.enums import SANSInstrument


@ISISSansSystemTest(SANSInstrument.SANS2D)
class SANS2DBatch(systemtesting.MantidSystemTest):
    def runTest(self):
        SANS2D()
        Set1D()
        Detector("rear-detector")
        MaskFile("MASKSANS2Doptions.091A")
        Gravity(True)

        csv_file = FileFinder.getFullPath("SANS2D_periodTests.csv")

        BatchReduce(csv_file, "nxs", plotresults=False, saveAlgs={"SaveCanSAS1D": "xml", "SaveNexus": "nxs"})

        os.remove(os.path.join(config["defaultsave.directory"], "5512p7_SANS2DBatch.xml"))

    def validate(self):
        # Need to disable checking of the Spectra-Detector map because it isn't
        # fully saved out to the nexus file (it's limited to the spectra that
        # are actually present in the saved workspace).
        self.disableChecking.append("SpectraMap")
        self.disableChecking.append("Axes")
        self.disableChecking.append("Instrument")

        return "5512p7_SANS2DBatch", "SANS2DBatch.nxs"


@ISISSansSystemTest(SANSInstrument.SANS2D)
class SANS2DNewSettingsCarriedAcrossInBatchMode(systemtesting.MantidSystemTest):
    """
    We want to make sure that any settings saved in the PropertyManager objects
    are used across all iterations of the reduction in Batch mode.  The MASKFILE
    command uses this new way of storing settings in ISIS SANS, and so we'll
    see if the same masks get applied in the second iteration as they do in the
    first.
    """

    def runTest(self):
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

    def validate(self):
        self.tolerance_is_rel_err = True
        self.tolerance = 1.0e-2
        self.disableChecking.append("Instrument")
        return "iteration_2", "SANS2DNewSettingsCarriedAcross.nxs"


@ISISSansSystemTest(SANSInstrument.SANS2D)
class SANS2DTUBESBatchWithZeroErrorCorrection(systemtesting.MantidSystemTest):
    """
    We want to make sure that the BatchMode can remove zero error values
    and replace them with a large default value.
    """

    def runTest(self):
        config["default.instrument"] = "SANS2D"
        SANS2DTUBES()
        Set1D()
        Detector("rear-detector")
        # This contains two MASKFILE commands, each resulting in a separate call to MaskDetectors.
        MaskFile("SANS2DTube_ZerroErrorFreeTest.txt")

        # Saves a file which produces an output file which does not contain any zero errors
        csv_file = FileFinder.getFullPath("SANS2DTUBES_ZeroErrorFree_batch.csv")
        saveAlg = {"SaveNexus": "nxs"}
        BatchReduce(csv_file, "nxs", saveAlgs=saveAlg, plotresults=False, save_as_zero_error_free=True)
        DeleteWorkspace("zero_free_out")

        # The zero correction only occurs for the saved files. Stephen King mentioned that the
        # original workspaces should not be tampered with
        self._final_output = os.path.join(config["defaultsave.directory"], "zero_free_out.nxs")
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
