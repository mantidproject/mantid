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
from os import path


class SANSParameterScan(DataProcessorAlgorithm):
    """
    Performs complete treatment of ILL SANS data; instruments D11, D16, D22, D33.
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
    dimensionality = None
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
        return 'SANSParameterScan'

    def setUp(self):
        self.sample = self.getPropertyValue('SampleRuns')
        self.absorber = self.getPropertyValue('AbsorberRuns').split(',')
        self.container = self.getPropertyValue('ContainerRuns').split(',')
        self.sensitivity = self.getPropertyValue('SensitivityMaps').split(',')
        self.default_mask = self.getPropertyValue('DefaultMaskFile')
        self.mask = self.getPropertyValue('MaskFiles').split(',')
        self.output = self.getPropertyValue('OutputWorkspace')
        self.normalise = self.getPropertyValue('NormaliseBy')
        self.output2D = self.getPropertyValue('Output2D')
        self.observable = self.getPropertyValue('Observable')
        self.dimensionality = len(self.sample)
        self.progress = Progress(self, start=0.0, end=1.0, nreports=10 * self.dimensionality)
        self.pixel_y_min = self.getProperty('PixelYMin').value
        self.pixel_y_max = self.getProperty('PixelYMax').value

    def PyInit(self):

        self.declareProperty(WorkspaceGroupProperty('OutputWorkspace', '',
                                                    direction=Direction.Output),
                             doc='The output workspace group containing reduced data.')

        self.declareProperty(WorkspaceProperty('Output2D', '', direction=Direction.Output),
                             doc="The output workspace containing the 2D reduced data.")

        self.declareProperty(MultipleFileProperty('SampleRuns',
                                                  action=FileAction.OptionalLoad,
                                                  extensions=['nxs'],
                                                  allow_empty=True),
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
                             doc='File(s) containing the map of relative detector efficiencies.')

        self.declareProperty(FileProperty('DefaultMaskFile', '', action=FileAction.OptionalLoad, extensions=['nxs']),
                             doc='File containing the default mask to be applied to all the detector configurations.')

        self.declareProperty(MultipleFileProperty('MaskFiles',
                                                  action=FileAction.OptionalLoad,
                                                  extensions=['nxs']),
                             doc='File(s) containing the beam stop and other detector mask.')

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

        Load(Filename=self.sample, OutputWorkspace="__loaded")
        ConjoinXRuns(InputWorkspaces="__loaded", OutputWorkspace="__joined", SampleLogAsXAxis=self.observable)
        SortXAxis(InputWorkspace="__joined", OutputWorkspace="__ws")
        # treat somehow

        instrument = mtd["__joined"].getInstrument()
        detector = instrument.getComponentByName("detector")
        if "detector-width" in detector.getParameterNames() or "detector-height" in detector.getParameterNames():
            width = int(detector.getNumberParameter("detector-width")[0])
            height = int(detector.getNumberParameter("detector-height")[0])
        else:
            width, height = 100, 100
            logger.warning("Width or height not found for the instrument. {0}, {1} assumed.".format(width, height))
        grouping = create_detector_grouping(self.pixel_y_min, self.pixel_y_max, width, height)
        GroupDetectors(InputWorkspace="__ws", OutputWorkspace=self.output2D, GroupingPattern=grouping)
        Rebin(InputWorkspace=self.output2D, OutputWorkspace=self.output, Params='1')


def create_detector_grouping(x_min, x_max, detector_width, detector_height):
    grouping = ''
    for i in range(detector_width):
        grouping += str(i*detector_height + x_min) + "-" + str(i*detector_height + x_max - 1)
        grouping += ","
    if grouping:
        grouping = grouping[:-1]
    return grouping


AlgorithmFactory.subscribe(SANSParameterScan)
