# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from abc import ABCMeta, abstractmethod
from systemtesting import MantidSystemTest

from mantid.api import AlgorithmFactory, AlgorithmManager
from mantid.simpleapi import CreateSampleWorkspace, CreateMDWorkspace, GroupWorkspaces
from mantidqt.interfacemanager import InterfaceManager
from mantidqt.utils.qt.testing import get_application

from qtpy.QtCore import Qt
# Import sip after Qt. Modern versions of PyQt ship an internal sip module
# located at PyQt5X.sip. Importing PyQt first sets a shim sip module to point
# to the correct place
import sip


def create_workspace_in_ads(workspace_type: str) -> None:
    """Creates a workspace in the ADS which can be auto-selected by an algorithm dialog."""
    if workspace_type == "MatrixWorkspace":
        CreateSampleWorkspace(OutputWorkspace="ws")
    elif workspace_type == "MDWorkspace":
        CreateMDWorkspace(Dimensions="3", EventType="MDEvent", Extents='-10,10,-5,5,-1,1',
                          Names="Q_lab_x,Q_lab_y,Q_lab_z", Units="1\\A,1\\A,1\\A", OutputWorkspace="ws")
    elif workspace_type == "WorkspaceGroup":
        ws1 = CreateSampleWorkspace()
        ws2 = CreateSampleWorkspace()
        GroupWorkspaces(InputWorkspaces=[ws1, ws2], OutputWorkspace="group")
    else:
        raise RuntimeError("An unexpected workspace type was provided.")


class AlgorithmDialogsStartupTestBase(MantidSystemTest, metaclass=ABCMeta):
    """
    The base class for a system tests that test the Algorithm Dialogs open ok with different workspaces in the ADS.
    The test will only open the most recent version of an algorithm.
    """

    def __init__(self):
        super(AlgorithmDialogsStartupTestBase, self).__init__()
        self._app = get_application()
        self._interface_manager = InterfaceManager()

        algorithm_names = [algorithm.name for algorithm in AlgorithmFactory.Instance().getDescriptors(True, True)]
        # Remove duplicate algorithm names as we will only test the most recent versions
        self._unique_algorithm_names = list(dict.fromkeys(algorithm_names))

        self._setup_test()

    @abstractmethod
    def _setup_test(self):
        raise NotImplementedError("This method needs implementing in a subclass of this class.")

    def runTest(self) -> None:
        """Run the test for a random workspace type as a speed-up measure whilst still having a degree of coverage"""
        if len(self._unique_algorithm_names) == 0:
            self.fail("Failed to find any of the Algorithms.")

        create_workspace_in_ads(self._workspace_type)

        print(f"Running the AlgorithmDialogsStartupTest with a '{self._workspace_type}' in the ADS")
        print(f"Excluding the following algorithms from the test: '{str(self._exclude_algorithms)}'")

        for algorithm_name in self._unique_algorithm_names:
            if algorithm_name not in self._exclude_algorithms:
                # print(algorithm_name)  # Useful for debugging when an algorithm dialog crashes
                self._attempt_to_open_algorithm_dialog(self._workspace_type, algorithm_name)

    def _attempt_to_open_algorithm_dialog(self, workspace_type: str, algorithm_name: str) -> None:
        """Attempt to open the most recent version of the algorithm provided."""
        try:
            dialog = self._interface_manager.createDialogFromName(algorithm_name, -1, None, False,
                                                                  self._get_algorithm_preset_values(algorithm_name))
            dialog.setAttribute(Qt.WA_DeleteOnClose, True)
            dialog.show()
            dialog.close()

            # Ensure the dialog has been deleted
            sip.delete(dialog)
            self.assertTrue(sip.isdeleted(dialog))
        except Exception as ex:
            self.fail(f"Exception thrown when attempting to open the '{algorithm_name}' algorithm dialog with a "
                      f"'{workspace_type}' in the ADS: {ex}.")

    @staticmethod
    def _get_algorithm_preset_values(algorithm_name: str) -> dict:
        """We want to set the OutputWorkspace on algorithms when possible so that validateInputs gets triggered."""
        algorithm = AlgorithmManager.create(algorithm_name)
        return {"OutputWorkspace": "test_output"} if algorithm.existsProperty("OutputWorkspace") else {}


class AlgorithmDialogsStartupMatrixWorkspaceTest(AlgorithmDialogsStartupTestBase):
    """
    A system test for testing that the Algorithm Dialogs open ok with a MatrixWorkspace in the ADS.
    """

    def _setup_test(self):
        self._workspace_type = "MatrixWorkspace"
        # These algorithms currently fail to open when the given workspace type is auto-selected from the ADS
        self._exclude_algorithms = ["HB2AReduce", "MaskBinsIf", "MuonPairingAsymmetry"]


class AlgorithmDialogsStartupMDWorkspaceTest(AlgorithmDialogsStartupTestBase):
    """
    A system test for testing that the Algorithm Dialogs open ok with a MDWorkspace in the ADS.
    """
    def _setup_test(self):
        self._workspace_type = "MDWorkspace"
        # These algorithms currently fail to open when the given workspace type is auto-selected from the ADS
        self._exclude_algorithms = ["HB2AReduce", "MaskAngle", "MuonPairingAsymmetry", "TOFTOFCropWorkspace",
                                    "VesuvioResolution"]


class AlgorithmDialogsStartupWorkspaceGroupTest(AlgorithmDialogsStartupTestBase):
    """
    A system test for testing that the Algorithm Dialogs open ok with a WorkspaceGroup in the ADS.
    """
    def _setup_test(self):
        self._workspace_type = "WorkspaceGroup"
        # These algorithms currently fail to open when the given workspace type is auto-selected from the ADS
        self._exclude_algorithms = ["AnvredCorrection", "CalculateDynamicRange", "CopyDataRange", "CreateEPP",
                                    "CropToComponent", "CylinderPaalmanPingsCorrection",
                                    "EnggEstimateFocussedBackground", "FlatPlatePaalmanPingsCorrection",
                                    "GetDetectorOffsets", "HB2AReduce", "HB3APredictPeaks", "LorentzCorrection",
                                    "MaskBinsIf", "MaskNonOverlappingBins", "MSDFit", "MuonPairingAsymmetry",
                                    "PDConvertRealSpace", "RebinRagged", "SetMDFrame", "Stitch1D", "Symmetrise",
                                    "TOFTOFCropWorkspace", "VesuvioResolution", "XrayAbsorptionCorrection"]
