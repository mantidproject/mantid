# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)
from mantid.api import PythonAlgorithm, MatrixWorkspaceProperty, WorkspaceUnitValidator, WorkspaceGroupProperty, \
    PropertyMode, MatrixWorkspace, NumericAxis
from mantid.kernel import EnabledWhenProperty, FloatArrayProperty, Direction, StringListValidator, \
    IntBoundedValidator, FloatBoundedValidator, PropertyCriterion, LogicOperator
from mantid.simpleapi import *
from MildnerCarpenter import *
import numpy as np


class SANSILLIntegration(PythonAlgorithm):

    _input_ws = ''
    _output_ws = ''
    _output_type = ''
    _resolution = ''

    def category(self):
        return 'ILL\\SANS'

    def summary(self):
        return 'Performs SANS integration and resolution calculation based on corrected data.'

    def seeAlso(self):
        return ['SANSILLReduction', 'Q1DWeighted', 'Qxy']

    def name(self):
        return 'SANSILLIntegration'

    def validateInputs(self):
        issues = dict()
        if not isinstance(self.getProperty('InputWorkspace').value, MatrixWorkspace):
            issues['InputWorkspace'] = 'The input must be a MatrixWorkspace'
        else:
            run = self.getProperty('InputWorkspace').value.getRun()
            if not run:
                issues['InputWorkspace'] = 'The input workspace does not have a run object attached.'
            else:
                processed = run.getLogData('ProcessedAs').value
                if processed != 'Sample':
                    issues['InputWorkspace'] = 'The input workspace is not processed as sample.'
            instrument = self.getProperty('InputWorkspace').value.getInstrument()
            if not instrument:
                issues['InputWorkspace'] += 'The input workspace does not have an instrument attached.'
        output_type = self.getPropertyValue('OutputType')
        if output_type == 'I(Q)':
            if self.getProperty('NumberOfWedges').value != 0 and not self.getPropertyValue('WedgeWorkspace'):
                issues['WedgeWorkspace'] = 'This is required when NumberOfWedges is not 0.'
        if output_type == 'I(Q)' or output_type == 'I(Phi,Q)':
            binning = self.getProperty('OutputBinning').value
            if len(binning) > 3 and len(binning) % 2 == 0:
                issues['OutputBinning'] = 'If specifying binning explicitly, the array should have odd number of items.'
        if output_type == 'I(Phi,Q)' and self.getProperty('NumberOfWedges').value == 0:
            issues['NumberOfWedges'] = 'This is required for I(Phi,Q) output.'
        return issues

    def PyInit(self):
        self.declareProperty(MatrixWorkspaceProperty('InputWorkspace', '', direction=Direction.Input,
                             validator=WorkspaceUnitValidator('Wavelength')),
                             doc='The input workspace.')

        self.declareProperty(MatrixWorkspaceProperty('OutputWorkspace', '', direction=Direction.Output),
                             doc='The output workspace.')

        self.declareProperty(name='OutputType', defaultValue='I(Q)',
                             validator=StringListValidator(['I(Q)', 'I(Qx,Qy)', 'I(Phi,Q)']),
                             doc='Choose the output type.')

        self.declareProperty(name='CalculateResolution',
                             defaultValue='MildnerCarpenter',
                             validator=StringListValidator(['MildnerCarpenter', 'None']),
                             doc='Choose to calculate the Q resolution.')

        self.declareProperty('ResolutionBasedBinning', False, 'Whether or not to use default binning based on 2*sigma of resolution.')
        self.setPropertySettings('ResolutionBasedBinning',
                                 EnabledWhenProperty('CalculateResolution', PropertyCriterion.IsNotEqualTo, 'None'))

        output_iq = EnabledWhenProperty('OutputType', PropertyCriterion.IsEqualTo, 'I(Q)')
        output_iphiq = EnabledWhenProperty('OutputType', PropertyCriterion.IsEqualTo, 'I(Phi,Q)')
        output_iqxy = EnabledWhenProperty('OutputType', PropertyCriterion.IsEqualTo, 'I(Qx,Qy)')

        self.declareProperty(FloatArrayProperty('OutputBinning'), doc='The Q binning of the output')
        self.setPropertySettings('OutputBinning', EnabledWhenProperty(output_iq, output_iphiq, LogicOperator.Or))

        self.declareProperty('NPixelDivision', 1, IntBoundedValidator(lower=1), 'Number of subpixels to split the pixel (NxN)')
        self.setPropertySettings('NPixelDivision', EnabledWhenProperty(output_iq, output_iphiq, LogicOperator.Or))

        self.declareProperty(name='NumberOfWedges', defaultValue=0, validator=IntBoundedValidator(lower=0),
                             doc='Number of wedges to integrate separately.')
        self.setPropertySettings('NumberOfWedges', EnabledWhenProperty(output_iq, output_iphiq, LogicOperator.Or))

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

        self.setPropertyGroup('OutputBinning', 'I(Q) Options')
        self.setPropertyGroup('NPixelDivision', 'I(Q) Options')
        self.setPropertyGroup('NumberOfWedges', 'I(Q) Options')
        self.setPropertyGroup('WedgeWorkspace', 'I(Q) Options')
        self.setPropertyGroup('WedgeAngle', 'I(Q) Options')
        self.setPropertyGroup('WedgeOffset', 'I(Q) Options')
        self.setPropertyGroup('AsymmetricWedges', 'I(Q) Options')

        self.declareProperty(name='MaxQxy', defaultValue=0., validator=FloatBoundedValidator(lower=0.),
                             doc='Maximum of absolute Qx and Qy.')
        self.setPropertySettings('MaxQxy', output_iqxy)

        self.declareProperty(name='DeltaQ', defaultValue=0., validator=FloatBoundedValidator(lower=0),
                             doc='The dimension of a Qx-Qy cell.')
        self.setPropertySettings('DeltaQ', output_iqxy)

        self.declareProperty(name='IQxQyLogBinning', defaultValue=False,
                             doc='I(Qx, Qy) log binning when binning is not specified.')
        self.setPropertySettings('IQxQyLogBinning', output_iqxy)

        self.setPropertyGroup('MaxQxy', 'I(Qx,Qy) Options')
        self.setPropertyGroup('DeltaQ', 'I(Qx,Qy) Options')
        self.setPropertyGroup('IQxQyLogBinning', 'I(Qx,Qy) Options')

    def PyExec(self):
        self._input_ws = self.getPropertyValue('InputWorkspace')
        self._output_type = self.getPropertyValue('OutputType')
        self._resolution = self.getPropertyValue('CalculateResolution')
        self._output_ws = self.getPropertyValue('OutputWorkspace')
        if self._output_type == 'I(Q)' or self._output_type == 'I(Phi,Q)':
            self._integrate_iq()
        elif self._output_type == 'I(Qx,Qy)':
            self._integrate_iqxy()
        self.setProperty('OutputWorkspace', self._output_ws)

    def _get_iq_binning(self, q_min, q_max, pixel_height, wavelength, l2):
        """
        Returns the OutputBinning string to be used in Q1DWeighted
        """
        if q_min < 0. or q_min >= q_max:
            raise ValueError('qmin must be positive and smaller than qmax. '
                             'Given qmin={0:.2f}, qmax={0:.2f}.'.format(qmin, qmax))
        q_binning = []
        binning = self.getProperty('OutputBinning').value
        use_resolution = self.getProperty('ResolutionBasedBinning').value
        if len(binning) == 0:
            if use_resolution:
                q_binning = self._mildner_carpenter_q_binning(q_min, q_max)
            else:
                q_binning = self._pixel_q_binning(q_min, q_max, pixel_height, wavelength, l2)
        elif len(binning) == 1:
            q_binning = [q_min, binning[0], q_max]
        elif len(binning) == 2:
            if use_resolution:
                q_binning = self._mildner_carpenter_q_binning(binning[0], binning[1])
            else:
                q_binning = self._pixel_q_binning(binning[0], binning[1], pixel_height, wavelength, l2)
        else:
            q_binning = binning
        return q_binning

    def _pixel_q_binning(self, q_min, q_max, pixel_height, wavelength, l2):
        """
        Returns q binning based on the height of a single pixel within the range of q_min and q_max
        """
        bins = []
        q = 0.
        pixels = 1
        while (q < q_max):
            two_theta = np.arctan(pixel_height * pixels / l2)
            q = 4 * np.pi * np.sin(two_theta / 2) / wavelength
            bins.append(q)
            pixels += 1
        q_bin_edges = np.array(bins[:-1])
        q_bin_edges = q_bin_edges[np.where(q_bin_edges > q_min)]
        q_bin_widths = np.diff(q_bin_edges)
        q_binning = np.empty(2 * q_bin_edges.size - 1)
        q_binning[0::2] = q_bin_edges
        q_binning[1::2] = q_bin_widths
        return q_binning

    def _mildner_carpenter_q_binning(self, qmin, qmax):
        """
        Returns q binning such that at each q, bin width is almost 2*sigma
        """
        q = qmin
        result = [qmin]
        while q < qmax:
            bin_width = 2 * self._deltaQ(q)
            result.append(bin_width)
            q += bin_width
            result.append(q)
        return result

    def _setup_mildner_carpenter(self):
        """
        Sets up the mildner carpenter formula based on acquisition type and beam parameters
        """
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
        to_meter = 0.001
        is_rectangular = True
        if 'x' not in source_aperture:
            is_rectangular = False
        if is_rectangular:
            pos1 = source_aperture.find('(') + 1
            pos2 = source_aperture.find('x')
            pos3 = source_aperture.find(')')
            x1 = float(source_aperture[pos1:pos2]) * to_meter
            y1 = float(source_aperture[pos2 + 1:pos3]) *to_meter
            x2 = run.getLogData('Beam.sample_ap_x_or_diam').value * to_meter
            y2 = run.getLogData('Beam.sample_ap_y').value * to_meter
            if is_tof:
                raise RuntimeError('TOF resolution is not supported yet')
            else:
                self._deltaQ = MonochromaticScalarQCartesian(wavelength, delta_wavelength, x1, y1, x2, y2, x3, y3, l1, l2)
        else:
            pos1 = source_aperture.find('(') + 1
            pos3 = source_aperture.find(')')
            r1 = float(source_aperture[pos1:pos3]) * to_meter
            r2 = run.getLogData('Beam.sample_ap_x_or_diam').value * to_meter
            if is_tof:
                raise RuntimeError('TOF resolution is not supported yet')
            else:
                self._deltaQ = MonochromaticScalarQCylindric(wavelength, delta_wavelength, r1, r2, x3, y3, l1, l2)

    def _integrate_iqxy(self):
        """
        Calls Qxy
        """
        max_qxy = self.getProperty('MaxQxy').value
        delta_q = self.getProperty('DeltaQ').value
        log_binning = self.getProperty('IQxQyLogBinning').value
        Qxy(InputWorkspace=self._input_ws, OutputWorkspace=self._output_ws, MaxQxy=max_qxy, DeltaQ=delta_q, IQxQyLogBinning=log_binning)

    def _integrate_iq(self):
        """
        Produces I(Q) or I(Phi,Q) using Q1DWeighted
        """
        if self._resolution == 'MildnerCarpenter':
            self._setup_mildner_carpenter()
        run = mtd[self._input_ws].getRun()
        q_min = run.getLogData('qmin').value
        q_max = run.getLogData('qmax').value
        self.log().information('Using qmin={0:.2f}, qmax={1:.2f}'.format(q_min, q_max))
        pixel_height = run.getLogData('pixel_height').value
        wavelength = run.getLogData('wavelength').value
        l2 = run.getLogData('l2').value
        q_binning = self._get_iq_binning(q_min, q_max, pixel_height, wavelength, l2)
        n_wedges = self.getProperty('NumberOfWedges').value
        pixel_division = self.getProperty('NPixelDivision').value
        if self._output_type == 'I(Q)':
            wedge_ws = self.getPropertyValue('WedgeWorkspace')
            wedge_angle = self.getProperty('WedgeAngle').value
            wedge_offset = self.getProperty('WedgeOffset').value
            asymm_wedges = self.getProperty('AsymmetricWedges').value
            Q1DWeighted(InputWorkspace=self._input_ws, OutputWorkspace=self._output_ws,
                        NumberOfWedges=n_wedges, OutputBinning=q_binning,
                        WedgeWorkspace=wedge_ws, WedgeAngle=wedge_angle, WedgeOffset=wedge_offset,
                        AsymmetricWedges=asymm_wedges, NPixelDivision=pixel_division)
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
        elif self._output_type == 'I(Phi,Q)':
            wedge_ws = '__wedges' + self._input_ws
            iq_ws = '__iq' + self._input_ws
            wedge_angle = 360./n_wedges
            azimuth_axis = NumericAxis.create(n_wedges)
            for i in range(n_wedges):
                azimuth_axis.setValue(i, i * wedge_angle)
            Q1DWeighted(InputWorkspace=self._input_ws, OutputWorkspace=iq_ws, NumberOfWedges=n_wedges,
                        NPixelDivision=pixel_division, OutputBinning=q_binning, WedgeWorkspace=wedge_ws,
                        WedgeAngle=wedge_angle, AsymmetricWedges=True)
            DeleteWorkspace(iq_ws)
            ConjoinSpectra(InputWorkspaces=wedge_ws, OutputWorkspace=self._output_ws)
            mtd[self._output_ws].replaceAxis(1, azimuth_axis)
            DeleteWorkspace(wedge_ws)
            if self._resolution == 'MildnerCarpenter':
                x = mtd[self._output_ws].readX(0)
                mid_x = (x[1:] + x[:-1]) / 2
                res = self._deltaQ(mid_x)
                for i in range(mtd[self._output_ws].getNumberHistograms()):
                    mtd[self._output_ws].setDx(i, res)

# Register algorithm with Mantid
AlgorithmFactory.subscribe(SANSILLIntegration)
