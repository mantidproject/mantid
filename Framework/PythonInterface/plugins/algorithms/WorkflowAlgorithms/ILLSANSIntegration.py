from __future__ import (absolute_import, division, print_function)

from mantid.api import DataProcessorAlgorithm, MatrixWorkspaceProperty, WorkspaceUnitValidator, WorkspaceGroupProperty, \
    PropertyMode, MatrixWorkspace
from mantid.kernel import EnabledWhenProperty, FloatArrayProperty, Direction, StringListValidator, \
    IntBoundedValidator, FloatBoundedValidator, PropertyCriterion, LogicOperator
from mantid.simpleapi import *
from MildnerCarpenter import *
import math


class ILLSANSIntegration(DataProcessorAlgorithm):

    _input_ws = ''
    _output_ws = ''
    _output_type = ''
    _resolution = ''

    def category(self):
        return 'ILL\\SANS'

    def summary(self):
        return 'Performs SANS integration and resolution calculation based on corrected data.'

    def seeAlso(self):
        return ['Q1DWeighted', 'Qxy', 'ILLSANSReduction']

    def name(self):
        return 'ILLSANSIntegration'

    def validateInputs(self):
        issues = dict()
        if not isinstance(self.getProperty('InputWorkspace').value, MatrixWorkspace):
            issues['InputWorkspace'] = 'The input must be a MatrixWorkspace'
        else:
            run = self.getProperty('InputWorkspace').value.getRun()
            if not run:
                issues['InputWorkspace'] = 'The input workspace does not have a run object attached.'
            instrument = self.getProperty('InputWorkspace').value.getInstrument()
            if not instrument:
                issues['InputWorkspace'] += 'The input workspace does not instrument attached.'
        if self.getPropertyValue('OutputType') == 'I(Q)':
            if self.getProperty('NumberOfWedges').value != 0 and not self.getPropertyValue('WedgeWorkspace'):
                issues['WedgeWorkspace'] = 'This is required when NumberOfWedges is not 0.'
            binning = self.getProperty('OutputBinning').value
            if len(binning) > 3 and len(binning) % 2 == 0:
                issues['OutputBinning'] = 'If specifying binning explicitly, the array should have odd number of items.'
        return issues

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

        self.declareProperty(FloatArrayProperty('OutputBinning'), doc='The Q binning of the output')
        self.setPropertySettings('OutputBinning', output_iq)

        self.declareProperty(name='NumberOfWedges', defaultValue=0, validator=IntBoundedValidator(lower=0),
                             doc='Number of wedges to integrate separately.')
        self.setPropertySettings('NumberOfWedges', output_iq)

        iq_with_wedges = EnabledWhenProperty(output_iq,
                                             EnabledWhenProperty('NumberOfWedges',
                                                                 PropertyCriterion.IsNotDefault), LogicOperator.And)

        self.declareProperty(WorkspaceGroupProperty('WedgeWorkspace', '', direction=Direction.Output,
                                                    optional=PropertyMode.Optional))
        self.setPropertySettings('WedgeWorkspace', iq_with_wedges)

        self.declareProperty(name='WedgeAngle', defaultValue=30., validator=FloatBoundedValidator(lower=0.),
                             doc='Wedge opening angle [degrees].')
        self.setPropertySettings('WedgeAngle', iq_with_wedges)

        self.declareProperty(name='WedgeOffset', defaultValue=0., validator=FloatBoundedValidator(lower=0.),
                             doc='Wedge offset angle from x+ axis.')
        self.setPropertySettings('WedgeOffset', iq_with_wedges)

        self.declareProperty(name='AsymmetricWedges', defaultValue=False, doc='Whether to have asymmetric wedges.')
        self.setPropertySettings('AsymmetricWedges', iq_with_wedges)

        self.declareProperty(name='ErrorWeighting', defaultValue=False,
                             doc='Perform error weighted average in integration.')
        self.setPropertySettings('ErrorWeighting', output_iq)

        self.setPropertyGroup('OutputBinning', 'I(Q) Options')
        self.setPropertyGroup('NumberOfWedges', 'I(Q) Options')
        self.setPropertyGroup('WedgeWorkspace', 'I(Q) Options')
        self.setPropertyGroup('WedgeAngle', 'I(Q) Options')
        self.setPropertyGroup('WedgeOffset', 'I(Q) Options')
        self.setPropertyGroup('AsymmetricWedges', 'I(Q) Options')
        self.setPropertyGroup('ErrorWeighting', 'I(Q) Options')

        self.declareProperty(name='MaxQxy', defaultValue=-0.1, validator=FloatBoundedValidator(lower=0.),
                             doc='Maximum of absolute Qx and Qy.')
        self.setPropertySettings('MaxQxy', output_iqxy)

        self.declareProperty(name='DeltaQ', defaultValue=-0.1, validator=FloatBoundedValidator(lower=0),
                             doc='The dimension of a Qx-Qy cell.')
        self.setPropertySettings('DeltaQ', output_iqxy)

        self.declareProperty(name='IQxQyLogBinning', defaultValue=False,
                             doc='I(Qx, Qy) log binning when binning is not specified.')
        self.setPropertySettings('IQxQyLogBinning', output_iqxy)

        self.setPropertyGroup('MaxQxy', 'I(QxQy) Options')
        self.setPropertyGroup('DeltaQ', 'I(QxQy) Options')
        self.setPropertyGroup('IQxQyLogBinning', 'I(QxQy) Options')

    def PyExec(self):
        self._input_ws = self.getPropertyValue('InputWorkspace')
        self._output_type = self.getPropertyValue('OutputType')
        self._resolution = self.getPropertyValue('CalculateResolution')
        self._output_ws = self.getPropertyValue('OutputWorkspace')
        if self._output_type == 'I(Q)':
            self._integrate_iq()
        elif self._output_type == 'I(QxQy)':
            self._integrate_iqxy()
        self.setProperty('OutputWorkspace', self._output_ws)

    def _get_iq_binning(self, q_min, q_max):
        q_binning = []
        binning = self.getProperty('OutputBinning').value
        if len(binning) == 0:
            if self._resolution == 'None':
                q_binning = [q_min, -0.1, q_max]
            elif self._resolution == 'MildnerCarpenter':
                q_binning = self._mildner_carpenter_q_binning(q_min, q_max)
        elif len(binning) == 1:
            q_binning = [q_min, binning[0], q_max]
        elif len(binning) == 2:
            if self._resolution == 'None':
                q_binning = [binning[0], -0.1, binning[1]]
            elif self._resolution == 'MildnerCarpenter':
                q_binning = self._mildner_carpenter_q_binning(binning[0], binning[1])
        else:
            q_binning = binning
        return q_binning

    def _mildner_carpenter_q_binning(self, qmin, qmax):
        if qmin < 0. or qmin >= qmax:
            raise ValueError('qmin must be positive and smaller than qmax. '
                             'Given qmin={0:.2f}, qmax={0:.2f}'.format(qmin, qmax))
        q = qmin
        result = [qmin]
        while q < qmax:
            bin = 2 * self._deltaQ(q)
            result.append(bin)
            q += bin
            result.append(q)
        return result

    def _setup_mildner_carpenter(self):

        run = mtd[self._input_ws].getRun()
        wavelength = run.getLogData('wavelength').value
        l1 = run.getLogData('collimation.sourceDistance').value
        l2 = run.getLogData('L2').value
        x3 = run.getLogData('pixel_width').value
        y3 = run.getLogData('pixel_height').value
        delta_wavelength = run.getLogData('selector.wavelength_res').value * 0.01
        source_aperture = run.getLogData('collimation.sourceAperture').value
        is_tof = False
        if not run.hasProperty('tof_mode'):
            self.log().information('No TOF flag available, assuming monochromatic.')
        else:
            is_tof = run.getLogData('tof_mode').value == 'TOF'
        is_rectangular = True
        if 'x' not in source_aperture:
            is_rectangular = False
        if is_rectangular:
            pos1 = source_aperture.find('(') + 1
            pos2 = source_aperture.find('x')
            pos3 = source_aperture.find(')')
            x1 = float(source_aperture[pos1:pos2]) * 0.001
            y1 = float(source_aperture[pos2 + 1:pos3]) * 0.001
            x2 = run.getLogData('Beam.sample_ap_x_or_diam').value * 0.001
            y2 = run.getLogData('Beam.sample_ap_y').value * 0.001
            if is_tof:
                raise RuntimeError('TOF resolution is not supported yet')
            else:
                self._deltaQ = MonochromaticScalarQCartesian(wavelength, delta_wavelength, x1, y1, x2, y2, x3, y3, l1, l2)
        else:
            r1 = float(source_aperture) * 0.001
            r2 = run.getLogData('Beam.sample_ap_x_or_diam').value * 0.001
            if is_tof:
                raise RuntimeError('TOF resolution is not supported yet')
            else:
                self._deltaQ = MonochromaticScalarQCylindric(wavelength, delta_wavelength, r1, r2, x3, y3, l1, l2)

    def _integrate_iqxy(self):
        max_qxy = self.getProperty('MaxQxy').value
        delta_q = self.getProperty('DeltaQ').value
        log_binning = self.getProperty('IQxQyLogBinning').value
        Qxy(InputWorkspace=self._input_ws, OutputWorkspace=self._output_ws, MaxQxy=max_qxy, DeltaQ=delta_q, IQxQyLogBinning=log_binning)

    def _integrate_iq(self):
        if self._resolution == 'MildnerCarpenter':
            self._setup_mildner_carpenter()
        run = mtd[self._input_ws].getRun()
        q_min = run.getLogData('qmin').value
        q_max = run.getLogData('qmax').value
        self.log().information('Using qmin={0:.2f}, qmax={1:.2f}'.format(q_min, q_max))
        q_binning = self._get_iq_binning(q_min, q_max)
        self.log().information(str(q_binning))
        n_wedges = self.getProperty('NumberOfWedges').value
        wedge_ws = self.getPropertyValue('WedgeWorkspace')
        wedge_angle = self.getProperty('WedgeAngle').value
        wedge_offset = self.getProperty('WedgeOffset').value
        asymm_wedges = self.getProperty('AsymmetricWedges').value
        error_weighting = self.getProperty('ErrorWeighting').value
        Q1DWeighted(InputWorkspace=self._input_ws, OutputWorkspace=self._output_ws, NumberOfWedges=n_wedges,
                    WedgeWorkspace=wedge_ws, WedgeAngle=wedge_angle, WedgeOffset=wedge_offset, AsymmetricWedges=asymm_wedges,
                    ErrorWeighting=error_weighting, OutputBinning=q_binning)
        if self._resolution == 'MildnerCarpenter':
            x = mtd[self._output_ws].readX(0)
            mid_x = (x[1:] + x[:-1]) / 2
            res = self._deltaQ(mid_x)
            mtd[self._output_ws].setDx(0, res)
            if n_wedges != 0:
                for wedge_ws in mtd[wedge_ws]:
                    wedge_ws.setDx(0, res)
        if n_wedges != 0:
            self.setProperty('WedgeWorkspace', mtd[wedge_ws])

# Register algorithm with Mantid
AlgorithmFactory.subscribe(ILLSANSIntegration)
