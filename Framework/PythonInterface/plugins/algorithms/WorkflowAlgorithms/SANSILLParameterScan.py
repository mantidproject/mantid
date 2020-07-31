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
    default_mask = None
    output = None
    normalise = None
    output2D = None
    output_joined = None
    observable = None
    pixel_y_min = None
    pixel_y_max = None
    wavelength = None

    def category(self):
        return 'ILL\\SANS;ILL\\Auto'

    def summary(self):
        return 'Integrate SANS scan data along a parameter'

    def seeAlso(self):
        return []

    def name(self):
        return 'SANSILLParameterScan'

    def validateInputs(self):
        issues = dict()
        if not (self.getPropertyValue('OutputJoinedWorkspace') or self.getPropertyValue("OutputWorkspace")):
            issues["OutputJoinedWorkspace"] = "Please provide either OutputJoinedWorkspace, OutputWorkspace or both."
            issues["OutputWorkspace"] = "Please provide either OutputJoinedWorkspace, OutputWorkspace or both."
        if self.getPropertyValue('PixelYmin') > self.getPropertyValue("PixelYmax"):
            issues["PixelYMin"] = "YMin needs to be lesser than YMax"
            issues["PixelYMax"] = "YMax needs to be greater than YMin"
        return issues

    def setUp(self):
        self.sample = self.getPropertyValue('SampleRuns')
        self.absorber = self.getPropertyValue('AbsorberRuns').replace(',', '+')
        self.container = self.getPropertyValue('ContainerRuns').replace(',', '+')
        self.sensitivity = self.getPropertyValue('SensitivityMap')
        self.default_mask = self.getPropertyValue('DefaultMaskFile')
        self.normalise = self.getPropertyValue('NormaliseBy')
        self.output2D = self.getPropertyValue('OutputWorkspace')
        self.output_joined = self.getPropertyValue('OutputJoinedWorkspace')
        self.observable = self.getPropertyValue('Observable')
        self.pixel_y_min = self.getProperty('PixelYMin').value
        self.pixel_y_max = self.getProperty('PixelYMax').value
        self.wavelength = self.getProperty('Wavelength').value
        self.progress = Progress(self, start=0.0, end=1.0, nreports=10)

    def checkPixelY(self, height):
        if self.pixel_y_max > height:
            self.pixel_y_max = height
            logger.warning("PixelYMax value is too high. Reduced to {0}.".format(self.pixel_y_max))

    def PyInit(self):

        self.declareProperty(WorkspaceProperty('OutputWorkspace', '', direction=Direction.Output,
                                               optional=PropertyMode.Optional),
                             doc="The output workspace containing the 2D reduced data.")

        self.declareProperty(WorkspaceProperty('OutputJoinedWorkspace', '', direction=Direction.Output,
                                               optional=PropertyMode.Optional),
                             doc="The output workspace containing all the reduced data, before grouping.")

        self.declareProperty(MultipleFileProperty('SampleRuns',
                                                  action=FileAction.Load,
                                                  extensions=['nxs']),
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

        self.declareProperty(FileProperty('SensitivityMap', '', action=FileAction.OptionalLoad, extensions=['nxs']),
                             doc='File containing the map of relative detector efficiencies.')

        self.declareProperty(FileProperty('DefaultMaskFile', '', action=FileAction.OptionalLoad, extensions=['nxs']),
                             doc='File containing the default mask to be applied to all the detector configurations.')

        self.copyProperties('SANSILLReduction', ['NormaliseBy'])

        self.declareProperty('Observable', 'Omega.value',
                             doc='Parameter from the sample logs along which the scan is made')

        self.declareProperty('PixelYMin', 140, validator=IntBoundedValidator(lower=0),
                             doc='Minimal y-index taken in the integration')
        self.declareProperty('PixelYMax', 180, validator=IntBoundedValidator(lower=0),
                             doc='Maximal y-index taken in the integration')

        self.declareProperty('Wavelength', 0., validator=FloatBoundedValidator(lower=0.),
                             doc='Wavelength of the experiment. Will try to read Nexus files if not provided.')

        self.setPropertyGroup('SensitivityMap', 'Options')
        self.setPropertyGroup('DefaultMaskFile', 'Options')
        self.setPropertyGroup('NormaliseBy', 'Options')
        self.setPropertyGroup('Observable', 'Options')
        self.setPropertyGroup('PixelYMin', 'Options')
        self.setPropertyGroup('PixelYMax', 'Options')

    def PyExec(self):

        self.setUp()

        _, load_ws_name = needs_loading(self.sample, "Load")
        self.progress.report(7, 'Loading samples')
        LoadAndMerge(Filename=self.sample, OutputWorkspace=load_ws_name + "_grouped",
                     LoaderOptions={"Wavelength": self.wavelength}, startProgress=0, endProgress=0.7)
        ConjoinXRuns(InputWorkspaces=load_ws_name + "_grouped",
                     OutputWorkspace=load_ws_name + "_joined",
                     SampleLogAsXAxis=self.observable, startProgress=0.7, endProgress=0.75)
        mtd[load_ws_name + '_grouped'].delete()

        sort_x_axis_output = load_ws_name + '_sorted' if not self.output_joined else self.output_joined
        SortXAxis(InputWorkspace=load_ws_name + "_joined", OutputWorkspace=sort_x_axis_output,
                  startProgress=0.75, endProgress=0.8)

        if self.observable == "Omega.value":
            mtd[sort_x_axis_output].getAxis(0).setUnit("label").setLabel(self.observable, 'degrees')

        load_sensitivity, sens_input = needs_loading(self.sensitivity, 'Sensitivity')
        self.progress.report('Loading sensitivity')
        if load_sensitivity:
            LoadNexusProcessed(Filename=self.sensitivity, OutputWorkspace=sens_input)

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

        self.progress.report("Reducing data.")
        SANSILLReduction(InputWorkspace=sort_x_axis_output,
                         AbsorberInputWorkspace=absorber_name,
                         ContainerInputWorkspace=container_name,
                         SensitivityInputWorkspace=sens_input,
                         DefaultMaskedInputWorkspace=default_mask_input,
                         NormaliseBy=self.normalise,
                         OutputWorkspace=sort_x_axis_output,
                         startProgress=0.8,
                         endProgress=0.95)

        instrument = mtd[load_ws_name + "_joined"].getInstrument()
        detector = instrument.getComponentByName("detector")
        if "detector-width" in detector.getParameterNames() and "detector-height" in detector.getParameterNames():
            width = int(detector.getNumberParameter("detector-width")[0])
            height = int(detector.getNumberParameter("detector-height")[0])
        else:
            raise RuntimeError('No width or height found for this instrument. Unable to group detectors.')
        mtd[load_ws_name + "_joined"].delete()

        self.checkPixelY(height)
        grouping = create_detector_grouping(self.pixel_y_min, self.pixel_y_max, width, height)

        GroupDetectors(InputWorkspace=sort_x_axis_output,
                       OutputWorkspace=self.output2D,
                       GroupingPattern=grouping,
                       Behaviour="Average")

        if not self.output_joined:
            mtd[sort_x_axis_output].delete()
        else:
            self.setProperty('OutputJoinedWorkspace', mtd[self.output_joined])

        self.progress.report("Convert axis.")
        ConvertSpectrumAxis(InputWorkspace=self.output2D,
                            OutputWorkspace=self.output2D,
                            Target="SignedInPlaneTwoTheta",
                            startProgress=0.9,
                            endProgress=1)

        Transpose(InputWorkspace=self.output2D, OutputWorkspace=self.output2D)

        self.setProperty('OutputWorkspace', mtd[self.output2D])


def create_detector_grouping(y_min, y_max, detector_width, detector_height):
    """
    Create the pixel grouping for the detector. Shape is assumed to be D16's.
    The pixel grouping consists of the vertical columns of pixels of the detector.
    :param y_min: index of the first line to take on each column.
    :param y_max: index of the last line to take on each column.
    :param detector_width: the total number of column of pixel on the detector.
    :param detector_height: the total number of lines of pixel on the detector.
    """
    grouping = []
    for i in range(detector_width):
        grouping.append(str(i * detector_height + y_min) + "-" + str(i * detector_height + y_max - 1))
    grouping = ",".join(grouping)
    return grouping


AlgorithmFactory.subscribe(SANSILLParameterScan)
