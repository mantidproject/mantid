# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

from mantid.api import *
from mantid.kernel import *
from mantid.simpleapi import *
from SANSILLAutoProcess import needs_loading, needs_processing


class SANSILLParameterScan(DataProcessorAlgorithm):
    """
    Performs treatment for scans along a parameter for D16.
    """
    progress = None
    reduction_type = None
    sample = None
    absorber = None
    container = None
    sensitivity = None
    mask = None
    default_mask = None
    output = None
    output_sens = None
    normalise = None
    thickness = None
    output2D = None
    observable = None
    pixel_y_min = None
    pixel_y_max = None

    def category(self):
        return 'ILL\\SANS;ILL\\Auto'

    def summary(self):
        return 'yfht'

    def seeAlso(self):
        return []

    def name(self):
        return 'SANSILLParameterScan'

    def setUp(self):
        self.sample = self.getPropertyValue('SampleRuns')
        self.absorber = self.getPropertyValue('AbsorberRuns').split(',')
        self.container = self.getPropertyValue('ContainerRuns').split(',')
        self.sensitivity = self.getPropertyValue('SensitivityMaps')
        self.default_mask = self.getPropertyValue('DefaultMaskFile')
        self.mask = self.getPropertyValue('MaskFiles').split(',')
        self.output = self.getPropertyValue('OutputWorkspace')
        self.output_sens = self.getPropertyValue('SensitivityOutputWorkspace')
        self.normalise = self.getPropertyValue('NormaliseBy')
        self.output2D = self.getPropertyValue('Output2D')
        self.observable = self.getPropertyValue('Observable')
        self.pixel_y_min = self.getProperty('PixelYMin').value
        self.pixel_y_max = self.getProperty('PixelYMax').value
        self.progress = Progress(self, start=0.0, end=1.0, nreports=10)

    def PyInit(self):

        self.declareProperty(WorkspaceProperty('OutputWorkspace', '',
                                               direction=Direction.Output),
                             doc='The output workspace containing reduced data.')

        self.declareProperty(WorkspaceProperty('Output2D', '', direction=Direction.Output),
                             doc="The output workspace containing the 2D reduced data.")

        self.declareProperty(MultipleFileProperty('SampleRuns',
                                                  action=FileAction.OptionalLoad,
                                                  extensions=['nxs'],
                                                  allow_empty=False),
                             doc='Sample run(s).')

        self.declareProperty(MultipleFileProperty('AbsorberRuns',
                                                  action=FileAction.OptionalLoad,
                                                  extensions=['nxs']),
                             doc='Absorber (Cd/B4C) run(s).')

        self.declareProperty(MultipleFileProperty('ContainerRuns',
                                                  action=FileAction.OptionalLoad,
                                                  extensions=['nxs']),
                             doc='Empty container run(s).')

        self.setPropertyGroup('SampleRuns', 'Numors')
        self.setPropertyGroup('AbsorberRuns', 'Numors')
        self.setPropertyGroup('ContainerRuns', 'Numors')

        self.declareProperty(MultipleFileProperty('SensitivityMaps',
                                                  action=FileAction.OptionalLoad,
                                                  extensions=['nxs']),
                             doc='File containing the map of relative detector efficiencies.')

        self.declareProperty(FileProperty('DefaultMaskFile', '', action=FileAction.OptionalLoad, extensions=['nxs']),
                             doc='File containing the default mask to be applied to all the detector configurations.')

        self.declareProperty(MultipleFileProperty('MaskFiles',
                                                  action=FileAction.OptionalLoad,
                                                  extensions=['nxs']),
                             doc='File(s) containing the beam stop and other detector mask.')

        self.declareProperty(MatrixWorkspaceProperty('SensitivityOutputWorkspace', '',
                                                     direction=Direction.Output,
                                                     optional=PropertyMode.Optional),
                             doc='The output sensitivity map workspace.')

        self.copyProperties('SANSILLReduction', ['NormaliseBy'])

        self.declareProperty('SampleThickness', 0.1, validator=FloatBoundedValidator(lower=0.),
                             doc='Sample thickness [cm]')

        self.declareProperty('Observable', 'Omega.value', doc='Parameter from the sample logs along which the scan is made')

        self.declareProperty('PixelYMin', 0, validator=IntBoundedValidator(lower=0), doc='Minimal y-index taken in the integration')
        self.declareProperty('PixelYMax', 320, validator=IntBoundedValidator(lower=0), doc='Maximal y-index taken in the integration')

        self.setPropertyGroup('SensitivityMaps', 'Options')
        self.setPropertyGroup('DefaultMaskFile', 'Options')
        self.setPropertyGroup('MaskFiles', 'Options')
        self.setPropertyGroup('NormaliseBy', 'Options')
        self.setPropertyGroup('SampleThickness', 'Options')
        self.setPropertyGroup('Observable', 'Options')
        self.setPropertyGroup('PixelYMin', 'Options')
        self.setPropertyGroup('PixelYMax', 'Options')

    def PyExec(self):

        self.setUp()

        _, load_ws_name = needs_loading(self.sample, "Load")
        Load(Filename=self.sample, OutputWorkspace=load_ws_name)
        ConjoinXRuns(InputWorkspaces=load_ws_name, OutputWorkspace="__joined", SampleLogAsXAxis=self.observable)

        sort_x_axis_output = 'sorted_ws'
        SortXAxis(InputWorkspace="__joined", OutputWorkspace="__" + sort_x_axis_output)

        load_sensitivity, sens_input = needs_loading(self.sensitivity, 'Sensitivity')
        self.progress.report('Loading sensitivity')
        if load_sensitivity:
            LoadNexusProcessed(Filename=self.sensitivity, OutputWorkspace=sens_input)

        load_mask, mask_input = needs_loading(self.mask, "Mask")
        self.progress.report('Loading mask')
        if load_mask:
            LoadNexusProcessed(Filename=self.mask, OutputWorkspace=mask_input)

        load_default_mask, default_mask_input = needs_loading(self.default_mask, "DefaultMask")
        self.progress.report('Loading default mask')
        if load_default_mask:
            LoadNexusProcessed(Filename=self.default_mask, OutputWorkspace=default_mask_input)

        process_absorber, absorber_name = needs_processing(self.absorber, 'Absorber')
        self.progress.report('Processing absorber')
        if process_absorber:
            SANSILLReduction(Run=self.absorber,
                             ProcessAs='Absorber',
                             NormaliseBy=self.normalise,
                             OutputWorkspace=absorber_name)

        process_container, container_name = needs_processing(self.container, 'Container')

        self.progress.report('Processing container')
        if process_container:
            SANSILLReduction(Run=self.container,
                             ProcessAs='Container',
                             OutputWorkspace=container_name,
                             AbsorberInputWorkspace=absorber_name,
                             CacheSolidAngle=True,
                             NormaliseBy=self.normalise)

        # nothing is provided for the sample name, so if it fails to find the already loaded and semi processed workspaces,
        # it will not reduce
        SANSILLReduction(Run="",
                         AbsorberInputWorkspace=absorber_name,
                         ContainerInputWorkspace=container_name,
                         SampleThickness=self.thickness,
                         SensivityMaps=sens_input,
                         SensivityOutputWorkspace=self.output_sens,
                         MaskedInputWorkspace=mask_input,
                         DefaultMaskedInputWorkspace=default_mask_input,
                         NormaliseBy=self.normalise,
                         OutputWorkspace=sort_x_axis_output)

        instrument = mtd["__joined"].getInstrument()
        detector = instrument.getComponentByName("detector")
        if "detector-width" in detector.getParameterNames() or "detector-height" in detector.getParameterNames():
            width = int(detector.getNumberParameter("detector-width")[0])
            height = int(detector.getNumberParameter("detector-height")[0])
        else:
            width, height = 100, 100
            logger.warning("Width or height not found for the instrument. {0}, {1} assumed.".format(width, height))

        grouping = create_detector_grouping(self.pixel_y_min, self.pixel_y_max, width, height)
        GroupDetectors(InputWorkspace=sort_x_axis_output, OutputWorkspace=self.output2D, GroupingPattern=grouping)
        Transpose(InputWorkspace=self.output2D, OutputWorkspace=self.output2D)
        Rebin(InputWorkspace=self.output2D, OutputWorkspace=self.output, Params='1')

        self.setProperty('OutputWorkspace', mtd[self.output])
        self.setProperty('Output2D', mtd[self.output2D])


def create_detector_grouping(x_min, x_max, detector_width, detector_height):
    grouping = []
    for i in range(detector_width):
        grouping.append(str(i*detector_height + x_min) + "-" + str(i*detector_height + x_max - 1))
    grouping = ",".join(grouping)
    return grouping


AlgorithmFactory.subscribe(SANSILLParameterScan)
