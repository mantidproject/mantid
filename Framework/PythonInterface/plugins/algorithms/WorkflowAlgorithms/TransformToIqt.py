#pylint: disable=no-init,too-many-instance-attributes
from __future__ import (absolute_import, division, print_function)
from mantid.simpleapi import *
from mantid.api import (PythonAlgorithm, AlgorithmFactory, MatrixWorkspaceProperty,
                        ITableWorkspaceProperty, PropertyMode, Progress)
from mantid.kernel import Direction, logger
import math


class TransformToIqt(PythonAlgorithm):

    _sample = None
    _resolution = None
    _e_min = None
    _e_max = None
    _e_width = None
    _number_points_per_bin = None
    _parameter_table = None
    _output_workspace = None
    _dry_run = None

    def category(self):
        return "Workflow\\Inelastic;Workflow\\MIDAS"

    def summary(self):
        return 'Transforms an inelastic reduction to I(Q, t)'

    def PyInit(self):
        self.declareProperty(MatrixWorkspaceProperty('SampleWorkspace', '',
                                                     optional=PropertyMode.Mandatory,
                                                     direction=Direction.Input),
                             doc="Name for the sample workspace.")

        self.declareProperty(MatrixWorkspaceProperty('ResolutionWorkspace', '',
                                                     optional=PropertyMode.Mandatory,
                                                     direction=Direction.Input),
                             doc="Name for the resolution workspace.")

        self.declareProperty(name='EnergyMin', defaultValue=-0.5,
                             doc='Minimum energy for fit. Default=-0.5')
        self.declareProperty(name='EnergyMax', defaultValue=0.5,
                             doc='Maximum energy for fit. Default=0.5')
        self.declareProperty(name='BinReductionFactor', defaultValue=10.0,
                             doc='Decrease total number of spectrum points by this ratio through merging of '
                                 'intensities from neighbouring bins. Default=1')

        self.declareProperty(ITableWorkspaceProperty('ParameterWorkspace', '',
                                                     direction=Direction.Output,
                                                     optional=PropertyMode.Optional),
                             doc='Table workspace for saving TransformToIqt properties')

        self.declareProperty(MatrixWorkspaceProperty('OutputWorkspace', '',
                                                     direction=Direction.Output,
                                                     optional=PropertyMode.Optional),
                             doc='Output workspace')

        self.declareProperty(name='DryRun', defaultValue=False,
                             doc='Only calculate and output the parameters')

    def PyExec(self):
        self._setup()

        self._calculate_parameters()

        if not self._dry_run:
            self._transform()

            self._add_logs()

        else:
            skip_prog = Progress(self, start=0.3, end=1.0, nreports=2)
            skip_prog.report('skipping transform')
            skip_prog.report('skipping add logs')
            logger.information('Dry run, will not run TransformToIqt')

        self.setProperty('ParameterWorkspace', self._parameter_table)
        self.setProperty('OutputWorkspace', self._output_workspace)

    def _setup(self):
        """
        Gets algorithm properties.
        """

        from IndirectCommon import getWSprefix

        self._sample = self.getPropertyValue('SampleWorkspace')
        self._resolution = self.getPropertyValue('ResolutionWorkspace')

        self._e_min = self.getProperty('EnergyMin').value
        self._e_max = self.getProperty('EnergyMax').value
        self._number_points_per_bin = self.getProperty('BinReductionFactor').value

        self._parameter_table = self.getPropertyValue('ParameterWorkspace')
        if self._parameter_table == '':
            self._parameter_table = getWSprefix(self._sample) + 'TransformToIqtParameters'

        self._output_workspace = self.getPropertyValue('OutputWorkspace')
        if self._output_workspace == '':
            self._output_workspace = getWSprefix(self._sample) + 'iqt'

        self._dry_run = self.getProperty('DryRun').value

    def validateInputs(self):
        """
        Validate input properties.
        """
        issues = dict()

        e_min = self.getProperty('EnergyMin').value
        e_max = self.getProperty('EnergyMax').value

        # Check for swapped energy values
        if e_min > e_max:
            energy_swapped = 'EnergyMin is greater than EnergyMax'
            issues['EnergyMin'] = energy_swapped
            issues['EnergyMax'] = energy_swapped

        return issues

    def _calculate_parameters(self):
        """
        Calculates the TransformToIqt parameters and saves in a table workspace.
        """
        workflow_prog = Progress(self, start=0.0, end=0.3, nreports=8)
        workflow_prog.report('Croping Workspace')
        CropWorkspace(InputWorkspace=self._sample,
                      OutputWorkspace='__TransformToIqt_sample_cropped',
                      Xmin=self._e_min,
                      Xmax=self._e_max)
        workflow_prog.report('Calculating table properties')
        x_data = mtd['__TransformToIqt_sample_cropped'].readX(0)
        number_input_points = len(x_data) - 1
        num_bins = int(number_input_points / self._number_points_per_bin)
        self._e_width = (abs(self._e_min) + abs(self._e_max)) / num_bins

        workflow_prog.report('Attemping to Access IPF')
        try:
            workflow_prog.report('Access IPF')
            instrument = mtd[self._sample].getInstrument()

            analyserName = instrument.getStringParameter('analyser')[0]
            analyser = instrument.getComponentByName(analyserName)

            if analyser is not None:
                logger.debug('Found %s component in instrument %s, will look for resolution there'
                             % (analyserName, instrument))
                resolution = analyser.getNumberParameter('resolution')[0]
            else:
                logger.debug('No %s component found on instrument %s, will look for resolution in top level instrument'
                             % (analyserName, instrument))
                resolution = instrument.getNumberParameter('resolution')[0]

            logger.information('Got resolution from IPF: %f' % resolution)
            workflow_prog.report('IPF resolution obtained')
        except (AttributeError, IndexError):
            workflow_prog.report('Resorting to Default')
            resolution = 0.0175
            logger.warning('Could not get resolution from IPF, using default value: %f' % (resolution))

        resolution_bins = int(round((2 * resolution) / self._e_width))

        if resolution_bins < 5:
            logger.warning('Resolution curve has <5 points. Results may be unreliable.')

        workflow_prog.report('Creating Parameter table')
        param_table = CreateEmptyTableWorkspace(OutputWorkspace=self._parameter_table)

        workflow_prog.report('Populating Parameter table')
        param_table.addColumn('int', 'SampleInputBins')
        param_table.addColumn('float', 'BinReductionFactor')
        param_table.addColumn('int', 'SampleOutputBins')
        param_table.addColumn('float', 'EnergyMin')
        param_table.addColumn('float', 'EnergyMax')
        param_table.addColumn('float', 'EnergyWidth')
        param_table.addColumn('float', 'Resolution')
        param_table.addColumn('int', 'ResolutionBins')

        param_table.addRow([number_input_points, self._number_points_per_bin, num_bins,
                            self._e_min, self._e_max, self._e_width,
                            resolution, resolution_bins])

        workflow_prog.report('Deleting temp Workspace')
        DeleteWorkspace('__TransformToIqt_sample_cropped')

        self.setProperty('ParameterWorkspace', param_table)

    def _add_logs(self):
        sample_logs = [('iqt_sample_workspace', self._sample),
                       ('iqt_resolution_workspace', self._resolution),
                       ('iqt_binning', '%f,%f,%f' % (self._e_min, self._e_width, self._e_max))]

        log_alg = self.createChildAlgorithm(name='AddSampleLogMultiple', startProgress=0.8,
                                            endProgress=1.0, enableLogging=True)
        log_alg.setProperty('Workspace', self._output_workspace)
        log_alg.setProperty('LogNames',[item[0] for item in sample_logs])
        log_alg.setProperty('LogValues', [item[1] for item in sample_logs])
        log_alg.execute()

    def _transform(self):
        """
        Run TransformToIqt.
        """
        from IndirectCommon import CheckHistZero, CheckHistSame, CheckAnalysers
        trans_prog = Progress(self, start=0.3, end=0.8, nreports=15)
        try:
            CheckAnalysers(self._sample, self._resolution)
        except ValueError:
            # A genuine error the shows that the two runs are incompatible
            raise
        except:
            # Checking could not be performed due to incomplete or no instrument
            logger.warning('Could not check for matching analyser and reflection')

        # Process resolution data
        num_res_hist = CheckHistZero(self._resolution)[0]
        if num_res_hist > 1:
            CheckHistSame(self._sample, 'Sample', self._resolution, 'Resolution')

        rebin_param = str(self._e_min) + ',' + str(self._e_width) + ',' + str(self._e_max)
        trans_prog.report('Rebinning Workspace')
        Rebin(InputWorkspace=self._sample,
              OutputWorkspace='__sam_data',
              Params=rebin_param,
              FullBinsOnly=True)

        # Sample
        trans_prog.report('Rebinning sample')
        Rebin(InputWorkspace='__sam_data',
              OutputWorkspace='__sam_data',
              Params=rebin_param)
        trans_prog.report('Integrating Sample')
        Integration(InputWorkspace='__sam_data',
                    OutputWorkspace='__sam_int')
        trans_prog.report('Converting Sample to data points')
        ConvertToPointData(InputWorkspace='__sam_data',
                           OutputWorkspace='__sam_data')
        trans_prog.report('Extracting FFT spectrum for Sample')
        ExtractFFTSpectrum(InputWorkspace='__sam_data',
                           OutputWorkspace='__sam_fft',
                           FFTPart=2)
        trans_prog.report('Dividing Sample')
        Divide(LHSWorkspace='__sam_fft',
               RHSWorkspace='__sam_int',
               OutputWorkspace='__sam')

        # Resolution
        trans_prog.report('Rebinnig Resolution')
        Rebin(InputWorkspace=self._resolution,
              OutputWorkspace='__res_data',
              Params=rebin_param)
        trans_prog.report('Integrating Resolution')
        Integration(InputWorkspace='__res_data',
                    OutputWorkspace='__res_int')
        trans_prog.report('Converting Resolution to data points')
        ConvertToPointData(InputWorkspace='__res_data',
                           OutputWorkspace='__res_data')
        trans_prog.report('Extractig FFT Resolution spectrum')
        ExtractFFTSpectrum(InputWorkspace='__res_data',
                           OutputWorkspace='__res_fft',
                           FFTPart=2)
        trans_prog.report('Dividing Resolution')
        Divide(LHSWorkspace='__res_fft',
               RHSWorkspace='__res_int',
               OutputWorkspace='__res')

        trans_prog.report('Diving Workspaces')
        Divide(LHSWorkspace='__sam',
               RHSWorkspace='__res',
               OutputWorkspace=self._output_workspace)

        # Cleanup sample workspaces
        trans_prog.report('Deleting Sample temp')
        DeleteWorkspace('__sam_data')
        DeleteWorkspace('__sam_int')
        DeleteWorkspace('__sam_fft')
        DeleteWorkspace('__sam')

        # Crop nonsense values off workspace
        binning = int(math.ceil(mtd[self._output_workspace].blocksize() / 2.0))
        bin_v = mtd[self._output_workspace].dataX(0)[binning]
        trans_prog.report('Cropping output')
        CropWorkspace(InputWorkspace=self._output_workspace,
                      OutputWorkspace=self._output_workspace,
                      XMax=bin_v)

        # Set Y axis unit and label
        mtd[self._output_workspace].setYUnit('')
        mtd[self._output_workspace].setYUnitLabel('Intensity')

        trans_prog.report('Deleting Resolution temp')
        # Clean up resolution workspaces
        DeleteWorkspace('__res_data')
        DeleteWorkspace('__res_int')
        DeleteWorkspace('__res_fft')
        DeleteWorkspace('__res')


# Register algorithm with Mantid
AlgorithmFactory.subscribe(TransformToIqt)
