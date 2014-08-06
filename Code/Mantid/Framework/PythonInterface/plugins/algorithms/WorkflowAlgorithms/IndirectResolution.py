from mantid.simpleapi import *
from mantid.api import *
from mantid.kernel import *
from mantid import config, logger


class IndirectResolution(PythonAlgorithm):

    def PyInit(self):
        self.declareProperty(name='InputFiles', defaultValue='', doc='')
        self.declareProperty(name='OutputWorkspace', defaultValue='', doc='')

        self.declareProperty(name='Instrument', defaultValue='', doc='')
        self.declareProperty(name='Analyser', defaultValue='', doc='')
        self.declareProperty(name='Reflection', defaultValue='', doc='')

        self.declareProperty(name='DetectorRange', defaultValue='', doc='')
        self.declareProperty(name='BackgroundRange', defaultValue='', doc='')
        self.declareProperty(name='RebinString', defaultValue='', doc='')
        self.declareProperty(name='ScaleFactor', defaultValue=1, doc='')

        self.declareProperty(name='Res', defaultValue=True, doc='')
        self.declareProperty(name='Verbose', defaultValue=False, doc='')
        self.declareProperty(name='Plot', defaultValue=False, doc='')
        self.declareProperty(name='Save', defaultValue=False, doc='')

    def PyExec(self):
        from IndirectCommon import StartTime, EndTime
        from IndirectImport import import_mantidplot
        import inelastic_indirect_reducer

        StartTime('IndirectResolution')

        mtd_plot = import_mantidplot()

        input_files = self.getProperty('InputFiles').value.split(',')

        instrument = self.getProperty('Instrument').value
        analyser = self.getProperty('Analyser').value
        reflection = self.getProperty('Reflection').value

        detector_range = self.getProperty('DetectorRange').value.split(',')
        bground = self.getProperty('BackgroundRange').value.split(',')
        rebin_string = self.getProperty('RebinString').value
        scale_factor = self.getProperty('ScaleFactor').value

        res = self.getProperty('Res').value
        verbose = self.getProperty('Verbose').value
        plot = self.getProperty('Plot').value
        save = self.getProperty('Save').value

        reducer = inelastic_indirect_reducer.IndirectReducer()
        reducer.set_instrument_name(instrument)
        reducer.set_detector_range(int(detector_range[0])-1, int(detector_range[1])-1)
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
            logger.error(str(ex))
            EndTime('IndirectResolution')
            return

        icon_ws = reducer.get_result_workspaces()[0]


        if scale_factor != 1:
            Scale(InputWorkspace=icon_ws, OutputWorkspace=icon_ws, Factor=scale_factor)

        if res:
            name = getWSprefix(icon_ws) + 'res'
            CalculateFlatBackground(InputWorkspace=icon_ws, OutputWorkspace=name, StartX=int(bground[0]), EndX=int(bground[1]),
                Mode='Mean', OutputMode='Subtract Background')
            Rebin(InputWorkspace=name, OutputWorkspace=name, Params=rebin_string)

            if save:
                if verbose:
                    logger.notice("Resolution file saved to default save directory.")
                SaveNexusProcessed(InputWorkspace=name, Filename=name+'.nxs')

            if plot:
                mtd_plot.plotSpectrum(name, 0)
            # return name
        else:
            if plot:
                mtd_plot.plotSpectrum(icon_ws, 0)
            # return icon_ws

        EndTime('IndirectResolution')

AlgorithmFactory.subscribe(IndirectResolution)
