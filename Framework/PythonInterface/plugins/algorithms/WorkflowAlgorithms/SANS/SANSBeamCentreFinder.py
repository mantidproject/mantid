# pylint: disable=too-few-public-methods

""" Finds the beam centre for SANS"""

from __future__ import (absolute_import, division, print_function)
from mantid.kernel import (Direction, StringListValidator)
from mantid.api import (DataProcessorAlgorithm, MatrixWorkspaceProperty, AlgorithmFactory, PropertyMode, Progress)

from sans.common.constants import EMPTY_NAME
from sans.common.enums import DetectorType
from sans.common.general_functions import (create_unmanaged_algorithm, append_to_sans_file_tag)
from sans.algorithm_detail.crop_helper import get_component_name


class SANSBeamCentreFinder(DataProcessorAlgorithm):
    def category(self):
        return 'SANS\\BeamCentreFinder'

    def summary(self):
        return 'Finds the position of the beam centre'

    def PyInit(self):
        # ----------
        # INPUT
        # ----------
        # Workspace which is to be cropped
        self.declareProperty(MatrixWorkspaceProperty("SampleScatter", '',
                                                     optional=PropertyMode.Mandatory, direction=Direction.Input),
                             doc='The sample scatter data')

        self.declareProperty(MatrixWorkspaceProperty("SampleTransmission", '',
                                                     optional=PropertyMode.Mandatory, direction=Direction.Input),
                             doc='The sample transmission data')

        self.declareProperty(MatrixWorkspaceProperty("SampleDirect", '',
                                                     optional=PropertyMode.Mandatory, direction=Direction.Input),
                             doc='The sample direct data')

        self.declareProperty(MatrixWorkspaceProperty("CanTransmission", '',
                                                     optional=PropertyMode.Mandatory, direction=Direction.Input),
                             doc='The sample transmission data')

        # The component, i.e. HAB or LAB
        allowed_detectors = StringListValidator([DetectorType.to_string(DetectorType.LAB),
                                                 DetectorType.to_string(DetectorType.HAB)])
        self.declareProperty("Component", DetectorType.to_string(DetectorType.LAB), validator=allowed_detectors,
                             direction=Direction.Input,
                             doc="The component of the instrument to which we want to crop.")

        self.declareProperty(MatrixWorkspaceProperty("OutputWorkspace", '',
                                                     optional=PropertyMode.Mandatory, direction=Direction.Output),
                             doc='The cropped output workspace')

    def PyExec(self):
        # Get the correct SANS move strategy from the SANSMaskFactory
        workspace = self.getProperty("InputWorkspace").value

        # Component to crop
        component = self._get_component(workspace)

        progress = Progress(self, start=0.0, end=1.0, nreports=2)
        progress.report("Starting to crop component {0}".format(component))

        # Crop to the component
        crop_name = "CropToComponent"
        crop_options = {"InputWorkspace": workspace,
                        "OutputWorkspace": EMPTY_NAME,
                        "ComponentNames": component}
        crop_alg = create_unmanaged_algorithm(crop_name, **crop_options)
        crop_alg.execute()
        output_workspace = crop_alg.getProperty("OutputWorkspace").value

        # Change the file tag and set the output
        append_to_sans_file_tag(output_workspace, "_cropped")
        self.setProperty("OutputWorkspace", output_workspace)
        progress.report("Finished cropping")

    def _get_component(self, workspace):
        component_as_string = self.getProperty("Component").value
        component = DetectorType.from_string(component_as_string)
        return get_component_name(workspace, component)


# Register algorithm with Mantid
#AlgorithmFactory.subscribe(SANSBeamCentreFinder)
