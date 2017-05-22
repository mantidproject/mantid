# pylint: disable=too-few-public-methods

""" Crops a selected component from a SANS."""

from __future__ import (absolute_import, division, print_function)
from mantid.kernel import (Direction, StringListValidator)
from mantid.api import (DataProcessorAlgorithm, MatrixWorkspaceProperty, AlgorithmFactory, PropertyMode, Progress)

from sans.common.constants import EMPTY_NAME
from sans.common.enums import DetectorType
from sans.common.general_functions import (create_unmanaged_algorithm, append_to_sans_file_tag)


class SANSCrop(DataProcessorAlgorithm):
    def category(self):
        return 'SANS\\Crop'

    def summary(self):
        return 'Crops a SANS workspaces.'

    def PyInit(self):
        # ----------
        # INPUT
        # ----------
        # Workspace which is to be cropped
        self.declareProperty(MatrixWorkspaceProperty("InputWorkspace", '',
                                                     optional=PropertyMode.Mandatory, direction=Direction.Input),
                             doc='The input workspace')

        # The component, i.e. HAB or LAB
        allowed_detectors = StringListValidator(["LAB", "HAB"])
        self.declareProperty("Component", "LAB", validator=allowed_detectors, direction=Direction.Input,
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
        comp = self.getProperty("Component").value
        is_hab = comp == "HAB"
        if is_hab:
            component = DetectorType.HAB
        else:
            component = DetectorType.LAB

        instrument = workspace.getInstrument()
        instrument_name = instrument.getName().strip()

        # TODO: Can clean up here: The detector bank selection could be made nicer here, but it is currently not
        #                          essential.
        if instrument_name == "SANS2D":
            component = "front-detector" if component is DetectorType.HAB else "rear-detector"
        elif instrument_name == "LOQ":
            component = "HAB" if component is DetectorType.HAB else "main-detector-bank"
        elif instrument_name == "LARMOR":
            component = "DetectorBench"
        else:
            raise RuntimeError("SANSCrop: The instrument {0} is currently not supported.".format(instrument_name))
        return component


# Register algorithm with Mantid
AlgorithmFactory.subscribe(SANSCrop)
