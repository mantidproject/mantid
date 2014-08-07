from mantid.kernel import *
from mantid.api import *
from mantid.simpleapi import *

class InelasticIndirectReduction(DataProcessorAlgorithm):

    def category(self):
        return "Workflow\\Inelastic;PythonAlgorithms;Inelastic"

    def summary(self):
        return ""  ##TODO

    def PyInit(self):
        self.declareProperty(WorkspaceProperty('OutputWorkspace', '',
            direction=Direction.Output))

        self.declareProperty(FileProperty('InputFiles', '', action=FileAction.Load,
            extensions=['nxs', 'raw', 'sav', 'add', 'nxspe', 'n*', 's*']))

        self.declareProperty(name='SumFiles', defaultValue=False, doc='')
        self.declareProperty(name='LoadLogs', defaultValue=False, doc='')

        self.declareProperty(name='Instrument', defaultValue='', doc='',
                validator=StringListValidator(['IRIS', 'OSIRIS', 'TOSCA']))
        self.declareProperty(name='Analyser', defaultValue='', doc='',
                validator=StringListValidator(['graphite', 'mica', 'fmica', 'diffraction']))
        self.declareProperty(name='Reflection', defaultValue='', doc='',
                validator=StringListValidator(['002', '004', '006', 'diffspec', 'diffonly']))

        self.declareProperty(WorkspaceProperty('CalibrationWorkspace', '',
            direction=Direction.Input, optional=PropertyMode.Optional))

        self.declareProperty(name='DetectorRange', defaultValue='0,1', doc='')
        self.declareProperty(name='BackgroundRange', defaultValue='', doc='')
        self.declareProperty(name='RebinString', defaultValue='', doc='')
        self.declareProperty(name='DetailedBalance', defaultValue=-1.0, doc='')
        self.declareProperty(name='ScaleFactor', defaultValue=1.0, doc='')
        self.declareProperty(name='MappingFile', defaultValue='', doc='')
        self.declareProperty(name='Fold', defaultValue=False, doc='')
        self.declareProperty(name='SaveCM1', defaultValue=False, doc='')

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

        detailed_balance = self.getProperty('DetailedBalance').value
    
        rebin_string = self.getPropertyValue('RebinString')

        scale_factor = self.getPropertyValue('ScaleFactor')

        map_file = self.getPropertyValue('MappingFile')

        ## Setup reducer
        reducer = irr.IndirectReducer()

        reducer.set_rename(False)

        reducer.set_instrument_name(instrument)
        reducer.set_parameter_file(param_file)

        for data_file in data_files:
            reducer.append_data_file(data_file)

        reducer.set_sum_files(self.getProperty('SumFiles').value)

        reducer.set_detector_range(int(detector_range[0])-1, int(detector_range[1])-1)

        if calib_ws.value != None:
            logger.debug('Using calibration workspace')
            reducer.set_calibration_workspace(calib_ws.valueAsStr)

        if background_range != None:
            logger.debug('Using background range: ' + str(background_range))
            reducer.set_background(float(background_range[0]), float(background_range[1]))

        if detailed_balance != -1.0:
            logger.debug('Using detailed balance: ' + str(detailed_balance))
            reducer.set_detailed_balance(detailed_balance)

        if rebin_string != "":
            logger.debug('Using rebin string: ' + rebin_string)
            reducer.set_rebin_string(rebin_string)

        if scale_factor != 1.0:
            logger.debug('Using scale factor: ' + str(scale_factor))
            reducer.set_scale_factor(scale_factor)

        if map_file != '':
            logger.debug('Using mapping file: ' + str(map_file))
            reducer.set_grouping_policy(map_file)

        reducer.set_fold_multiple_frames(self.getProperty('Fold').value)
        reducer.set_save_to_cm_1(self.getProperty('SaveCM1').value)

        ## Do reduction and get result workspaces
        reducer.reduce()
        ws_list = reducer.get_result_workspaces()

        RenameWorkspace(InputWorkspace=ws_list[0], OutputWorkspace=out_ws)

        self.setProperty('OutputWorkspace', out_ws)

        ##TODO: Add sample logs

        EndTime('InelasticIndirectReduction')

# Register algorithm with Mantid
AlgorithmFactory.subscribe(InelasticIndirectReduction)
