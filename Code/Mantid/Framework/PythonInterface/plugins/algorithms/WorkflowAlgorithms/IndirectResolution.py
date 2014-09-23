from mantid.simpleapi import *
from mantid.api import *
from mantid.kernel import *
from mantid import config, logger


class IndirectResolution(DataProcessorAlgorithm):

    def category(self):
        return 'Workflow\\Inelastic;PythonAlgorithms;Inelastic'

    def summary(self):
        return 'Creates a resolution workspace'

    def PyInit(self):
        self.declareProperty(StringArrayProperty(name='InputFiles'),
                             doc='')

        self.declareProperty(WorkspaceProperty('OutputWorkspace', '',
                             optional=PropertyMode.Optional,
                             direction=Direction.Output))

        self.declareProperty(name='Instrument', defaultValue='',
                             validator=StringListValidator(['IRIS', 'OSIRIS', 'TOSCA']),
                             doc='Instrument used during run')
        self.declareProperty(name='Analyser', defaultValue='',
                             validator=StringListValidator(['graphite', 'mica', 'fmica']),
                             doc='Analyser used during run')
        self.declareProperty(name='Reflection', defaultValue='',
                             validator=StringListValidator(['002', '004', '006']),
                             doc='Reflection used during run')

        self.declareProperty(IntArrayProperty(name='DetectorRange', values=[0, 1]),
                             doc='Range of detetcors to use in resolution calculation')
        self.declareProperty(FloatArrayProperty(name='BackgroundRange', values=[0, 0]),
                             doc='')

        self.declareProperty(name='RebinParam', defaultValue='', doc='')
        self.declareProperty(name='ScaleFactor', defaultValue=1.0, doc='')

        self.declareProperty(name='Res', defaultValue=True, doc='')

        self.declareProperty(name='Verbose', defaultValue=False, doc='')
        self.declareProperty(name='Plot', defaultValue=False, doc='')
        self.declareProperty(name='Save', defaultValue=False, doc='')


    def PyExec(self):
        from IndirectCommon import StartTime, EndTime, getWSprefix
        import inelastic_indirect_reducer

        StartTime('IndirectResolution')
        self._setup()

        # TODO: Replace with InelasticIndirectReduction algorithm
        reducer = inelastic_indirect_reducer.IndirectReducer()
        reducer.set_instrument_name(self._instrument)
        reducer.set_detector_range(self._detector_range[0] - 1, self._detector_range[1] - 1)
        for in_file in self._input_files:
            reducer.append_data_file(in_file)
        parfile = config['instrumentDefinition.directory']
        parfile += self._instrument + "_" + self._analyser + "_" + self._reflection + "_Parameters.xml"
        reducer.set_parameter_file(parfile)
        reducer.set_grouping_policy('All')
        reducer.set_sum_files(True)

        try:
            reducer.reduce()
        except Exception, ex:
            logger.error('IndirectResolution failed with error: ' + str(ex))
            EndTime('IndirectResolution')
            return

        icon_ws = reducer.get_result_workspaces()[0]

        if self._out_ws == "":
            self._out_ws = getWSprefix(icon_ws) + 'res'

        if self._scale_factor != 1.0:
            Scale(InputWorkspace=icon_ws, OutputWorkspace=icon_ws, Factor=self._scale_factor)

        if self._res:
            CalculateFlatBackground(InputWorkspace=icon_ws, OutputWorkspace=self._out_ws,
                                    StartX=self._background[0], EndX=self._background[1],
                                    Mode='Mean', OutputMode='Subtract Background')
            Rebin(InputWorkspace=self._out_ws, OutputWorkspace=self._out_ws, Params=self._rebin_string)
        else:
            RenameWorkspace(InputWorkspace=icon_ws, OutputWorkspace=self._out_ws)

        self._post_process()
        self.setProperty('OutputWorkspace', self._out_ws)

        EndTime('IndirectResolution')


    def _setup(self):
        """
        Gets algorithm properties.
        """

        self._input_files = self.getProperty('InputFiles').value
        self._out_ws = self.getPropertyValue('OutputWorkspace')

        self._instrument = self.getProperty('Instrument').value
        self._analyser = self.getProperty('Analyser').value
        self._reflection = self.getProperty('Reflection').value

        self._detector_range = self.getProperty('DetectorRange').value
        self._background = self.getProperty('BackgroundRange').value
        self._rebin_string = self.getProperty('RebinParam').value
        self._scale_factor = self.getProperty('ScaleFactor').value
        self._res = self.getProperty('Res').value

        self._verbose = self.getProperty('Verbose').value
        self._plot = self.getProperty('Plot').value
        self._save = self.getProperty('Save').value


    def _post_process(self):
        """
        Handles adding logs, saving and plotting.
        """

        use_scale_factor = self._scale_factor == 1.0
        AddSampleLog(Workspace=self._out_ws, LogName='scale', LogType='String', LogText=str(use_scale_factor))
        if use_scale_factor:
            AddSampleLog(Workspace=self._out_ws, LogName='scale_factor', LogType='Number', LogText=str(self._scale_factor))

        if self._res:
            AddSampleLog(Workspace=self._out_ws, LogName='back_start', LogType='Number', LogText=str(self._background[0]))
            AddSampleLog(Workspace=self._out_ws, LogName='back_end', LogType='Number', LogText=str(self._background[1]))

            rebin_params = self._rebin_string.split(',')
            if len(rebin_params) == 3:
                AddSampleLog(Workspace=self._out_ws, LogName='rebin_low', LogType='Number', LogText=rebin_params[0])
                AddSampleLog(Workspace=self._out_ws, LogName='rebin_width', LogType='Number', LogText=rebin_params[1])
                AddSampleLog(Workspace=self._out_ws, LogName='rebin_high', LogType='Number', LogText=rebin_params[2])

        self.setProperty('OutputWorkspace', self._out_ws)

        if self._save:
            if self._verbose:
                logger.notice("Resolution file saved to default save directory.")
            SaveNexusProcessed(InputWorkspace=self._out_ws, Filename=self._out_ws + '.nxs')

        if self._plot:
            from IndirectImport import import_mantidplot
            mtd_plot = import_mantidplot()
            mtd_plot.plotSpectrum(self._out_ws, 0)


AlgorithmFactory.subscribe(IndirectResolution)
