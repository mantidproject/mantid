from __future__ import (absolute_import, division, print_function)

from mantid.api import PythonAlgorithm, MatrixWorkspaceProperty, WorkspaceUnitValidator, WorkspaceGroupProperty, PropertyMode
from mantid.kernel import RebinParamsValidator, EnabledWhenProperty, FloatArrayProperty, \
    Direction, StringListValidator, IntBoundedValidator, FloatBoundedValidator, PropertyCriterion, LogicOperator
from mantid.simpleapi import *


class ILLSANSIntegration(PythonAlgorithm):

    def category(self):
        return 'ILL\\SANS'

    def summary(self):
        return 'Performs SANS integration based on corrected data.'

    def seeAlso(self):
        return ['Q1DWeighted', 'Qxy', 'ILLSANSReduction']

    def name(self):
        return 'ILLSANSIntegration'

    def validateInputs(self):
        issues = dict()
        if self.getProperty('NumberOfWedges').value != 0 and not self.getPropertyValue('WedgeWorkspace'):
            issues['WedgeWorkspace'] = 'This is required when NumberOfWedges is not 0.'

    def PyInit(self):

        self.declareProperty(MatrixWorkspaceProperty('InputWorkspace', '', direction=Direction.Input,
                             validator=WorkspaceUnitValidator('Wavelength')),
                             doc='The input workspace.')

        self.declareProperty(MatrixWorkspaceProperty('OutputWorkspace', '', direction=Direction.Output),
                             doc='The output workspace.')

        self.declareProperty(name='OutputType', defaultValue='I(Q)',
                             validator=StringListValidator(['I(Q)', 'I(QxQy)']),
                             doc='Choose the output type.')

        self.declareProperty(name='CalculateResolution',
                             defaultValue='MildnerCarpenter',
                             validator=StringListValidator(['MildnerCarpenter', 'None']),
                             doc='Choose to calculate the Q resolution.')

        output_iq = EnabledWhenProperty('OutputType', PropertyCriterion.IsEqualTo, 'I(Q)')
        output_iqxy = EnabledWhenProperty('OutputType', PropertyCriterion.IsEqualTo, 'I(QxQy)')

        self.declareProperty(FloatArrayProperty('OutputBinning', validator=RebinParamsValidator(True, True)),
                             doc='The Q binning of the output.')
        self.setPropertySettings('OutputBinning', output_iq)

        self.declareProperty(name='NPixelDivision', defaultValue=1, validator=IntBoundedValidator(lower=1),
                             doc='Choose to split each pixel to N x N subpixels.')
        self.setPropertySettings('NPixelDivision', output_iq)

        self.declareProperty(name='NumberOfWedges', defaultValue=0, validator=IntBoundedValidator(lower=0),
                             doc='Number of wedges to integrate separately.')
        self.setPropertySettings('NumberOfWedges', output_iq)

        iq_with_wedges = EnabledWhenProperty(output_iq, EnabledWhenProperty('NumberOfWedges', PropertyCriterion.IsNotDefault), LogicOperator.And)

        self.declareProperty(WorkspaceGroupProperty('WedgeWorkspace', '', direction=Direction.Output, optional=PropertyMode.Optional))
        self.setPropertySettings('WedgeWorkspace', iq_with_wedges)

        self.declareProperty(name='WedgeAngle', defaultValue=30., validator=FloatBoundedValidator(lower=0.),
                             doc='Wedge opening angle [degrees].')
        self.setPropertySettings('WedgeAngle', iq_with_wedges)

        self.declareProperty(name='WedgeOffset', defaultValue=0., validator=FloatBoundedValidator(lower=0.),
                             doc='Wedge offset angle from x+ axis.')
        self.setPropertySettings('WedgeOffset', iq_with_wedges)

        self.declareProperty(name='AsymmetricWedges', defaultValue=False, doc='Whether to have asymmetric wedges.')
        self.setPropertySettings('AsymmetricWedges', iq_with_wedges)

        self.declareProperty(name='ErrorWeighting', defaultValue=False, doc='Perform error weighted average in integration.')
        self.setPropertySettings('ErrorWeighting', output_iq)

        self.setPropertyGroup('OutputBinning', 'I(Q) Options')
        self.setPropertyGroup('NPixelDivision', 'I(Q) Options')
        self.setPropertyGroup('NumberOfWedges', 'I(Q) Options')
        self.setPropertyGroup('WedgeWorkspace', 'I(Q) Options')
        self.setPropertyGroup('WedgeAngle', 'I(Q) Options')
        self.setPropertyGroup('WedgeOffset', 'I(Q) Options')
        self.setPropertyGroup('AsymmetricWedges', 'I(Q) Options')
        self.setPropertyGroup('ErrorWeighting', 'I(Q) Options')

        self.declareProperty(name='MaxQxy', defaultValue=0., validator=FloatBoundedValidator(lower=0.),
                             doc='Maximum of absolute Qx and Qy.')
        self.setPropertySettings('MaxQxy', output_iqxy)

        self.declareProperty(name='DeltaQ', defaultValue=0., validator=FloatBoundedValidator(lower=0),
                             doc='The dimension of a Qx-Qy cell.')
        self.setPropertySettings('DeltaQ', output_iqxy)

        self.declareProperty(name='IQxQyLogBinning', defaultValue=False,
                             doc='I(Qx, Qy) log binning when binning is not specified.')
        self.setPropertySettings('IQxQyLogBinning', output_iqxy)

        self.setPropertyGroup('MaxQxy', 'I(QxQy) Options')
        self.setPropertyGroup('DeltaQ', 'I(QxQy) Options')
        self.setPropertyGroup('IQxQyLogBinning', 'I(QxQy) Options')

    def PyExec(self):
        pass

# Register algorithm with Mantid
AlgorithmFactory.subscribe(ILLSANSIntegration)
