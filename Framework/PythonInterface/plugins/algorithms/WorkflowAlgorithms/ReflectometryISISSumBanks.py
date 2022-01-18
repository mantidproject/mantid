# -*- coding: utf-8 -*-# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from mantid.api import (AlgorithmFactory, DataProcessorAlgorithm, MatrixWorkspaceProperty, MatrixWorkspace, PropertyMode)
from mantid.geometry import RectangularDetector
from mantid.kernel import Direction


class ReflectometryISISSumBanks(DataProcessorAlgorithm):
    _WORKSPACE = 'InputWorkspace'
    _ROI = 'ROIDetectorIDs'
    _OUTPUT_WS = 'OutputWorkspace'

    def category(self):
        """Return the categories of the algorithm."""
        return 'Reflectometry\\ISIS;Workflow\\Reflectometry'

    def name(self):
        """Return the name of the algorithm."""
        return 'ReflectometryISISSumBanks'

    def summary(self):
        """Return a summary of the algorithm."""
        return "Sum banks from an ISIS reflectometry 2D detector into a single bank"

    def seeAlso(self):
        """Return a list of related algorithm names."""
        return ['ReflectometryISISLoadAndProcess']

    def PyInit(self):
        self.declareProperty(
            MatrixWorkspaceProperty(self._WORKSPACE, '', direction=Direction.Input, optional=PropertyMode.Mandatory),
            doc='An input 2D detector workspace')
        self.declareProperty(self._ROI, defaultValue="",
                             doc="List of detector IDs to include")
        self.declareProperty(
            MatrixWorkspaceProperty(self._OUTPUT_WS, '', direction=Direction.Output),
            doc='The preprocessed output workspace. If multiple input runs are specified '
                'they will be summed into a single output workspace.')

    def PyExec(self):
        input_workspace = self.getProperty(self._WORKSPACE).value

        if not self.getProperty(self._ROI).isDefault:
            roi_detector_ids = self.getProperty(self._ROI).value
            masked_workspace = self.mask_detectors(input_workspace, roi_detector_ids)
            summed_workspace = self.sum_banks(masked_workspace)
        else:
            summed_workspace = self.sum_banks(input_workspace)

        self.setProperty(self._OUTPUT_WS, summed_workspace)

    def validateInputs(self):
        issues = dict()
        workspace = self.getProperty(self._WORKSPACE).value
        try:
            self._get_rectangular_detector_component(workspace)
        except Exception as err:
            issues['InputWorkspace'] = str(err)
        return issues

    def mask_detectors(self, workspace: MatrixWorkspace, roi_detector_ids: str) -> MatrixWorkspace:
        # We need to apply the mask to the original workspace so we can extract a MaskWorkspace to use in
        # BinaryOperateMasks which inverts it. First, we take a clone to not destructively alter the original WS
        cloned_ws = self._run_child_with_out_props("CloneWorkspace", InputWorkspace=workspace)
        self.createChildAlgorithm('MaskDetectors', Workspace=cloned_ws, DetectorList=roi_detector_ids).execute()
        mask_ws = self._run_child_with_out_props('ExtractMask', InputWorkspace=cloned_ws)

        self.createChildAlgorithm('BinaryOperateMasks', InputWorkspace1=mask_ws,
                                  OperationType='NOT', OutputWorkspace=mask_ws).execute()

        # and then re-apply this to the original workspace, again taking a clone
        cloned_ws = self._run_child_with_out_props("CloneWorkspace", InputWorkspace=workspace)
        self.createChildAlgorithm('MaskDetectors', Workspace=cloned_ws, MaskedWorkspace=mask_ws).execute()
        return cloned_ws

    def sum_banks(self, workspace: MatrixWorkspace):
        component = self._get_rectangular_detector_component(workspace)
        num_banks = component.xpixels()
        return self._run_child_with_out_props('SmoothNeighbours', InputWorkspace=workspace,
                                              SumPixelsX=num_banks, SumPixelsY=1)

    def _get_rectangular_detector_component(self, workspace: MatrixWorkspace):
        result = None
        inst = workspace.getInstrument()
        if not inst:
            raise RuntimeError('The input workspace must have an instrument')

        for idx in range(inst.nelements()):
            component = inst[idx]
            if type(component) == RectangularDetector:
                if result:
                    raise RuntimeError('The input workspace must only contain one rectangular detector: multiple were found')
                result = component
        if not result:
            raise RuntimeError('The input workspace must contain a rectangular detector')
        return result

    def _run_child_with_out_props(self, *args, **kwargs) -> MatrixWorkspace:
        alg = self.createChildAlgorithm(*args, **kwargs)
        alg.execute()
        return alg.getProperty("OutputWorkspace").value


AlgorithmFactory.subscribe(ReflectometryISISSumBanks)
