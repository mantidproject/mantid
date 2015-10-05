#pylint: disable=no-init
from mantid.simpleapi import *
from mantid.api import *
from mantid.kernel import *
from mantid import logger

#pylint: disable=too-many-instance-attributes
class IndirectResolution(DataProcessorAlgorithm):

    _input_files = None
    _out_ws = None
    _instrument = None
    _analyser = None
    _reflection = None
    _detector_range = None
    _background = None
    _rebin_string = None
    _scale_factor = None


    def category(self):
        return 'Workflow\\Inelastic;PythonAlgorithms;Inelastic'


    def summary(self):
        return 'Creates a resolution workspace for an indirect inelastic instrument.'


    def PyInit(self):
        self.declareProperty(StringArrayProperty(name='InputFiles'),
                             doc='Comma seperated list if input files')

        self.declareProperty(name='Instrument', defaultValue='',
                             validator=StringListValidator(['IRIS', 'OSIRIS', 'TOSCA']),
                             doc='Instrument used during run.')
        self.declareProperty(name='Analyser', defaultValue='',
                             validator=StringListValidator(['graphite', 'mica', 'fmica']),
                             doc='Analyser used during run.')
        self.declareProperty(name='Reflection', defaultValue='',
                             validator=StringListValidator(['002', '004', '006']),
                             doc='Reflection used during run.')

        self.declareProperty(IntArrayProperty(name='DetectorRange', values=[0, 1]),
                             doc='Range of detetcors to use in resolution calculation.')
        self.declareProperty(FloatArrayProperty(name='BackgroundRange', values=[0.0, 0.0]),
                             doc='Energy range to use as background.')


        self.declareProperty(name='RebinParam', defaultValue='',
                             doc='Rebinning parameters (min,width,max)')
        self.declareProperty(name='ScaleFactor', defaultValue=1.0,
                             doc='Factor to scale resolution curve by')

        self.declareProperty(WorkspaceProperty('OutputWorkspace', '',
                                               direction=Direction.Output),
                             doc='Output resolution workspace.')


    def PyExec(self):
        self._setup()

        ISISIndirectEnergyTransfer(Instrument=self._instrument,
                                   Analyser=self._analyser,
                                   Reflection=self._reflection,
                                   GroupingMethod='All',
                                   SumFiles=True,
                                   InputFiles=self._input_files,
                                   SpectraRange=self._detector_range,
                                   OutputWorkspace='__et_ws_group')

        icon_ws = mtd['__et_ws_group'].getItem(0).getName()

        if self._scale_factor != 1.0:
            Scale(InputWorkspace=icon_ws,
                  OutputWorkspace=icon_ws,
                  Factor=self._scale_factor)

        CalculateFlatBackground(InputWorkspace=icon_ws,
                                OutputWorkspace=self._out_ws,
                                StartX=self._background[0],
                                EndX=self._background[1],
                                Mode='Mean',
                                OutputMode='Subtract Background')

        Rebin(InputWorkspace=self._out_ws,
              OutputWorkspace=self._out_ws,
              Params=self._rebin_string)

        self._post_process()
        self.setProperty('OutputWorkspace', self._out_ws)


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


    def _post_process(self):
        """
        Handles adding logs, saving and plotting.
        """

        sample_logs = [('res_back_start', self._background[0]),
                       ('res_back_end', self._background[1])]

        if self._scale_factor != 1.0:
            sample_logs.append(('res_scale_factor', self._scale_factor))

        rebin_params = self._rebin_string.split(',')
        if len(rebin_params) == 3:
            sample_logs.append(('rebin_low', rebin_params[0]))
            sample_logs.append(('rebin_width', rebin_params[1]))
            sample_logs.append(('rebin_high', rebin_params[2]))

        AddSampleLogMultiple(Workspace=self._out_ws,
                             LogNames=[log[0] for log in sample_logs],
                             LogValues=[log[1] for log in sample_logs])

        self.setProperty('OutputWorkspace', self._out_ws)


AlgorithmFactory.subscribe(IndirectResolution)
