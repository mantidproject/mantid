from __future__ import (absolute_import, division, print_function)

from mantid.api import PythonAlgorithm, MatrixWorkspaceProperty, WorkspaceUnitValidator
from mantid.kernel import RebinParamsValidator, EnabledWhenProperty, FloatArrayProperty, Direction, StringListValidator, IntBoundedValidator, FloatBoundedValidator, PropertyCriterion
from mantid.simpleapi import *


class ILLSANSIntegration(PythonAlgorithm):

    def category(self):
        return "ILL\\SANS"

    def summary(self):
        return 'Performs SANS integration based on corrected data.'

    def seeAlso(self):
        return ['Q1DWeighted']

    def name(self):
        return "ILLSANSIntegration"

    def PyInit(self):

        self.declareProperty(MatrixWorkspaceProperty('InputWorkspace', '', direction=Direction.Input,
                             validator=WorkspaceUnitValidator('Wavelength')),
                             doc='The input workspace.')

        self.declareProperty(MatrixWorkspaceProperty('OutputWorkspace', '', direction=Direction.Output),
                             doc='The output workspace.')

        self.declareProperty(FloatArrayProperty('XAxisBinning', validator=RebinParamsValidator(True, True)),
                             doc='The binning of x-axis of the output.')

        self.declareProperty(name='CalculateResolution',
                             defaultValue='MildnerCarpenter',
                             validator=StringListValidator(['MildnerCarpenter', 'None']),
                             doc='Choose to calculate the Q resolution.')

        self.declareProperty(name='NPixelDivision', defaultValue=1, validator=IntBoundedValidator(lower=1),
                             doc='Choose to split each pixel to N x N subpixels.')

        self.declareProperty(name='NumberOfWedges', defaultValue=0, validator=IntBoundedValidator(lower=0),
                             doc='Number of wedges to integrate separately.')

        self.declareProperty(name='WedgeAngle', defaultValue=30., validator=FloatBoundedValidator(lower=0.),
                             doc='Wedge opening angle [degrees].')
        self.setPropertySettings('WedgeAngle', EnabledWhenProperty('NumberOfWedges', PropertyCriterion.IsNotDefault))

        self.declareProperty(name='WedgeOffset', defaultValue=0., validator=FloatBoundedValidator(lower=0.),
                             doc='Wedge offset angle from x+ axis.')
        self.setPropertySettings('WedgeOffset', EnabledWhenProperty('NumberOfWedges', PropertyCriterion.IsNotDefault))

        self.declareProperty(name='AsymmetricWedges', defaultValue=False, doc='Whether to have asymmetric wedges.')
        self.setPropertySettings('AsymmetricWedges', EnabledWhenProperty('NumberOfWedges', PropertyCriterion.IsNotDefault))

        self.declareProperty(name='ErrorWeighting', defaultValue=False, doc='Perform error weighted average in integration.')

        self.setPropertyGroup('NPixelDivision', 'I(Q) Options')
        self.setPropertyGroup('NumberOfWedges', 'I(Q) Options')
        self.setPropertyGroup('WedgeAngle', 'I(Q) Options')
        self.setPropertyGroup('WedgeOffset', 'I(Q) Options')
        self.setPropertyGroup('AsymmetricWedges', 'I(Q) Options')
        self.setPropertyGroup('ErrorWeighting', 'I(Q) Options')

    def PyExec(self):
        pass

# Register algorithm with Mantid
AlgorithmFactory.subscribe(ILLSANSIntegration)
