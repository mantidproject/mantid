from __future__ import (absolute_import, division, print_function)

from mantid.kernel import CompositeValidator, Direction, FloatArrayLengthValidator, FloatArrayOrderedPairsValidator, \
    FloatArrayProperty, StringListValidator, IntBoundedValidator
from mantid.api import DataProcessorAlgorithm, MultipleFileProperty, Progress, WorkspaceGroupProperty, FileProperty, FileAction
from mantid.simpleapi import *


class PowderDiffILLDetScanReduction(DataProcessorAlgorithm):
    _progress = None

    def category(self):
        return "ILL\\Diffraction;Diffraction\\Reduction"

    def summary(self):
        return 'Performs powder diffraction data reduction for D2B and D20 (when doing a detector scan).'

    def seeAlso(self):
        return [ "PowderDiffILLReduction" ]

    def name(self):
        return "PowderDiffILLDetScanReduction"

    def validateInputs(self):
        issues = dict()

        if not (self.getProperty("Output2DTubes").value or
                self.getProperty("Output2D").value or
                self.getProperty("Output1D").value):
            issues['Output2DTubes'] = 'No output chosen'
            issues['Output2D'] = 'No output chosen'
            issues['Output1D'] = 'No output chosen'

        return issues

    def PyInit(self):

        self.declareProperty(MultipleFileProperty('Run', extensions=['nxs']),
                             doc='File path of run(s).')

        self.declareProperty(name='NormaliseTo',
                             defaultValue='Monitor',
                             validator=StringListValidator(['None', 'Monitor']),
                             doc='Normalise to monitor, or skip normalisation.')

        self.declareProperty(FileProperty('CalibrationFile', '', action=FileAction.OptionalLoad, extensions=['nxs']),
                             doc='File containing the detector efficiencies.')

        self.declareProperty(name='UseCalibratedData',
                             defaultValue=True,
                             doc='Whether or not to use the calibrated data in the NeXus files.')

        self.declareProperty(name='Output2DTubes',
                             defaultValue=False,
                             doc='Output a 2D workspace of height along tube against tube scattering angle.')

        self.declareProperty(name='Output2D',
                             defaultValue=False,
                             doc='Output a 2D workspace of height along tube against the real scattering angle.')

        self.declareProperty(name='Output1D',
                             defaultValue=True,
                             doc='Output a 1D workspace with counts against scattering angle.')

        self.declareProperty(name='CropNegativeScatteringAngles', defaultValue=True,
                             doc='Whether or not to crop the negative scattering angles.')

        self.declareProperty(FloatArrayProperty(name='HeightRange', values=[],
                                                validator=CompositeValidator([FloatArrayOrderedPairsValidator(),
                                                                              FloatArrayLengthValidator(0, 2)])),
                             doc='A pair of values, comma separated, to give the minimum and maximum height range (in m). If not specified '
                                 'the full height range is used.')

        self.declareProperty(WorkspaceGroupProperty('OutputWorkspace', '',
                                                    direction=Direction.Output),
                             doc='Output workspace containing the reduced data.')

        self.declareProperty(name='InitialMask', defaultValue=20, validator=IntBoundedValidator(lower=0, upper=64),
                             doc='Number of pixels to mask from the bottom and the top of each tube before superposition.')

        self.declareProperty(name='FinalMask', defaultValue=30, validator=IntBoundedValidator(lower=0, upper=70),
                             doc='Number of spectra to mask from the bottom and the top of the result of 2D options.')

        self.declareProperty(name='ComponentsToMask', defaultValue='',
                             doc='Comma separated list of component names to mask, for instance: tube_1, tube_2')

    def _generate_mask(self, n_pix, instrument):
        """
        Generates the DetectorList input for MaskDetectors
        Masks the bottom and top n_pix pixels in each tube
        @param n_pix : Number of pixles to mask from top and bottom of each tube
        @param instrument : Instrument
        """
        mask = ''
        det = instrument.getComponentByName('detectors')
        tube = instrument.getComponentByName('tube_1')
        n_tubes = det.nelements()
        n_pixels = tube.nelements()
        for tube in range(n_tubes):
            start_bottom = tube * n_pixels + 1
            end_bottom = start_bottom + n_pix - 1
            start_top = (tube + 1) * n_pixels - n_pix + 1
            end_top = start_top + n_pix - 1
            mask += str(start_bottom)+'-'+str(end_bottom)+','
            mask += str(start_top)+'-'+str(end_top)+','
        self.log().debug('Preparing to mask with DetectorList='+mask[:-1])
        return mask[:-1]

    def PyExec(self):
        data_type = 'Raw'
        if self.getProperty('UseCalibratedData').value:
            data_type = 'Calibrated'

        self._progress = Progress(self, start=0.0, end=1.0, nreports=6)

        self._progress.report('Loading data')
        input_workspace = LoadAndMerge(Filename=self.getPropertyValue('Run'),
                                       LoaderName='LoadILLDiffraction',
                                       LoaderOptions={'DataType': data_type})
        # We might already have a group, but group just in case
        input_group = GroupWorkspaces(InputWorkspaces=input_workspace)

        instrument = input_group[0].getInstrument()
        supported_instruments = ['D2B', 'D20']
        if instrument.getName() not in supported_instruments:
            self.log.warning('Running for unsupported instrument, use with caution. Supported instruments are: '
                             + str(supported_instruments))
        if instrument.getName() == 'D20':
            if self.getProperty('Output2DTubes').value:
                raise RuntimeError('Output2DTubes is not supported for D20 (1D detector)')
            if self.getProperty('Output2D').value:
                raise RuntimeError('Output2D is not supported for D20 (1D detector)')

        self._progress.report('Normalising to monitor')
        if self.getPropertyValue('NormaliseTo') == 'Monitor':
            input_group = NormaliseToMonitor(InputWorkspace=input_group, MonitorID=0)

        calib_file = self.getPropertyValue('CalibrationFile')
        if calib_file:
            self._progress.report('Applying detector efficiencies')
            LoadNexusProcessed(Filename=calib_file, OutputWorkspace='__det_eff')
            for ws in input_group:
                name = ws.getName()
                ExtractMonitors(InputWorkspace=name, DetectorWorkspace=name)
                ApplyDetectorScanEffCorr(InputWorkspace=name,DetectorEfficiencyWorkspace='__det_eff',OutputWorkspace=name)

        pixels_to_mask = self.getProperty('InitialMask').value
        if pixels_to_mask != 0:
            mask = self._generate_mask(pixels_to_mask, input_group[0].getInstrument())
            for ws in input_group:
                MaskDetectors(Workspace=ws, DetectorList=mask)
        components_to_mask = self.getPropertyValue('ComponentsToMask')
        if components_to_mask:
            for ws in input_group:
                MaskDetectors(Workspace=ws, ComponentList=components_to_mask)

        height_range = ''
        height_range_prop = self.getProperty('HeightRange').value
        if not height_range_prop:
            run = mtd["input_group"].getItem(0).getRun()
            if run.hasProperty("PixelHeight") and run.hasProperty("MaxHeight"):
                pixelHeight = run.getLogData("PixelHeight").value
                maxHeight = run.getLogData("MaxHeight").value
                height_range = str(-maxHeight) + ',' + str(pixelHeight) + ',' + str(maxHeight)
        elif len(height_range_prop) == 2:
            height_range = str(height_range_prop[0]) + ', ' + str(height_range_prop[1])

        output_workspaces = []
        output_workspace_name = self.getPropertyValue('OutputWorkspace')

        mirror_angles = False
        crop_negative = self.getProperty('CropNegativeScatteringAngles').value
        if instrument.hasParameter("mirror_scattering_angles"):
            mirror_angles = instrument.getBoolParameter("mirror_scattering_angles")[0]

        scatterting_angle_tol = 1000.
        final_mask = self.getProperty('FinalMask').value

        self._progress.report('Doing Output2DTubes Option')
        if self.getProperty('Output2DTubes').value:
            output2DtubesName = output_workspace_name + '_2DTubes'
            output2DTubes = SumOverlappingTubes(InputWorkspaces=input_group,
                                                OutputType='2DTubes',
                                                HeightAxis=height_range,
                                                MirrorScatteringAngles=mirror_angles,
                                                CropNegativeScatteringAngles=crop_negative,
                                                ScatteringAngleTolerance=scatterting_angle_tol,
                                                OutputWorkspace=output2DtubesName)
            if final_mask != 0.:
                nSpec = mtd[output2DtubesName].getNumberHistograms()
                mask_list = '0-{0},{1}-{2}'.format(final_mask,nSpec-final_mask,nSpec-1)
                MaskDetectors(Workspace=output2DtubesName,WorkspaceIndexList=mask_list)

            output_workspaces.append(output2DTubes)

        self._progress.report('Doing Output2D Option')
        if self.getProperty('Output2D').value:
            output2DName = output_workspace_name + '_2D'
            output2D = SumOverlappingTubes(InputWorkspaces=input_group,
                                           OutputType='2D',
                                           HeightAxis=height_range,
                                           MirrorScatteringAngles=mirror_angles,
                                           CropNegativeScatteringAngles=crop_negative,
                                           ScatteringAngleTolerance=scatterting_angle_tol,
                                           OutputWorkspace = output2DName)
            if final_mask != 0.:
                nSpec = mtd[output2DName].getNumberHistograms()
                mask_list = '0-{0},{1}-{2}'.format(final_mask,nSpec-final_mask,nSpec-1)
                MaskDetectors(Workspace=output2DName,WorkspaceIndexList=mask_list)

            output_workspaces.append(output2D)

        self._progress.report('Doing Output1D Option')
        if self.getProperty('Output1D').value:
            output1D = SumOverlappingTubes(InputWorkspaces=input_group,
                                           OutputType='1D',
                                           HeightAxis=height_range,
                                           MirrorScatteringAngles=mirror_angles,
                                           CropNegativeScatteringAngles=crop_negative,
                                           OutputWorkspace=output_workspace_name + '_1D',
                                           ScatteringAngleTolerance=scatterting_angle_tol)
            output_workspaces.append(output1D)
        DeleteWorkspace('input_group')

        self._progress.report('Finishing up...')

        GroupWorkspaces(InputWorkspaces=output_workspaces, OutputWorkspace=output_workspace_name)
        self.setProperty('OutputWorkspace', output_workspace_name)

# Register the algorithm with Mantid
AlgorithmFactory.subscribe(PowderDiffILLDetScanReduction)
