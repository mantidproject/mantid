from mantid.kernel import *
from mantid.api import *
from mantid.simpleapi import *

class InelasticIndirectReduction(DataProcessorAlgorithm):

    def category(self):
        return "Workflow\\Inelastic;PythonAlgorithms;Inelastic"

    def summary(self):
        return "Runs a reduction for an inelastic indirect geometry instrument."

    def PyInit(self):
        self.declareProperty(WorkspaceProperty('OutputWorkspace', '',
            direction=Direction.Output, optional=PropertyMode.Optional), doc='Optionally override the name for the output workspace')

        self.declareProperty(FileProperty('InputFiles', '', action=FileAction.Load,
            extensions=['nxs', 'raw', 'sav', 'add', 'nxspe', 'n*', 's*']),
            doc='Comma separated list of input files')

        self.declareProperty(name='SumFiles', defaultValue=False, doc='Toggle input file summing or sequential processing')
        self.declareProperty(name='LoadLogs', defaultValue=False, doc='Load sample logs from input files')

        ##TODO: Add other indirect instruments and configurations (only covers ISIS)
        self.declareProperty(name='Instrument', defaultValue='', doc='Instrument used during run',
                validator=StringListValidator(['IRIS', 'OSIRIS', 'TOSCA']))
        self.declareProperty(name='Analyser', defaultValue='', doc='Analyser used during run',
                validator=StringListValidator(['graphite', 'mica', 'fmica', 'diffraction']))
        self.declareProperty(name='Reflection', defaultValue='', doc='Reflection used during run',
                validator=StringListValidator(['002', '004', '006', 'diffspec', 'diffonly']))

        self.declareProperty(WorkspaceProperty('CalibrationWorkspace', '',
            direction=Direction.Input, optional=PropertyMode.Optional), doc='Workspace contining calibration data')

        self.declareProperty(name='DetectorRange', defaultValue='0,1', doc='Comma separated range of detectors to use')
        self.declareProperty(name='BackgroundRange', defaultValue='', doc='')
        self.declareProperty(name='RebinString', defaultValue='', doc='Rebin string parameters')
        self.declareProperty(name='DetailedBalance', defaultValue=-1.0, doc='')
        self.declareProperty(name='ScaleFactor', defaultValue=1.0, doc='')
        self.declareProperty(name='MappingFile', defaultValue='', doc='')
        self.declareProperty(name='Fold', defaultValue=False, doc='')
        self.declareProperty(name='SaveCM1', defaultValue=False, doc='')
        self.declareProperty(name='SaveFormats', defaultValue='', doc='Comma separated list of save formats')

        self.declareProperty(name='Plot', defaultValue='none', doc='Type of plot to output after reduction',
                validator=StringListValidator(['none', 'spectra', 'contour']))

    def PyExec(self):
        from mantid import config, logger
        from IndirectCommon import StartTime, EndTime
        import inelastic_indirect_reducer as irr

        StartTime('InelasticIndirectReduction')

        ## Get parameter values
        out_ws = self.getPropertyValue('OutputWorkspace')
        data_files = self.getPropertyValue('InputFiles').split(',')

        instrument = self.getPropertyValue('Instrument')
        analyser = self.getPropertyValue('Analyser')
        reflection = self.getPropertyValue('Reflection')

        param_file = config['instrumentDefinition.directory'] + instrument + '_' + analyser + '_' + reflection + '_Parameters.xml'

        detector_range = self.getPropertyValue('DetectorRange').split(',')

        background_range = self.getPropertyValue('BackgroundRange').split(',')
        if len(background_range) < 2:
            background_range = None

        calib_ws = self.getProperty('CalibrationWorkspace')
        calib_ws_name = self.getPropertyValue('CalibrationWorkspace')

        detailed_balance = self.getProperty('DetailedBalance').value

        rebin_string = self.getPropertyValue('RebinString')

        scale_factor = self.getPropertyValue('ScaleFactor')

        map_file = self.getPropertyValue('MappingFile')

        save_format_string = self.getPropertyValue('SaveFormats')
        save_formats = save_format_string.split(',')

        ## Validate save format string
        valid_formats = ['nxs', 'spe', 'nxspe', 'ascii', 'aclimax']
        if len(save_format_string) > 0:
            for save_format in save_formats:
                if save_format not in valid_formats:
                    raise ValueError('Save format "' + save_format + '" is not valid.\nValid formats: ' + str(valid_formats))

        ## Setup reducer
        reducer = irr.IndirectReducer()

        reducer.set_rename(True)

        reducer.set_instrument_name(instrument)
        reducer.set_parameter_file(param_file)

        for data_file in data_files:
            reducer.append_data_file(data_file)

        reducer.set_sum_files(self.getProperty('SumFiles').value)

        reducer.set_detector_range(int(detector_range[0])-1, int(detector_range[1])-1)

        use_calib_ws = calib_ws.value != None
        if use_calib_ws:
            logger.debug('Using calibration workspace')
            reducer.set_calibration_workspace(calib_ws.valueAsStr)

        if background_range != None:
            logger.debug('Using background range: ' + str(background_range))
            reducer.set_background(float(background_range[0]), float(background_range[1]))

        ##TODO: There should be a better way to do this
        use_detailed_balance = detailed_balance != -1.0
        if use_detailed_balance:
            logger.debug('Using detailed balance: ' + str(detailed_balance))
            reducer.set_detailed_balance(detailed_balance)

        if rebin_string != "":
            logger.debug('Using rebin string: ' + rebin_string)
            reducer.set_rebin_string(rebin_string)

        use_scale_factor = scale_factor != 1.0
        if use_scale_factor:
            logger.debug('Using scale factor: ' + str(scale_factor))
            reducer.set_scale_factor(scale_factor)

        if map_file != '':
            logger.debug('Using mapping file: ' + str(map_file))
            reducer.set_grouping_policy(map_file)

        reducer.set_fold_multiple_frames(self.getProperty('Fold').value)
        reducer.set_save_to_cm_1(self.getProperty('SaveCM1').value)
        reducer.set_save_formats(save_formats)

        ## Do reduction and get result workspaces
        reducer.reduce()
        ws_list = reducer.get_result_workspaces()

        if len(ws_list) < 1:
            logger.error('Failed to complete reduction')
            return

        ## Add sample logs to output workspace(s)
        for workspace in ws_list:
            AddSampleLog(Workspace=workspace, LogName='use_calib_wokspace', LogType='String', LogText=str(use_calib_ws))
            if use_calib_ws:
                AddSampleLog(Workspace=workspace, LogName='calib_workspace_name', LogType='String', LogText=str(calib_ws_name))

            AddSampleLog(Workspace=workspace, LogName='use_detailed_balance', LogType='String', LogText=str(use_detailed_balance))
            if use_detailed_balance:
                AddSampleLog(Workspace=workspace, LogName='detailed_balance', LogType='Number', LogText=str(detailed_balance))

            AddSampleLog(Workspace=workspace, LogName='use_scale_factor', LogType='String', LogText=str(use_scale_factor))
            if use_scale_factor:
                AddSampleLog(Workspace=workspace, LogName='scale_factor', LogType='Number', LogText=str(scale_factor))

        ## Rename output workspace
        ## Only renames first workspace, but used in this way the reucer should only output one
        use_provided_out_ws = self.getPropertyValue('OutputWorkspace') != ''
        if use_provided_out_ws:
            logger.information('Renaming output workspace ' + str(ws_list[0]) + ' to ' + str(out_ws))
            RenameWorkspace(InputWorkspace=ws_list[0], OutputWorkspace=out_ws)
        else:
            out_ws = ws_list[0]

        self.setProperty('OutputWorkspace', out_ws)

        ## Do plotting
        plot_type = self.getPropertyValue('Plot')

        if plot_type != 'none':
            if plot_type == 'spectra':
                from mantidplot import plotSpectrum
                num_spectra = mtd[out_ws].getNumberHistograms()
                try:
                    plotSpectrum(out_ws, range(0, num_spectra))
                except RuntimeError:
                    logger.notice('Spectrum plotting canceled by user')

            if plot_type == 'contour':
                from mantidplot import importMatrixWorkspace
                plot_workspace = importMatrixWorkspace(out_ws)
                plot_workspace.plotGraph2D()

        EndTime('InelasticIndirectReduction')

# Register algorithm with Mantid
AlgorithmFactory.subscribe(InelasticIndirectReduction)
