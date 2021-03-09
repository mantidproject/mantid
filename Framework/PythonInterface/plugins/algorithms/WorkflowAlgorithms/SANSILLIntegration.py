# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.api import PythonAlgorithm, MatrixWorkspaceProperty, WorkspaceUnitValidator, WorkspaceGroupProperty, \
    PropertyMode, MatrixWorkspace, NumericAxis
from mantid.kernel import EnabledWhenProperty, FloatArrayProperty, Direction, StringListValidator, \
    IntBoundedValidator, FloatBoundedValidator, PropertyCriterion, LogicOperator, FloatArrayOrderedPairsValidator, \
    FloatArrayLengthValidator, CompositeValidator
from mantid.simpleapi import *
from MildnerCarpenter import *
import numpy as np


class SANSILLIntegration(PythonAlgorithm):

    _input_ws = ''
    _output_ws = ''
    _output_type = ''
    _resolution = ''
    _masking_criterion = ''
    _lambda_range = []

    def category(self):
        return 'ILL\\SANS'

    def summary(self):
        return 'Performs SANS integration and resolution calculation based on corrected data.'

    def seeAlso(self):
        return ['SANSILLReduction']

    def name(self):
        return 'SANSILLIntegration'

    def validateInputs(self):
        issues = dict()
        if self.getPropertyValue('DefaultQBinning') == 'ResolutionBased' and self.getPropertyValue('CalculateResolution') == 'None':
            issues['CalculateResolution'] = 'Please choose a resolution calculation method if resolution based binning is requested.'
        if not isinstance(self.getProperty('InputWorkspace').value, MatrixWorkspace):
            issues['InputWorkspace'] = 'The input must be a MatrixWorkspace'
        else:
            run = self.getProperty('InputWorkspace').value.getRun()
            if not run:
                issues['InputWorkspace'] = 'The input workspace does not have a run object attached.'
            else:
                if run.hasProperty('ProcessedAs'):
                    processed = run.getLogData('ProcessedAs').value
                    if processed != 'Sample':
                        issues['InputWorkspace'] = 'The input workspace is not processed as sample.'
                else:
                    issues['InputWorkspace'] = 'The input workspace is not processed by SANSILLReduction'
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
                             defaultValue='None',
                             validator=StringListValidator(['MildnerCarpenter', 'None']),
                             doc='Choose to calculate the Q resolution.')

        output_iq = EnabledWhenProperty('OutputType', PropertyCriterion.IsEqualTo, 'I(Q)')
        output_iphiq = EnabledWhenProperty('OutputType', PropertyCriterion.IsEqualTo, 'I(Phi,Q)')
        output_iqxy = EnabledWhenProperty('OutputType', PropertyCriterion.IsEqualTo, 'I(Qx,Qy)')

        self.declareProperty(name='DefaultQBinning', defaultValue='PixelSizeBased',
                             validator=StringListValidator(['PixelSizeBased', 'ResolutionBased']),
                             doc='Choose how to calculate the default Q binning.')
        self.setPropertySettings('DefaultQBinning', EnabledWhenProperty(output_iq, output_iphiq, LogicOperator.Or))

        self.declareProperty(name='BinningFactor', defaultValue=1.,
                             validator=FloatBoundedValidator(lower=0.),
                             doc='Specify a multiplicative factor for default Q binning (pixel or resolution based).')
        self.setPropertySettings('BinningFactor', EnabledWhenProperty(output_iq, output_iphiq, LogicOperator.Or))

        self.declareProperty(FloatArrayProperty('OutputBinning'), doc='The manual Q binning of the output')
        self.setPropertySettings('OutputBinning', EnabledWhenProperty(output_iq, output_iphiq, LogicOperator.Or))

        self.declareProperty('NPixelDivision', 1, IntBoundedValidator(lower=1), 'Number of subpixels to split the pixel (NxN)')
        self.setPropertySettings('NPixelDivision', EnabledWhenProperty(output_iq, output_iphiq, LogicOperator.Or))

        self.declareProperty(name='NumberOfWedges', defaultValue=0, validator=IntBoundedValidator(lower=0),
                             doc='Number of wedges to integrate separately.')
        self.setPropertySettings('NumberOfWedges', EnabledWhenProperty(output_iq, output_iphiq, LogicOperator.Or))

        iq_with_wedges = EnabledWhenProperty(output_iq,
                                             EnabledWhenProperty('NumberOfWedges',
                                                                 PropertyCriterion.IsNotDefault), LogicOperator.And)

        self.declareProperty(WorkspaceGroupProperty('WedgeWorkspace', '', direction=Direction.Output, optional=PropertyMode.Optional),
                             doc='WorkspaceGroup containing I(Q) for each azimuthal wedge.')
        self.setPropertySettings('WedgeWorkspace', iq_with_wedges)

        self.declareProperty(name='WedgeAngle', defaultValue=30., validator=FloatBoundedValidator(lower=0.),
                             doc='Wedge opening angle [degrees].')
        self.setPropertySettings('WedgeAngle', iq_with_wedges)

        self.declareProperty(name='WedgeOffset', defaultValue=0., validator=FloatBoundedValidator(lower=0.),
                             doc='Wedge offset angle from x+ axis.')
        self.setPropertySettings('WedgeOffset', iq_with_wedges)

        self.declareProperty(name='AsymmetricWedges', defaultValue=False, doc='Whether to have asymmetric wedges.')
        self.setPropertySettings('AsymmetricWedges', iq_with_wedges)

        self.setPropertyGroup('DefaultQBinning', 'I(Q) Options')
        self.setPropertyGroup('BinningFactor', 'I(Q) Options')
        self.setPropertyGroup('OutputBinning', 'I(Q) Options')
        self.setPropertyGroup('NPixelDivision', 'I(Q) Options')
        self.setPropertyGroup('NumberOfWedges', 'I(Q) Options')
        self.setPropertyGroup('WedgeWorkspace', 'I(Q) Options')
        self.setPropertyGroup('WedgeAngle', 'I(Q) Options')
        self.setPropertyGroup('WedgeOffset', 'I(Q) Options')
        self.setPropertyGroup('AsymmetricWedges', 'I(Q) Options')

        self.declareProperty(name='MaxQxy', defaultValue=-1.0,
                             validator=FloatBoundedValidator(lower=-1.0),
                             doc='Maximum of absolute Qx and Qy.')
        self.setPropertySettings('MaxQxy', output_iqxy)

        self.declareProperty(name='DeltaQ', defaultValue=-1.0,
                             validator=FloatBoundedValidator(lower=-1.0),
                             doc='The dimension of a Qx-Qy cell.')
        self.setPropertySettings('DeltaQ', output_iqxy)

        self.declareProperty(name='IQxQyLogBinning', defaultValue=False,
                             doc='I(Qx, Qy) log binning when binning is not specified.')
        self.setPropertySettings('IQxQyLogBinning', output_iqxy)

        self.setPropertyGroup('MaxQxy', 'I(Qx,Qy) Options')
        self.setPropertyGroup('DeltaQ', 'I(Qx,Qy) Options')
        self.setPropertyGroup('IQxQyLogBinning', 'I(Qx,Qy) Options')
        self.declareProperty(WorkspaceGroupProperty('PanelOutputWorkspaces', '',
                                                    direction=Direction.Output,
                                                    optional=PropertyMode.Optional),
                             doc='The name of the output workspace group for detector panels.')
        self.setPropertyGroup('PanelOutputWorkspaces', 'I(Q) Options')

        lambda_range_validator = CompositeValidator()
        lambda_range_validator.add(FloatArrayOrderedPairsValidator())
        lambda_range_validator.add(FloatArrayLengthValidator(2))
        self.declareProperty(FloatArrayProperty('WavelengthRange', [1., 10.], validator=lambda_range_validator),
                             doc='Wavelength range [Angstrom] to be used in integration (TOF only).')

    def PyExec(self):
        self._input_ws = self.getPropertyValue('InputWorkspace')
        self._output_type = self.getPropertyValue('OutputType')
        self._resolution = self.getPropertyValue('CalculateResolution')
        self._output_ws = self.getPropertyValue('OutputWorkspace')
        self._lambda_range = self.getProperty('WavelengthRange').value
        is_tof = mtd[self._input_ws].getRun().getLogData('tof_mode').value == 'TOF' # D33 only
        if is_tof:
            cut_input_ws = self._input_ws+'_cut'
            CropWorkspaceRagged(InputWorkspace=self._input_ws,
                                OutputWorkspace=cut_input_ws,
                                XMin=self._lambda_range[0],
                                XMax=self._lambda_range[1])
            self._input_ws = cut_input_ws
            # re-calculate the Q-range after lambda cut
            # TOF is only D33 which has panels
            panel_names = mtd[self._input_ws].getInstrument().getStringParameter('detector_panels')[0].split(',')
            CalculateDynamicRange(Workspace=self._input_ws, ComponentNames=panel_names)
        self._integrate(self._input_ws, self._output_ws)
        self.setProperty('OutputWorkspace', self._output_ws)
        panels_out_ws = self.getPropertyValue('PanelOutputWorkspaces')
        if mtd[self._output_ws].getInstrument().getName() in ['D33', 'D11B', 'D22B'] and panels_out_ws:
            panel_names = mtd[self._output_ws].getInstrument().getStringParameter('detector_panels')[0].split(',')
            panel_outputs = []
            for panel in panel_names:
                in_ws = self._input_ws + '_' + panel
                out_ws = panels_out_ws + '_' + panel
                CropToComponent(InputWorkspace=self._input_ws, OutputWorkspace=in_ws, ComponentNames=panel)
                self._integrate(in_ws, out_ws, panel)
                DeleteWorkspace(in_ws)
                ReplaceSpecialValues(InputWorkspace=out_ws, OutputWorkspace=out_ws, NaNValue=0, NaNError=0)
                panel_outputs.append(out_ws)
            GroupWorkspaces(InputWorkspaces=panel_outputs, OutputWorkspace=panels_out_ws)
            self.setProperty('PanelOutputWorkspaces', mtd[panels_out_ws])
        if is_tof:
            DeleteWorkspace(self._input_ws)

    def _integrate(self, in_ws, out_ws, panel=None):
        if self._output_type == 'I(Q)' or self._output_type == 'I(Phi,Q)':
            self._integrate_iq(in_ws, out_ws, panel)
        elif self._output_type == 'I(Qx,Qy)':
            self._integrate_iqxy(in_ws, out_ws)

    def _get_iq_binning(self, q_min, q_max, pixel_size, wavelength, l2, binning_factor, offset):
        """
        Returns the OutputBinning string to be used in Q1DWeighted
        """
        if q_min < 0. or q_min >= q_max:
            raise ValueError('qmin must be positive and smaller than qmax. '
                             'Given qmin={0:.2f}, qmax={0:.2f}.'.format(q_min, q_max))
        q_binning = []
        binning = self.getProperty('OutputBinning').value
        strategy = self.getPropertyValue('DefaultQBinning')
        if len(binning) == 0:
            if strategy == 'ResolutionBased':
                q_binning = self._mildner_carpenter_q_binning(q_min, q_max, binning_factor)
            else:
                if wavelength != 0:
                    run = mtd[self._input_ws].getRun()
                    instrument = mtd[self._input_ws].getInstrument()
                    if instrument.getName() == "D16" and run.hasProperty("Gamma.value") \
                            and run.getLogData("Gamma.value") != 0:
                        if instrument.hasParameter('detector-width'):
                            pixel_nb = instrument.getNumberParameter('detector-width')[0]
                        else:
                            self.log().warning("Width of the instrument not found. Assuming 320 pixels.")
                            pixel_nb = 320
                        q_binning = self._pixel_q_binning_non_aligned(q_min, q_max, pixel_nb, binning_factor)
                    else:
                        q_binning = self._pixel_q_binning(q_min, q_max, pixel_size * binning_factor, wavelength, l2, offset)
                else:
                    q_binning = self._tof_default_q_binning(q_min, q_max)
        elif len(binning) == 1:
            q_binning = [q_min, binning[0], q_max]
        elif len(binning) == 2:
            if strategy == 'ResolutionBased':
                q_binning = self._mildner_carpenter_q_binning(binning[0], binning[1], binning_factor)
            else:
                if wavelength != 0:
                    q_binning = self._pixel_q_binning(binning[0], binning[1], pixel_size * binning_factor, wavelength, l2, offset)
                else:
                    q_binning = self._tof_default_q_binning(binning[0], binning[1])
        else:
            q_binning = binning
        return q_binning

    def _tof_default_q_binning(self, q_min, q_max):
        """
        Returns default q binning for tof mode
        """
        return [q_min, -0.05, q_max]

    def _pixel_q_binning_non_aligned(self, q_min, q_max, pixel_nb, binning_factor):
        """
        Returns q binning based on q_min, q_max. Used when the detector is not aligned with axis Z.
        """
        step = (q_max - q_min) * binning_factor / pixel_nb
        return [q_min, step, q_max]

    def _pixel_q_binning(self, q_min, q_max, pixel_size, wavelength, l2, offset):
        """
        Returns q binning based on the size of a single pixel within the range of q_min and q_max
        Size is the largest size, i.e. max(height, width)
        """
        bins = []
        q = 0.
        pixels = 1
        while (q < q_max):
            two_theta = np.arctan((pixel_size * pixels + offset) / l2)
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

    def _mildner_carpenter_q_binning(self, qmin, qmax, factor):
        """
        Returns q binning such that at each q, bin width is almost factor*sigma
        """
        q = qmin
        result = [qmin]
        while q < qmax:
            bin_width = factor * self._deltaQ(q)
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
        l1 = run.getLogData('collimation.actual_position').value
        l2 = run.getLogData('L2').value
        instrument = mtd[self._input_ws].getInstrument()
        if instrument.hasParameter("x-pixel-size") and instrument.hasParameter("y-pixel-size"):
            x3 = instrument.getNumberParameter('x-pixel-size')[0] / 1000
            y3 = instrument.getNumberParameter('y-pixel-size')[0] / 1000
        else:
            raise RuntimeError('Unable to calculate resolution, missing pixel size.')
        delta_wavelength = run.getLogData('selector.wavelength_res').value * 0.01
        if run.hasProperty('collimation.sourceAperture'):
            source_aperture = run.getLogData('collimation.sourceAperture').value
        elif run.hasProperty('collimation.ap_size'):
            source_aperture = str(run.getLogData('collimation.ap_size').value)
        else:
            raise RuntimeError('Unable to calculate resolution, missing source aperture size.')
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
            if '(' in source_aperture:
                pos1 = source_aperture.find('(') + 1
                pos3 = source_aperture.find(')')
                source_aperture = source_aperture[pos1:pos3]
            r1 = float(source_aperture) * to_meter
            r2 = run.getLogData('Beam.sample_ap_x_or_diam').value * to_meter
            if is_tof:
                raise RuntimeError('TOF resolution is not supported yet')
            else:
                self._deltaQ = MonochromaticScalarQCylindric(wavelength, delta_wavelength, r1, r2, x3, y3, l1, l2)

    def _integrate_iqxy(self, ws_in, ws_out):
        """
        Calls Qxy
        """
        max_qxy = self.getProperty('MaxQxy').value
        delta_q = self.getProperty('DeltaQ').value
        log_binning = self.getProperty('IQxQyLogBinning').value
        if max_qxy == -1:
            qmax = mtd[ws_in].getRun().getLogData("qmax").value
            max_qxy = qmax * 0.7071 # np.sqrt(2) / 2
            self.log().information("Nothing ptovided for MaxQxy. Using a "
                                   "calculated value: {0}".format(max_qxy))
        if delta_q == -1:
            if log_binning:
                delta_q = max_qxy / 10
            else:
                delta_q = max_qxy / 64
            self.log().information("Nothing provided for DeltaQ. Using a "
                                   "calculated value: {0}".format(delta_q))
        Qxy(InputWorkspace=ws_in, OutputWorkspace=ws_out, MaxQxy=max_qxy,
            DeltaQ=delta_q, IQxQyLogBinning=log_binning)

    def _integrate_iq(self, ws_in, ws_out, panel=None):
        """
        Produces I(Q) or I(Phi,Q) using Q1DWeighted
        """
        if self._resolution == 'MildnerCarpenter':
            self._setup_mildner_carpenter()
        run = mtd[ws_in].getRun()
        q_min_name = 'qmin'
        q_max_name = 'qmax'
        if panel:
            q_min_name += ('_' + panel)
            q_max_name += ('_' + panel)
        q_min = run.getLogData(q_min_name).value
        q_max = run.getLogData(q_max_name).value
        self.log().information('Using qmin={0:.2f}, qmax={1:.2f}'.format(q_min, q_max))
        instrument = mtd[self._input_ws].getInstrument()
        pixel_width = instrument.getNumberParameter('x-pixel-size')[0] / 1000
        pixel_height = instrument.getNumberParameter('y-pixel-size')[0] / 1000

        pixel_size = pixel_height if pixel_height >= pixel_width else pixel_width
        binning_factor = self.getProperty('BinningFactor').value
        wavelength = 0. # for TOF mode there is no wavelength
        if run.hasProperty('wavelength'):
            wavelength = run.getLogData('wavelength').value
        l2 = run.getLogData('l2').value
        beamY = 0.
        if run.hasProperty('BeamCenterY'):
            beamY = run.getLogData('BeamCenterY').value
        q_binning = self._get_iq_binning(q_min, q_max, pixel_size, wavelength, l2, binning_factor, -beamY)
        n_wedges = self.getProperty('NumberOfWedges').value
        pixel_division = self.getProperty('NPixelDivision').value
        gravity = wavelength == 0.
        if self._output_type == 'I(Q)':
            if panel:
                # do not process wedges for panels
                n_wedges = 0
            wedge_ws = self.getPropertyValue('WedgeWorkspace')
            wedge_angle = self.getProperty('WedgeAngle').value
            wedge_offset = self.getProperty('WedgeOffset').value
            asymm_wedges = self.getProperty('AsymmetricWedges').value
            Q1DWeighted(InputWorkspace=ws_in, OutputWorkspace=ws_out,
                        NumberOfWedges=n_wedges, OutputBinning=q_binning,
                        AccountForGravity=gravity, WedgeWorkspace=wedge_ws,
                        WedgeAngle=wedge_angle, WedgeOffset=wedge_offset,
                        AsymmetricWedges=asymm_wedges,
                        NPixelDivision=pixel_division)
            if self._resolution == 'MildnerCarpenter':
                x = mtd[ws_out].readX(0)
                mid_x = (x[1:] + x[:-1]) / 2
                res = self._deltaQ(mid_x)
                mtd[ws_out].setDx(0, res)
                if n_wedges != 0:
                    for wedge in range(n_wedges):
                        mtd[wedge_ws].getItem(wedge).setDx(0, res)
            if n_wedges != 0:
                self.setProperty('WedgeWorkspace', mtd[wedge_ws])
        elif self._output_type == 'I(Phi,Q)':
            wedge_ws = '__wedges' + ws_in
            iq_ws = '__iq' + ws_in
            wedge_angle = 360./n_wedges
            azimuth_axis = NumericAxis.create(n_wedges)
            azimuth_axis.setUnit("Phi")
            for i in range(n_wedges):
                azimuth_axis.setValue(i, i * wedge_angle)
            Q1DWeighted(InputWorkspace=ws_in, OutputWorkspace=iq_ws,
                        NumberOfWedges=n_wedges, NPixelDivision=pixel_division,
                        OutputBinning=q_binning, WedgeWorkspace=wedge_ws,
                        WedgeAngle=wedge_angle, AsymmetricWedges=True,
                        AccountForGravity=gravity)
            DeleteWorkspace(iq_ws)
            ConjoinSpectra(InputWorkspaces=wedge_ws, OutputWorkspace=ws_out)
            mtd[ws_out].replaceAxis(1, azimuth_axis)
            DeleteWorkspace(wedge_ws)
            if self._resolution == 'MildnerCarpenter':
                x = mtd[ws_out].readX(0)
                mid_x = (x[1:] + x[:-1]) / 2
                res = self._deltaQ(mid_x)
                for i in range(mtd[ws_out].getNumberHistograms()):
                    mtd[ws_out].setDx(i, res)


# Register algorithm with Mantid
AlgorithmFactory.subscribe(SANSILLIntegration)
