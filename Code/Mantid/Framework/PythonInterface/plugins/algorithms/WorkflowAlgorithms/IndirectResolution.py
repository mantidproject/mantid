from mantid.simpleapi import *
from mantid.api import *
from mantid.kernel import *
from mantid import logger


class IndirectResolution(DataProcessorAlgorithm):

    def category(self):
        return 'Workflow\\Inelastic;PythonAlgorithms;Inelastic'

    def summary(self):
        return 'Creates a resolution workspace'

    def PyInit(self):
        self.declareProperty(StringArrayProperty(name='InputFiles'),
                             doc='Comma seperated list if input files')

        self.declareProperty(WorkspaceProperty('OutputWorkspace', '',
                             optional=PropertyMode.Optional,
                             direction=Direction.Output),
                             doc='Output resolution workspace (if left blank a name will be gernerated automatically)')

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
        self.declareProperty(FloatArrayProperty(name='BackgroundRange', values=[0.0, 0.0]),
                             doc='Energy range to use as background')

        self.declareProperty(name='RebinParam', defaultValue='',
                             doc='Rebinning parameters (min,width,max)')
        self.declareProperty(name='ScaleFactor', defaultValue=1.0,
                             doc='Factor to scale resolution curve by')
        self.declareProperty(name='Smooth', defaultValue=False,
                             doc='Apply WienerSmooth to resolution')

        self.declareProperty(name='Plot', defaultValue=False, doc='Plot resolution curve')
        self.declareProperty(name='Save', defaultValue=False,
                             doc='Save resolution workspace as a Nexus file')


    def PyExec(self):
        from IndirectCommon import StartTime, EndTime, getWSprefix

        StartTime('IndirectResolution')
        self._setup()

        InelasticIndirectReduction(Instrument=self._instrument,
                                   Analyser=self._analyser,
                                   Reflection=self._reflection,
                                   Grouping='All',
                                   SumFiles=True,
                                   InputFiles=self._input_files,
                                   DetectorRange=self._detector_range,
                                   OutputWorkspace='__icon_ws_group')

        icon_ws = mtd['__icon_ws_group'].getItem(0).getName()

        if self._out_ws == "":
            self._out_ws = getWSprefix(icon_ws) + 'res'

        if self._scale_factor != 1.0:
            Scale(InputWorkspace=icon_ws, OutputWorkspace=icon_ws, Factor=self._scale_factor)

        CalculateFlatBackground(InputWorkspace=icon_ws, OutputWorkspace=self._out_ws,
                                StartX=self._background[0], EndX=self._background[1],
                                Mode='Mean', OutputMode='Subtract Background')

        Rebin(InputWorkspace=self._out_ws, OutputWorkspace=self._out_ws, Params=self._rebin_string)

        if self._smooth:
            WienerSmooth(InputWorkspace=self._out_ws, OutputWorkspace='__smooth_temp')
            CopyLogs(InputWorkspace=self._out_ws, OutputWorkspace='__smooth_temp')
            RenameWorkspace(InputWorkspace='__smooth_temp', OutputWorkspace=self._out_ws)

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
        self._smooth = self.getProperty('Smooth').value

        self._plot = self.getProperty('Plot').value
        self._save = self.getProperty('Save').value


    def _post_process(self):
        """
        Handles adding logs, saving and plotting.
        """

        use_scale_factor = self._scale_factor == 1.0
        AddSampleLog(Workspace=self._out_ws, LogName='scale',
                     LogType='String', LogText=str(use_scale_factor))
        if use_scale_factor:
            AddSampleLog(Workspace=self._out_ws, LogName='scale_factor',
                         LogType='Number', LogText=str(self._scale_factor))

        AddSampleLog(Workspace=self._out_ws, LogName='res_smoothing_applied',
                     LogType='String', LogText=str(self._smooth))

        AddSampleLog(Workspace=self._out_ws, LogName='back_start',
                     LogType='Number', LogText=str(self._background[0]))
        AddSampleLog(Workspace=self._out_ws, LogName='back_end',
                     LogType='Number', LogText=str(self._background[1]))

        rebin_params = self._rebin_string.split(',')
        if len(rebin_params) == 3:
            AddSampleLog(Workspace=self._out_ws, LogName='rebin_low',
                         LogType='Number', LogText=rebin_params[0])
            AddSampleLog(Workspace=self._out_ws, LogName='rebin_width',
                         LogType='Number', LogText=rebin_params[1])
            AddSampleLog(Workspace=self._out_ws, LogName='rebin_high',
                         LogType='Number', LogText=rebin_params[2])

        self.setProperty('OutputWorkspace', self._out_ws)

        if self._save:
            logger.information("Resolution file saved to default save directory.")
            SaveNexusProcessed(InputWorkspace=self._out_ws, Filename=self._out_ws + '.nxs')

        if self._plot:
            from IndirectImport import import_mantidplot
            mtd_plot = import_mantidplot()
            mtd_plot.plotSpectrum(self._out_ws, 0)


AlgorithmFactory.subscribe(IndirectResolution)
