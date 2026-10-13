# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init,invalid-name,bare-except
import mantid.simpleapi as api
from mantid.api import AlgorithmFactory, PythonAlgorithm, WorkspaceProperty
from mantid.kernel import Direction, IntBoundedValidator, Logger
from plugins.algorithms.component_info_utils import get_assembly_children, get_detector_id, resolve_component_index
import numpy as np


class SANSPatchSensitivity(PythonAlgorithm):
    """
    Calculate the detector sensitivity and patch the pixels that are masked in a second workspace.

    Patchs the InputWorkspace based on the mask difined in PatchWorkspace.
    For the masked peaks calculates a regression

    """

    def category(self):
        return "Workflow\\SANS"

    def name(self):
        return "SANSPatchSensitivity"

    def summary(self):
        return "Calculate the detector sensitivity and patch the pixels that are masked in a second workspace. "

    def PyInit(self):
        self.declareProperty(
            WorkspaceProperty("InputWorkspace", "", direction=Direction.Input), doc="Input sensitivity workspace to be patched"
        )
        self.declareProperty(
            WorkspaceProperty("PatchWorkspace", "", direction=Direction.Input),
            doc="Workspace defining the patch. Masked detectors will be patched.",
        )
        self.declareProperty("ComponentName", "", doc="Component Name to apply the patch.")

        self.declareProperty(
            "DegreeOfThePolynomial", defaultValue=1, validator=IntBoundedValidator(0), doc="Degree of the polynomial to fit the patch zone"
        )

        self.declareProperty("OutputMessage", "", direction=Direction.Output, doc="Output message")

    def PyExec(self):
        in_ws = self.getProperty("InputWorkspace").value
        patch_ws = self.getProperty("PatchWorkspace").value
        component_name = self.getProperty("ComponentName").value

        component_info = in_ws.componentInfo()
        component = self.__get_component_to_patch(in_ws, component_name)
        tubes = get_assembly_children(component_info, component)

        for tube_idx in range(len(tubes)):
            if len(get_assembly_children(component_info, tubes[0])) <= 1:
                # Handles EQSANS
                tube = get_assembly_children(component_info, tubes[tube_idx])[0]
            else:
                # Handles Biosans/GPSANS
                tube = tubes[tube_idx]
            self.__patch_workspace(tube, in_ws, patch_ws)

        api.ClearMaskFlag(Workspace=in_ws, ComponentName=component_name)

    def __get_component_to_patch(self, workspace, component_name):
        """
        Get the component index to apply the pacth
        Either from the field or from the IDF parameters
        """
        component_info = workspace.componentInfo()

        # Get the default from the parameters file
        if component_name is None or component_name == "":
            component_name = component_info.getStringParameter("detector-name")[0]
        try:
            component = resolve_component_index(component_name, component_info)
        except ValueError:
            Logger("SANSPatchSensitivity").error("Component not valid! %s" % component_name)
            return
        return component

    def __patch_workspace(self, tube_in_input_ws, in_ws, patch_ws):
        """
        @param tube_in_input_ws :: Component index of the tube to patch
        @param in_ws: Workspace to patch
        @param patch_ws: where the mask is defined

        For every tube:
            In patch_ws : finds the masked pixels_ids
            In in_ws : Finds (id, Y, E) for the non-masked pixels in patch_ws
            Calculates the polynomial for the non-masked pixels
            Fits the  masked pixels_ids and sets Y and E in the in_ws
        """

        # Arrays to calculate the polynomial
        id_to_calculate_fit = []
        y_to_calculate_fit = []
        e_to_calculate_fit = []
        # Array that will be fit
        id_to_fit = []

        patchDetInfo = patch_ws.detectorInfo()
        inputDetInfo = in_ws.detectorInfo()
        inputCompInfo = in_ws.componentInfo()
        for pixel_in_input_ws in get_assembly_children(inputCompInfo, tube_in_input_ws):
            # ID will be the same in both WS
            detector_id = get_detector_id(inputCompInfo, inputDetInfo, pixel_in_input_ws)
            detector_idx = detector_id - 1  # See note on hack below

            if patchDetInfo.isMasked(detector_idx):
                id_to_fit.append(detector_id)
            elif not inputDetInfo.isMasked(detector_idx):
                id_to_calculate_fit.append(detector_id)
                y_to_calculate_fit.append(in_ws.y(detector_idx).sum())
                e_to_calculate_fit.append(in_ws.e(detector_idx).sum())

        degree = self.getProperty("DegreeOfThePolynomial").value
        # Returns coeffcients for the polynomial fit

        if len(id_to_calculate_fit) <= 50:
            Logger("SANSPatchSensitivity").warning("Tube %s has not enough data for polyfit." % inputCompInfo.fullName(tube_in_input_ws))
            return

        py = np.polyfit(id_to_calculate_fit, y_to_calculate_fit, degree)
        pe = np.polyfit(id_to_calculate_fit, e_to_calculate_fit, degree)

        for id_ in id_to_fit:
            # HUGE hack. There's no detector_id to spectrum_idx possibility
            # spect_idx for biosans / gpsans is detector_id -1
            spec_idx = id_ - 1
            vy = np.polyval(py, [id_])
            ve = np.polyval(pe, [id_])
            in_ws.setSharedY(spec_idx, vy)
            in_ws.setSharedE(spec_idx, ve)


AlgorithmFactory.subscribe(SANSPatchSensitivity())
