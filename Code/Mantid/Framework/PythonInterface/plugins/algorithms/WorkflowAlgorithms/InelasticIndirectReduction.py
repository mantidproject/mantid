from mantid.kernel import *
from mantid.api import *
from mantid.simpleapi import *


class InelasticIndirectReduction(DataProcessorAlgorithm):

    def category(self):
        return 'Workflow\\Inelastic;PythonAlgorithms;Inelastic'


    def summary(self):
        return 'Runs a reduction for an inelastic indirect geometry instrument.'


    def PyInit(self):
        self.declareProperty(StringArrayProperty(name='InputFiles'),
                             doc='Comma separated list of input files')

        self.declareProperty(WorkspaceGroupProperty('OutputWorkspace', '',
                             direction=Direction.Output),
                             doc='Workspace group for the resulting workspaces')

        self.declareProperty(name='SumFiles', defaultValue=False, doc='Toggle input file summing or sequential processing')
        self.declareProperty(name='LoadLogs', defaultValue=False, doc='Load sample logs from input files')

        self.declareProperty(name='Instrument', defaultValue='', doc='Instrument used during run',
                             validator=StringListValidator(['IRIS', 'OSIRIS', 'TOSCA', 'TFXA', 'BASIS', 'VISION']))
        self.declareProperty(name='Analyser', defaultValue='', doc='Analyser used during run',
                             validator=StringListValidator(['graphite', 'mica', 'fmica', 'silicon']))
        self.declareProperty(name='Reflection', defaultValue='', doc='Reflection used during run',
                             validator=StringListValidator(['002', '004', '006', '111']))

        self.declareProperty(WorkspaceProperty('CalibrationWorkspace', '',
                             direction=Direction.Input, optional=PropertyMode.Optional), doc='Workspace contining calibration data')

        self.declareProperty(IntArrayProperty(name='DetectorRange', values=[0, 1],
                             validator=IntArrayMandatoryValidator()),
                             doc='Comma separated range of detectors to use')
        self.declareProperty(FloatArrayProperty(name='BackgroundRange'),
                             doc='')

        self.declareProperty(name='RebinString', defaultValue='', doc='Rebin string parameters')
        self.declareProperty(name='DetailedBalance', defaultValue=-1.0, doc='')
        self.declareProperty(name='ScaleFactor', defaultValue=1.0, doc='')
        self.declareProperty(name='Grouping', defaultValue='',
                             doc='Method used to group spectra, can be either: Individual, All, a .map filename or a group workspace name')
        self.declareProperty(name='Fold', defaultValue=False, doc='')
        self.declareProperty(name='SaveCM1', defaultValue=False, doc='')
        self.declareProperty(StringArrayProperty(name='SaveFormats'), doc='Comma separated list of save formats')

        self.declareProperty(name='Plot', defaultValue='none', doc='Type of plot to output after reduction',
                             validator=StringListValidator(['none', 'spectra', 'contour']))


    def PyExec(self):
        from mantid import config, logger
        from IndirectCommon import StartTime, EndTime
        import inelastic_indirect_reducer

        self._setup()

        StartTime('InelasticIndirectReduction')

        # Setup reducer
        reducer = inelastic_indirect_reducer.IndirectReducer()

        reducer.set_rename(True)

        reducer.set_instrument_name(self._instrument)
        reducer.set_parameter_file(self._param_file)

        for data_file in self._data_files:
            reducer.append_data_file(data_file)

        reducer.set_sum_files(self._sum_files)

        reducer.set_detector_range(int(self._detector_range[0]) - 1, int(self._detector_range[1]) - 1)

        self._use_calib_ws = self._calib_ws_name != ''
        if self._use_calib_ws:
            logger.information('Using calibration workspace: %s' % self._calib_ws_name)
            reducer.set_calibration_workspace(self._calib_ws_name)

        if len(self._background_range) == 2:
            logger.debug('Using background range: ' + str(self._background_range))
            reducer.set_background(float(self._background_range[0]), float(self._background_range[1]))

        # TODO: There should be a better way to do this
        self._use_detailed_balance = self._detailed_balance != -1.0
        if self._use_detailed_balance:
            logger.debug('Using detailed balance: ' + str(self._detailed_balance))
            reducer.set_detailed_balance(self._detailed_balance)

        if self._rebin_string != '':
            logger.debug('Using rebin string: ' + self._rebin_string)
            reducer.set_rebin_string(self._rebin_string)

        self._use_scale_factor = self._scale_factor != 1.0
        if self._use_scale_factor:
            logger.debug('Using scale factor: ' + str(self._scale_factor))
            reducer.set_scale_factor(self._scale_factor)

        if self._map_file != '':
            logger.debug('Using mapping file: ' + str(self._map_file))
            reducer.set_grouping_policy(self._map_file)

        reducer.set_fold_multiple_frames(self.getProperty('Fold').value)
        reducer.set_save_to_cm_1(self.getProperty('SaveCM1').value)
        reducer.set_save_formats(self._save_formats)

        # Do reduction and get result workspaces
        reducer.reduce()
        ws_list = reducer.get_result_workspaces()

        self._plot_ws = ws_list[0]

        if len(ws_list) < 1:
            logger.error('Failed to complete reduction')
            return

        # Add sample logs to output workspace(s)
        for workspace in ws_list:
            self._add_ws_logs(workspace)

        # Group output workspaces
        GroupWorkspaces(InputWorkspaces=ws_list, OutputWorkspace=self._out_ws_group)
        self.setProperty('OutputWorkspace', self._out_ws_group)

        # Do plotting
        if self._plot_type != 'none':
            self._plot()

        EndTime('InelasticIndirectReduction')


    def validateInputs(self):
        """
        Validates algorithm properties.
        """
        issues = dict()

        # Validate save format string
        save_formats = self.getProperty('SaveFormats').value
        valid_formats = ['nxs', 'spe', 'nxspe', 'ascii', 'aclimax', 'davegrp']
        invalid_formats = list()
        for save_format in save_formats:
            if save_format not in valid_formats:
                invalid_formats.append(save_format)
        if len(invalid_formats) > 0:
            issues['SaveFormats'] = 'The following save formats are not valid: ' + ','.join(invalid_formats)

        return issues


    def _setup(self):
        """
        Gets and algorithm properties.
        """

        # Get parameter values
        self._out_ws_group = self.getPropertyValue('OutputWorkspace')
        self._data_files = self.getProperty('InputFiles').value

        self._instrument = self.getPropertyValue('Instrument')
        self._analyser = self.getPropertyValue('Analyser')
        self._reflection = self.getPropertyValue('Reflection')

        self._param_file = config['instrumentDefinition.directory'] + self._instrument + '_' + self._analyser + '_' + self._reflection + '_Parameters.xml'

        self._detector_range = self.getProperty('DetectorRange').value
        self._background_range = self.getProperty('BackgroundRange').value

        self._calib_ws_name = self.getPropertyValue('CalibrationWorkspace')

        self._detailed_balance = self.getProperty('DetailedBalance').value
        self._rebin_string = self.getPropertyValue('RebinString')
        self._scale_factor = self.getProperty('ScaleFactor').value
        self._sum_files = self.getProperty('SumFiles').value

        self._map_file = self.getPropertyValue('Grouping')

        self._save_formats = self.getProperty('SaveFormats').value
        self._plot_type = self.getPropertyValue('Plot')


    def _add_ws_logs(self, workspace_name):
        """
        Adds sample logs to a given output workspace.
        """

        AddSampleLog(Workspace=workspace_name, LogName='use_calib_wokspace', LogType='String', LogText=str(self._use_calib_ws))
        if self._use_calib_ws:
            AddSampleLog(Workspace=workspace_name, LogName='calib_workspace_name', LogType='String', LogText=str(self._calib_ws_name))

        AddSampleLog(Workspace=workspace_name, LogName='use_detailed_balance', LogType='String', LogText=str(self._use_detailed_balance))
        if self._use_detailed_balance:
            AddSampleLog(Workspace=workspace_name, LogName='detailed_balance', LogType='Number', LogText=str(self._detailed_balance))

        AddSampleLog(Workspace=workspace_name, LogName='use_scale_factor', LogType='String', LogText=str(self._use_scale_factor))
        if self._use_scale_factor:
            AddSampleLog(Workspace=workspace_name, LogName='scale_factor', LogType='Number', LogText=str(self._scale_factor))


    def _plot(self):
        """
        Plots results.
        """

        if self._plot_type == 'spectra':
            from mantidplot import plotSpectrum
            num_spectra = mtd[self._plot_ws].getNumberHistograms()
            try:
                plotSpectrum(self._plot_ws, range(0, num_spectra))
            except RuntimeError:
                logger.notice('Spectrum plotting canceled by user')

        if self._plot_type == 'contour':
            from mantidplot import importMatrixWorkspace
            plot_workspace = importMatrixWorkspace(self._plot_ws)
            plot_workspace.plotGraph2D()


# Register algorithm with Mantid
AlgorithmFactory.subscribe(InelasticIndirectReduction)
