# -*- coding: utf-8 -*-# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from mantid.api import AlgorithmFactory, DataProcessorAlgorithm, MatrixWorkspaceProperty, MatrixWorkspace, PropertyMode
from mantid.kernel import Direction


class ReflectometryISISSumBanks(DataProcessorAlgorithm):
    _WORKSPACE = "InputWorkspace"
    _ROI = "ROIDetectorIDs"
    _OUTPUT_WS = "OutputWorkspace"

    def category(self):
        """Return the categories of the algorithm."""
        return "Reflectometry\\ISIS;Workflow\\Reflectometry"

    def name(self):
        """Return the name of the algorithm."""
        return "ReflectometryISISSumBanks"

    def summary(self):
        """Return a summary of the algorithm."""
        return "Sum banks from an ISIS reflectometry 2D detector into a single bank"

    def seeAlso(self):
        """Return a list of related algorithm names."""
        return ["ReflectometryISISLoadAndProcess"]

    def PyInit(self):
        self.declareProperty(
            MatrixWorkspaceProperty(self._WORKSPACE, "", direction=Direction.Input, optional=PropertyMode.Mandatory),
            doc="An input 2D detector workspace",
        )
        self.declareProperty(self._ROI, defaultValue="", doc="List of detector IDs to include")
        self.declareProperty(
            MatrixWorkspaceProperty(self._OUTPUT_WS, "", direction=Direction.Output),
            doc="The preprocessed output workspace. If multiple input runs are specified "
            "they will be summed into a single output workspace.",
        )

    def PyExec(self):
        input_workspace = self.getProperty(self._WORKSPACE).value

        masked_workspace = self.mask_workspace(input_workspace)

        num_banks = self._get_rectangular_detector_component(input_workspace).xpixels()
        if num_banks == 1:
            self.setProperty(self._OUTPUT_WS, masked_workspace)
            return

        summed_workspace = self.sum_banks(masked_workspace, num_banks)
        result = self._prepend_monitors(input_workspace, summed_workspace)
        self.setProperty(self._OUTPUT_WS, result)

    def mask_workspace(self, input_workspace):
        if self.getProperty(self._ROI).isDefault:
            return input_workspace

        roi_detector_ids = self.getProperty(self._ROI).value
        return self.mask_detectors(input_workspace, roi_detector_ids)

    def validateInputs(self):
        issues = dict()
        workspace = self.getProperty(self._WORKSPACE).value
        try:
            self._get_rectangular_detector_component(workspace)
        except Exception as err:
            issues["InputWorkspace"] = str(err)
        return issues

    def mask_detectors(self, workspace: MatrixWorkspace, roi_detector_ids: str) -> MatrixWorkspace:
        # We need to apply the mask to the original workspace so we can extract a MaskWorkspace to use in
        # BinaryOperateMasks which inverts it. First, we take a clone to not destructively alter the original WS
        cloned_ws = self._run_child_with_out_props("CloneWorkspace", InputWorkspace=workspace)
        cloned_ws = self._remove_monitors(cloned_ws)
        self.createChildAlgorithm("MaskDetectors", Workspace=cloned_ws, DetectorList=roi_detector_ids).execute()
        mask_ws = self._run_child_with_out_props("ExtractMask", InputWorkspace=cloned_ws)

        self.createChildAlgorithm("BinaryOperateMasks", InputWorkspace1=mask_ws, OperationType="NOT", OutputWorkspace=mask_ws).execute()

        # and then re-apply this to the original workspace, again taking a clone
        cloned_ws = self._run_child_with_out_props("CloneWorkspace", InputWorkspace=workspace)
        self.createChildAlgorithm("MaskDetectors", Workspace=cloned_ws, MaskedWorkspace=mask_ws).execute()
        return cloned_ws

    def sum_banks(self, workspace: MatrixWorkspace, num_banks: int):
        return self._run_child_with_out_props("SmoothNeighbours", InputWorkspace=workspace, SumPixelsX=num_banks, SumPixelsY=1)

    def _get_rectangular_detector_component(self, workspace: MatrixWorkspace):
        instrument = workspace.getInstrument()
        if not instrument:
            raise RuntimeError("The input workspace must have an instrument")

        rect_detectors = instrument.findRectDetectors()
        if len(rect_detectors) == 0:
            raise RuntimeError("The input workspace must contain a rectangular detector")
        if len(rect_detectors) > 1:
            raise RuntimeError("The input workspace must only contain one rectangular detector: multiple were found")
        return rect_detectors[0]

    def _run_child_with_out_props(self, *args, **kwargs) -> MatrixWorkspace:
        alg = self.createChildAlgorithm(*args, **kwargs)
        alg.execute()
        return alg.getProperty("OutputWorkspace").value

    def _prepend_monitors(self, input_workspace, summed_workspace):
        crop_alg = self.createChildAlgorithm(
            "ExtractMonitors", InputWorkspace=input_workspace, MonitorWorkspace="__preview_summed_ws_monitors"
        )
        crop_alg.execute()
        monitor_workspace = crop_alg.getProperty("MonitorWorkspace").value

        append_alg = self.createChildAlgorithm("AppendSpectra", InputWorkspace1=monitor_workspace, InputWorkspace2=summed_workspace)
        append_alg.execute()
        return append_alg.getProperty("OutputWorkspace").value

    def _remove_monitors(self, input_workspace):
        crop_alg = self.createChildAlgorithm("ExtractMonitors", InputWorkspace=input_workspace, DetectorWorkspace="__preview_ws_detectors")
        crop_alg.execute()
        return crop_alg.getProperty("DetectorWorkspace").value


AlgorithmFactory.subscribe(ReflectometryISISSumBanks)
