""" SANSDiagnosticIntegral algorithm performs the workspace reshaping and integration needed by the diagnostic tab."""

from __future__ import (absolute_import, division, print_function)
from mantid.kernel import (Direction, PropertyManagerProperty, StringListValidator)
from mantid.api import (DataProcessorAlgorithm, MatrixWorkspaceProperty, AlgorithmFactory, PropertyMode,
                        IEventWorkspace, Progress)

from sans.state.state_base import create_deserialized_sans_state_from_property_manager
from sans.common.constants import EMPTY_NAME
from sans.common.general_functions import (create_child_algorithm, append_to_sans_file_tag, create_managed_non_child_algorithm)
from sans.common.enums import (DetectorType, DataType, IntegralEnum)
from mantid.api import AlgorithmPropertyWithValue
from sans.algorithm_detail.batch_execution import set_output_workspaces_on_load_algorithm, get_workspace_from_algorithm
from sans.common.file_information import get_instrument_paths_for_sans_file
from sans.common.xml_parsing import get_named_elements_from_ipf_file
from mantid.simpleapi import SumSpectra, ConvertAxesToRealSpace

class SANSDiagnosticIntegral(DataProcessorAlgorithm):
    def category(self):
        return 'SANS\\Diagnostic'

    def summary(self):
        return ' Sums a workspace into columns or rows.'

    def PyInit(self):
        # ----------
        # INPUT
        # ----------
        self.declareProperty(PropertyManagerProperty('SANSState'),
                             doc='A property manager which fulfills the SANSState contract.')

        self.declareProperty(PropertyManagerProperty('Ranges'),
                             doc='A property manager which fulfills the SANSState contract.')

        # The component
        allowed_detectors = StringListValidator([DetectorType.to_string(DetectorType.LAB),
                                                 DetectorType.to_string(DetectorType.HAB)])
        self.declareProperty("Component", DetectorType.to_string(DetectorType.LAB),
                             validator=allowed_detectors, direction=Direction.Input,
                             doc="The component of the instrument which is to be reduced.")

        self.declareProperty('Mask', defaultValue=False,
                             direction=Direction.Input,
                             doc='A boolean value controlling whether a mask is applied to the input workspace')

        # Integral Type
        allowed_integrals = StringListValidator([IntegralEnum.to_string(IntegralEnum.Horizontal),
                                                 IntegralEnum.to_string(IntegralEnum.Vertical),
                                                 IntegralEnum.to_string(IntegralEnum.Time)])

        self.declareProperty("Integral", IntegralEnum.to_string(IntegralEnum.Horizontal),
                             validator=allowed_integrals, direction=Direction.Input,
                             doc="The type of integral to do.")

        self.declareProperty("RangeMin", AlgorithmPropertyWithValue.EMPTY_INT,
                             direction=Direction.Input, doc="Bottom of range to integrate.")
        self.declareProperty("RangeMax", AlgorithmPropertyWithValue.EMPTY_INT,
                             direction=Direction.Input, doc="Top of range to integrate.")

        # ----------
        # OUTPUT
        # ----------
        self.declareProperty(MatrixWorkspaceProperty("OutputWorkspace", '', direction=Direction.Output),
                             doc='The output workspace.')

    def PyExec(self):
        state = self._get_state()
        state_serialized = state.property_manager
        component_as_string = self.getProperty("Component").value
        progress = self._get_progress()

        # --------------------------------------------------------------------------------------------------------------
        # 1. Load Workspace
        # --------------------------------------------------------------------------------------------------------------
        workspace = self._load_workspace(state)

        # --------------------------------------------------------------------------------------------------------------
        # 2. Crop workspace by detector name
        #    This will create a reduced copy of the original workspace with only those spectra which are relevant
        #    for this particular reduction.
        # --------------------------------------------------------------------------------------------------------------
        progress.report("Cropping ...")
        workspace = self._get_cropped_workspace(workspace, component_as_string)

        # --------------------------------------------------------------------------------------------------------------
        # 3. Apply masking (pixel masking and time masking)
        # --------------------------------------------------------------------------------------------------------------
        mask = self.getProperty('Mask')
        if mask:
            progress.report("Masking ...")
            workspace = self._mask(state_serialized, workspace, component_as_string)

        # --------------------------------------------------------------------------------------------------------------
        # 3. Retrieve detector dimensions from instrument file
        # --------------------------------------------------------------------------------------------------------------
        instrument_file = get_instrument_paths_for_sans_file(state.data.sample_scatter)
        if component_as_string == DetectorType.to_string(DetectorType.HAB):
            x_dim = get_named_elements_from_ipf_file(instrument_file[1], "high-angle-detector-num-columns",
                                                     float)['high-angle-detector-num-columns']
            y_dim = get_named_elements_from_ipf_file(instrument_file[1], "high-angle-detector-num-rows",
                                                     float)['high-angle-detector-num-rows']
        else:
            x_dim = get_named_elements_from_ipf_file(instrument_file[1], "low-angle-detector-num-columns", float)[
                'low-angle-detector-num-columns']
            y_dim = get_named_elements_from_ipf_file(instrument_file[1], "low-angle-detector-num-rows", float)[
                'low-angle-detector-num-rows']

        # # --------------------------------------------------------------------------------------------------------------
        # # 3. Reshape and Sum workspace
        # # --------------------------------------------------------------------------------------------------------------
        # integral = self.getProperty("Integral")
        # if integral == IntegralEnum.Horizontal:
        #     ConvertAxesToRealSpace(InputWorkspace=workspace, OutputWorkspace=output_workspace, VerticalAxis='x',
        #                            HorizontalAxis='y', NumberVerticalBins=int(x_dim), NumberHorizontalBins=int(y_dim))
        #     SumSpectra(InputWorkspace=output_workspace, OutputWorkspace=output_workspace, StartWorkspaceIndex=hv_min,
        #                EndWorkspaceIndex=hv_max)
        # elif integral == IntegralEnum.Vertical:
        #     ConvertAxesToRealSpace(InputWorkspace=workspace, OutputWorkspace=output_workspace, VerticalAxis='y',
        #                            HorizontalAxis='x', NumberVerticalBins=int(x_dim), NumberHorizontalBins=int(y_dim))
        #     SumSpectra(InputWorkspace=output_workspace, OutputWorkspace=output_workspace, StartWorkspaceIndex=hv_min,
        #                EndWorkspaceIndex=hv_max)
        # elif integral == IntegralEnum.Time:
        #     SumSpectra(InputWorkspace=workspace, OutputWorkspace=output_workspace,
        #                StartWorkspaceIndex=hv_min, EndWorkspaceIndex=hv_max)

    def _get_state(self):
        state_property_manager = self.getProperty("SANSState").value
        state = create_deserialized_sans_state_from_property_manager(state_property_manager)
        state.property_manager = state_property_manager
        return state

    def _get_progress(self):
        return Progress(self, start=0.0, end=1.0, nreports=10)

    def _get_cropped_workspace(self, workspace, component):
        scatter_workspace = self.getProperty("ScatterWorkspace").value
        crop_name = "SANSCrop"
        crop_options = {"InputWorkspace": scatter_workspace,
                        "OutputWorkspace": EMPTY_NAME,
                        "Component": component}
        crop_alg = create_child_algorithm(self, crop_name, **crop_options)
        crop_alg.execute()
        return crop_alg.getProperty("OutputWorkspace").value

    def _load_workspace(self, state):
        use_optimizations = True
        # Load the data
        state_serialized = state.property_manager
        load_name = "SANSLoad"
        load_options = {"SANSState": state_serialized,
                        "PublishToCache": use_optimizations,
                        "UseCached": use_optimizations,
                        "MoveWorkspace": False}

        # Set the output workspaces
        set_output_workspaces_on_load_algorithm(load_options, state)

        load_alg = create_managed_non_child_algorithm(load_name, **load_options)
        load_alg.execute()

        return get_workspace_from_algorithm(load_alg, 'SampleScatterWorkspace')