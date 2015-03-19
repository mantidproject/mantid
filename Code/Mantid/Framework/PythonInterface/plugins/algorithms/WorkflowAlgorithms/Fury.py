#pylint: disable=no-init
from mantid.simpleapi import *
from mantid.api import PythonAlgorithm, AlgorithmFactory, MatrixWorkspaceProperty, PropertyMode
from mantid.kernel import Direction, logger
from mantid import config
import math
import os


class Fury(PythonAlgorithm):

    _sample = None
    _resolution = None
    _e_min = None
    _e_max = None
    _e_width = None
    _number_points_per_bin = None
    _parameter_table = None
    _output_workspace = None
    _plot = None
    _save = None
    _dry_run = None

    def category(self):
        return "Workflow\\MIDAS;PythonAlgorithms"

    def PyInit(self):
        self.declareProperty(MatrixWorkspaceProperty('Sample', '',\
                             optional=PropertyMode.Mandatory, direction=Direction.Input),
                             doc="Name for the Sample workspace.")

        self.declareProperty(MatrixWorkspaceProperty('Resolution', '',\
                             optional=PropertyMode.Mandatory, direction=Direction.Input),
                             doc="Name for the Resolution workspace.")

        self.declareProperty(name='EnergyMin', defaultValue=-0.5,
                             doc='Minimum energy for fit. Default=-0.5')
        self.declareProperty(name='EnergyMax', defaultValue=0.5,
                             doc='Maximum energy for fit. Default=0.5')
        self.declareProperty(name='NumBins', defaultValue=1,
                             doc='Decrease total number of spectrum points by this ratio through merging of '
                                 'intensities from neighbouring bins. Default=1')

        self.declareProperty(MatrixWorkspaceProperty('ParameterWorkspace', '',\
                             direction=Direction.Output, optional=PropertyMode.Optional),
                             doc='Table workspace for saving Fury properties')

        self.declareProperty(MatrixWorkspaceProperty('OutputWorkspace', '',\
                             direction=Direction.Output, optional=PropertyMode.Optional),
                             doc='Output workspace')

        self.declareProperty(name='Plot', defaultValue=False,
                             doc='Switch Plot Off/On')
        self.declareProperty(name='Save', defaultValue=False,
                             doc='Switch Save result to nxs file Off/On')
        self.declareProperty(name='DryRun', defaultValue=False,
                             doc='Only calculate and output the parameters')


    def PyExec(self):
        self._setup()

        self._calculate_parameters()

        if not self._dry_run:
            self._fury()

            self._add_logs()

            if self._save:
                workdir = config['defaultsave.directory']
                opath = os.path.join(workdir, self._output_workspace + '.nxs')
                SaveNexusProcessed(InputWorkspace=self._output_workspace, Filename=opath)
                logger.information('Output file : ' + opath)

            if self._plot:
                self._plot_output()
        else:
            logger.information('Dry run, will not run Fury')

        self.setProperty('ParameterWorkspace', self._parameter_table)
        self.setProperty('OutputWorkspace', self._output_workspace)


    def _setup(self):
        """
        Gets algorithm properties.
        """
        from IndirectCommon import getWSprefix

        self._sample = self.getPropertyValue('Sample')
        self._resolution = self.getPropertyValue('Resolution')

        self._e_min = self.getProperty('EnergyMin').value
        self._e_max = self.getProperty('EnergyMax').value
        self._number_points_per_bin = self.getProperty('NumBins').value

        self._parameter_table = self.getPropertyValue('ParameterWorkspace')
        if self._parameter_table == '':
            self._parameter_table = getWSprefix(self._sample) + 'FuryParameters'

        self._output_workspace = self.getPropertyValue('OutputWorkspace')
        if self._output_workspace == '':
            self._output_workspace = getWSprefix(self._sample) + 'iqt'

        self._plot = self.getProperty('Plot').value
        self._save = self.getProperty('Save').value
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
        Calculates the Fury parameters and saves in a table workspace.
        """
        CropWorkspace(InputWorkspace=self._sample, OutputWorkspace='__Fury_sample_cropped',
                      Xmin=self._e_min, Xmax=self._e_max)
        x_data = mtd['__Fury_sample_cropped'].readX(0)
        number_input_points = len(x_data) - 1
        num_bins = number_input_points / self._number_points_per_bin
        self._e_width = (abs(self._e_min) + abs(self._e_max)) / num_bins

        try:
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

        except (AttributeError, IndexError):
            resolution = 0.0175
            logger.warning('Could not get resolution from IPF, using default value: %f' % (
                            resolution))

        resolution_bins = int(round((2 * resolution) / self._e_width))

        if resolution_bins < 5:
            logger.warning('Resolution curve has <5 points. Results may be unreliable.')

        param_table = CreateEmptyTableWorkspace(OutputWorkspace=self._parameter_table)

        param_table.addColumn('int', 'SampleInputBins')
        param_table.addColumn('int', 'NumberBins')
        param_table.addColumn('int', 'SampleOutputBins')
        param_table.addColumn('float', 'EnergyMin')
        param_table.addColumn('float', 'EnergyMax')
        param_table.addColumn('float', 'EnergyWidth')
        param_table.addColumn('float', 'Resolution')
        param_table.addColumn('int', 'ResolutionBins')

        param_table.addRow([number_input_points, self._number_points_per_bin, num_bins,
                            self._e_min, self._e_max, self._e_width,
                            resolution, resolution_bins])

        DeleteWorkspace('__Fury_sample_cropped')

        self.setProperty('ParameterWorkspace', param_table)


    def _plot_output(self):
        """
        Plot output.
        """
        from IndirectImport import import_mantidplot
        mtd_plot = import_mantidplot()

        spectra_range = range(0, mtd[self._output_workspace].getNumberHistograms())

        graph = mtd_plot.plotSpectrum(self._output_workspace, spectra_range)

        layer = graph.activeLayer()
        layer.setScale(mtd_plot.Layer.Left, 0, 1.0)


    def _add_logs(self):
        AddSampleLog(Workspace=self._output_workspace, LogName='fury_resolution_ws',
                     LogType='String', LogText=self._resolution)
        AddSampleLog(Workspace=self._output_workspace, LogName='fury_rebin_emin',
                     LogType='Number', LogText=str(self._e_min))
        AddSampleLog(Workspace=self._output_workspace, LogName='fury_rebin_ewidth',
                     LogType='Number', LogText=str(self._e_width))
        AddSampleLog(Workspace=self._output_workspace, LogName='fury_rebin_emax',
                     LogType='Number', LogText=str(self._e_max))


    def _fury(self):
        """
        Run Fury.
        """
        from IndirectCommon import CheckHistZero, CheckHistSame, CheckAnalysers

        # Process resolution data
        CheckAnalysers(self._sample, self._resolution)
        num_res_hist = CheckHistZero(self._resolution)[0]
        if num_res_hist > 1:
            CheckHistSame(self._sample, 'Sample', self._resolution, 'Resolution')

        rebin_param = str(self._e_min) + ',' + str(self._e_width) + ',' + str(self._e_max)
        Rebin(InputWorkspace=self._sample, OutputWorkspace='__sam_rebin', Params=rebin_param,
              FullBinsOnly=True)

        Rebin(InputWorkspace=self._resolution, OutputWorkspace='__res_data', Params=rebin_param)
        Integration(InputWorkspace='__res_data', OutputWorkspace='__res_int')
        ConvertToPointData(InputWorkspace='__res_data', OutputWorkspace='__res_data')
        ExtractFFTSpectrum(InputWorkspace='__res_data', OutputWorkspace='__res_fft', FFTPart=2)
        Divide(LHSWorkspace='__res_fft', RHSWorkspace='__res_int', OutputWorkspace='__res')

        Rebin(InputWorkspace='__sam_rebin', OutputWorkspace='__sam_data', Params=rebin_param)
        Integration(InputWorkspace='__sam_data', OutputWorkspace='__sam_int')
        ConvertToPointData(InputWorkspace='__sam_data', OutputWorkspace='__sam_data')
        ExtractFFTSpectrum(InputWorkspace='__sam_data', OutputWorkspace='__sam_fft', FFTPart=2)
        Divide(LHSWorkspace='__sam_fft', RHSWorkspace='__sam_int', OutputWorkspace='__sam')

        Divide(LHSWorkspace='__sam', RHSWorkspace='__res', OutputWorkspace=self._output_workspace)

        # Cleanup sample workspaces
        DeleteWorkspace('__sam_rebin')
        DeleteWorkspace('__sam_data')
        DeleteWorkspace('__sam_int')
        DeleteWorkspace('__sam_fft')
        DeleteWorkspace('__sam')

        # Crop nonsense values off workspace
        binning = int(math.ceil(mtd[self._output_workspace].blocksize() / 2.0))
        bin_v = mtd[self._output_workspace].dataX(0)[binning]
        CropWorkspace(InputWorkspace=self._output_workspace, OutputWorkspace=self._output_workspace,
                      XMax=bin_v)

        # Clean up resolution workspaces
        DeleteWorkspace('__res_data')
        DeleteWorkspace('__res_int')
        DeleteWorkspace('__res_fft')
        DeleteWorkspace('__res')


# Register algorithm with Mantid
AlgorithmFactory.subscribe(Fury)
