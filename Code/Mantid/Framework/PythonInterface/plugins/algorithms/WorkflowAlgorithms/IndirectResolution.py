from mantid.simpleapi import *
from mantid.api import *
from mantid.kernel import *
from mantid import config, logger


class IndirectResolution(DataProcessorAlgorithm):

    def PyInit(self):
        self.declareProperty(FileProperty('InputFiles', '',
            action=FileAction.Load,
            extensions=['raw']))

        self.declareProperty(WorkspaceProperty('OutputWorkspace', '',
            direction=Direction.Output))

        self.declareProperty(name='Instrument', defaultValue='', doc='Instrument used during run',
            validator=StringListValidator(['IRIS', 'OSIRIS', 'TOSCA']))
        self.declareProperty(name='Analyser', defaultValue='', doc='Analyser used during run',
            validator=StringListValidator(['graphite', 'mica', 'fmica']))
        self.declareProperty(name='Reflection', defaultValue='', doc='Reflection used during run',
            validator=StringListValidator(['002', '004', '006']))

        self.declareProperty(IntArrayProperty(name='DetectorRange', values=[0, 1]),
            doc='')
        self.declareProperty(FloatArrayProperty(name='BackgroundRange', values=[0, 0]),
            doc='')

        self.declareProperty(name='RebinString', defaultValue='', doc='')
        self.declareProperty(name='ScaleFactor', defaultValue=1.0, doc='')

        self.declareProperty(name='Res', defaultValue=True, doc='')
        self.declareProperty(name='Verbose', defaultValue=False, doc='')
        self.declareProperty(name='Plot', defaultValue=False, doc='')
        self.declareProperty(name='Save', defaultValue=False, doc='')

    def PyExec(self):
        from IndirectCommon import StartTime, EndTime, getWSprefix
        from IndirectImport import import_mantidplot
        import inelastic_indirect_reducer

        StartTime('IndirectResolution')

        mtd_plot = import_mantidplot()

        input_files = self.getProperty('InputFiles').value.split(',')
        out_ws = self.getPropertyValue('OutputWorkspace')

        instrument = self.getProperty('Instrument').value
        analyser = self.getProperty('Analyser').value
        reflection = self.getProperty('Reflection').value

        detector_range = self.getProperty('DetectorRange').value
        bground = self.getProperty('BackgroundRange').value
        rebin_string = self.getProperty('RebinString').value
        scale_factor = self.getProperty('ScaleFactor').value

        res = self.getProperty('Res').value
        verbose = self.getProperty('Verbose').value
        plot = self.getProperty('Plot').value
        save = self.getProperty('Save').value

        #TODO: Replace with InelasticIndirectReduction algorithm
        reducer = inelastic_indirect_reducer.IndirectReducer()
        reducer.set_instrument_name(instrument)
        reducer.set_detector_range(detector_range[0]-1, detector_range[1]-1)
        for in_file in input_files:
            reducer.append_data_file(in_file)
        parfile = config['instrumentDefinition.directory']
        parfile += instrument +"_"+ analyser +"_"+ reflection +"_Parameters.xml"
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

        if scale_factor != 1.0:
            Scale(InputWorkspace=icon_ws, OutputWorkspace=icon_ws, Factor=scale_factor)

        if res:
            name = getWSprefix(icon_ws) + 'res'
            CalculateFlatBackground(InputWorkspace=icon_ws, OutputWorkspace=name, StartX=bground[0], EndX=bground[1],
                Mode='Mean', OutputMode='Subtract Background')
            Rebin(InputWorkspace=name, OutputWorkspace=name, Params=rebin_string)

            RenameWorkspace(InputWorkspace=name, OutputWorkspace=out_ws)

            if save:
                if verbose:
                    logger.notice("Resolution file saved to default save directory.")
                SaveNexusProcessed(InputWorkspace=out_ws, Filename=name+'.nxs')

            if plot:
                mtd_plot.plotSpectrum(out_ws, 0)

        else:
            RenameWorkspace(InputWorkspace=icon_ws, OutputWorkspace=out_ws)
            if plot:
                mtd_plot.plotSpectrum(out_ws, 0)

        AddSampleLog(Workspace=out_ws, LogName='scale', LogType='String', LogText=str(scale_factor != 1.0))
        if scale_factor != 1.0:
            AddSampleLog(Workspace=out_ws, LogName='scale_factor', LogType='Number', LogText=str(scale_factor))

        if res:
            AddSampleLog(Workspace=out_ws, LogName='back_start', LogType='Number', LogText=bground[0])
            AddSampleLog(Workspace=out_ws, LogName='back_end', LogType='Number', LogText=bground[1])

            rebin_params = rebin_string.split(',')
            if len(rebin_params) == 3:
                AddSampleLog(Workspace=out_ws, LogName='rebin_low', LogType='Number', LogText=rebin_params[0])
                AddSampleLog(Workspace=out_ws, LogName='rebin_width', LogType='Number', LogText=rebin_params[1])
                AddSampleLog(Workspace=out_ws, LogName='rebin_high', LogType='Number', LogText=rebin_params[2])

        self.setProperty('OutputWorkspace', out_ws)

        EndTime('IndirectResolution')

AlgorithmFactory.subscribe(IndirectResolution)
