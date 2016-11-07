import numpy as np
import os

from mantid import config
from mantid.simpleapi import *


class SqwMomentsScan(DataProcessorAlgorithm):
    _data_files = None
    _sum_files = None
    _load_logs = None
    _calibration_ws = None
    _instrument_name = None
    _analyser = None
    _reflection = None
    _efixed = None
    _spectra_range = None
    _background_range = None
    _elastic_range = None
    _inelastic_range = None
    _rebin_string = None
    _detailed_balance = None
    _grouping_method = None
    _grouping_ws = None
    _grouping_map_file = None
    _output_ws = None
    _output_x_units = None
    _plot_type = None
    _save_formats = None
    _ipf_filename = None
    _sample_log_name = None
    _sample_log_value = None
    _workspace_names = None
    _scan_ws = None

    def category(self):
        return 'Workflow\\Inelastic;Inelastic\\Indirect;Workflow\\MIDAS'

    def summary(self):
        return 'Runs an energy transfer reduction for an inelastic indirect geometry instrument.'

    def PyInit(self):
        # Input properties
        self.declareProperty(StringArrayProperty(name='InputFiles'),
                             doc='Comma separated list of input files')

        self.declareProperty(name='LoadLogFiles', defaultValue=True,
                             doc='Load log files when loading runs')

        self.declareProperty(WorkspaceProperty('CalibrationWorkspace', '',
                                               direction=Direction.Input,
                                               optional=PropertyMode.Optional),
                             doc='Workspace containing calibration data')

        # Instrument configuration properties
        self.declareProperty(name='Instrument', defaultValue='',
                             validator=StringListValidator(['IRIS', 'OSIRIS']),
                             doc='Instrument used during run.')
        self.declareProperty(name='Analyser', defaultValue='',
                             validator=StringListValidator(['graphite', 'mica', 'fmica']),
                             doc='Analyser bank used during run.')
        self.declareProperty(name='Reflection', defaultValue='',
                             validator=StringListValidator(['002', '004', '006']),
                             doc='Reflection number for instrument setup during run.')

        self.declareProperty(IntArrayProperty(name='SpectraRange', values=[0, 1],
                                              validator=IntArrayMandatoryValidator()),
                             doc='Comma separated range of spectra number to use.')
        self.declareProperty(FloatArrayProperty(name='QRange'),
                             doc='Range of background to subtract from raw data in time of flight.')
        self.declareProperty(FloatArrayProperty(name='EnergyRange'),
                             doc='Range of background to subtract from raw data in time of flight.')
        self.declareProperty(name='DetailedBalance', defaultValue=Property.EMPTY_DBL,
                             doc='Apply the detailed balance correction')

        # Spectra grouping options
        self.declareProperty(name='GroupingMethod', defaultValue='Individual',
                             validator=StringListValidator(['Individual', 'All', 'File', 'Workspace', 'IPF']),
                             doc='Method used to group spectra.')
        self.declareProperty(WorkspaceProperty('GroupingWorkspace', '',
                                               direction=Direction.Input,
                                               optional=PropertyMode.Optional),
                             doc='Workspace containing spectra grouping.')
        self.declareProperty(FileProperty('MapFile', '',
                                          action=FileAction.OptionalLoad,
                                          extensions=['.map']),
                             doc='File containing spectra grouping.')

        self.declareProperty(name='SampleEnvironmentLogName', defaultValue='sample',
                             doc='Name of the sample environment log entry')

        sampEnvLogVal_type = ['last_value', 'average']
        self.declareProperty('SampleEnvironmentLogValue', 'last_value',
                             StringListValidator(sampEnvLogVal_type),
                             doc='Value selection of the sample environment log entry')

        # Output properties
        self.declareProperty(WorkspaceGroupProperty('ReducedWorkspace', defaultValue='Reduced',
                                                    direction=Direction.Output),
                             doc='The output reduced workspace.')
        self.declareProperty(WorkspaceGroupProperty('SqwWorkspace', defaultValue='Sqw',
                                                    direction=Direction.Output),
                             doc='The output Sqw workspace.')
        self.declareProperty(name='MomentWorkspace', defaultValue='Moment',
                             doc='The output Moment workspace.')

    def PyExec(self):

        self._setup()
        progress = Progress(self, 0.0, 0.05, 3)

        progress.report('Energy transfer')
        scan_alg = self.createChildAlgorithm("ISISIndirectEnergyTransfer", 0.05, 0.95)
        scan_alg.setProperty('InputFiles', self._data_files)
        scan_alg.setProperty('SumFiles', self._sum_files)
        scan_alg.setProperty('LoadLogFiles', self._load_logs)
        scan_alg.setProperty('CalibrationWorkspace', self._calibration_ws)
        scan_alg.setProperty('Instrument', self._instrument_name)
        scan_alg.setProperty('Analyser', self._analyser)
        scan_alg.setProperty('Reflection', self._reflection)
        scan_alg.setProperty('Efixed', self._efixed)
        scan_alg.setProperty('SpectraRange', self._spectra_range)
        scan_alg.setProperty('BackgroundRange', self._background_range)
        scan_alg.setProperty('RebinString', self._rebin_string)
        scan_alg.setProperty('DetailedBalance', self._detailed_balance)
        scan_alg.setProperty('ScaleFactor', self._scale_factor)
        scan_alg.setProperty('FoldMultipleFrames', self._fold_multiple_frames)
        scan_alg.setProperty('GroupingMethod', self._grouping_method)
        scan_alg.setProperty('GroupingWorkspace', self._grouping_ws)
        scan_alg.setProperty('MapFile', self._grouping_map_file)
        scan_alg.setProperty('UnitX', self._output_x_units)
        scan_alg.setProperty('OutputWorkspace', self._red_ws)
        scan_alg.execute()
        logger.information('ReducedWorkspace : %s' % self._red_ws)

        rebin_temp = '__rebin_temp'
        Rebin(InputWorkspace=self._red_ws,
              OutputWorkspace=self._red_ws,
              Params=self._energy_range,
              EnableLogging=False)

        input_workspace_names = mtd[self._red_ws].getNames()
        output_workspaces = list()
        temperatures = list()
        sofqw_alg = self.createChildAlgorithm("SofQW", enableLogging=False)
        for input_ws in input_workspace_names:
            progress.report('SofQW for workspace: %s' % input_ws)
            sofqw_alg.setProperty("InputWorkspace", input_ws)
            sofqw_alg.setProperty("QAxisBinning", self._q_range)
            sofqw_alg.setProperty("EMode", 'Indirect')
            sofqw_alg.setProperty("ReplaceNaNs", True)
            sofqw_alg.setProperty("Method", 'Polygon')
            sofqw_alg.setProperty("OutputWorkspace", input_ws + '_sqw')
            sofqw_alg.execute()
            mtd.addOrReplace(input_ws + '_sqw', sofqw_alg.getProperty("OutputWorkspace").value)
            output_workspaces.append(input_ws + '_sqw')

            # Get the sample temperature
            temp = self._get_temperature(input_ws + '_sqw')
            if temp is not None:
                temperatures.append(temp)

        group_alg = self.createChildAlgorithm("GroupWorkspaces", enableLogging=False)
        group_alg.setProperty("InputWorkspaces", output_workspaces)
        group_alg.setProperty("OutputWorkspace", self._sqw_ws)
        group_alg.execute()
        mtd.addOrReplace(self._sqw_ws, group_alg.getProperty("OutputWorkspace").value)
        logger.information('SqwWorkspace : %s' % self._sqw_ws)

        # Get input workspaces
        input_workspace_names = mtd[self._sqw_ws].getNames()
        output_workspaces = list()
        width_workspaces = list()

        for input_ws in input_workspace_names:
            logger.information('Running SofQWMoments for workspace: %s' % input_ws)
            SofQWMoments(InputWorkspace=input_ws,
                         EnergyMin=self._energy_range[0],
                         EnergyMax=self._energy_range[1],
                         Scale=self._scale_factor,
                         OutputWorkspace=input_ws + '_mom',
                         EnableLogging=False)
            output_workspaces.append(input_ws + '_mom')
            width_ws = input_ws + '_width'
            extract_alg = self.createChildAlgorithm("ExtractSingleSpectrum", enableLogging=False)
            extract_alg.setProperty("InputWorkspace", input_ws + '_mom')
            extract_alg.setProperty("OutputWorkspace", width_ws)
            extract_alg.setProperty("WorkspaceIndex", 2)
            extract_alg.execute()
            mtd.addOrReplace(width_ws, extract_alg.getProperty("OutputWorkspace").value)
            num_hist = mtd[width_ws].getNumberHistograms()
            for idx in range(num_hist):
                y_m2 = mtd[width_ws].readY(idx)
                e_m2 = mtd[width_ws].readE(idx)
                rel_error = e_m2 / y_m2
                y_width = np.sqrt(y_m2) / 100.0
                e_width = 0.5 * rel_error * y_width
                mtd[width_ws].setY(idx, y_width)
                mtd[width_ws].setE(idx, e_width)
            width_workspaces.append(width_ws)
        group_alg.setProperty("InputWorkspaces", output_workspaces)
        group_alg.setProperty("OutputWorkspace", self._moment_ws)
        group_alg.execute()
        mtd.addOrReplace(self._moment_ws, group_alg.getProperty("OutputWorkspace").value)
        logger.information('MomentWorkspace : %s' % self._moment_ws)

        width_workspace = self._moment_ws + '_width'
        clone_alg = self.createChildAlgorithm("CloneWorkspace", enableLogging=False)
        append_alg = self.createChildAlgorithm("AppendSpectra", enableLogging=False)
        for idx in range(len(width_workspaces)):
            if idx == 0:
                clone_alg.setProperty("InputWorkspace", width_workspaces[0])
                clone_alg.setProperty("OutputWorkspace", width_workspace)
                clone_alg.execute()
                mtd.addOrReplace(width_workspace, clone_alg.getProperty("OutputWorkspace").value)
            else:
                append_alg.setProperty("InputWorkspace1", width_workspace)
                append_alg.setProperty("InputWorkspace2", width_workspaces[idx])
                append_alg.setProperty("OutputWorkspace", width_workspace)
                append_alg.execute()
                mtd.addOrReplace(width_workspace, append_alg.getProperty("OutputWorkspace").value)

        delete_alg = self.createChildAlgorithm("DeleteWorkspace", enableLogging=False)
        for ws in width_workspaces:
            delete_alg.setProperty("Workspace", ws)
            delete_alg.execute()

        # Set the vertical axis units
        num_hist = mtd[width_workspace].getNumberHistograms()
        v_axis_is_temp = num_hist == len(temperatures)

        if v_axis_is_temp:
            logger.notice('Vertical axis is in temperature')
            unit = ('Temperature', 'K')
        else:
            logger.notice('Vertical axis is in run number')
            unit = ('Run No', 'last 3 digits')

        # Create a new vertical axis for the workspaces
        y_ws_axis = NumericAxis.create(len(width_workspace))
        y_ws_axis.setUnit("Label").setLabel(unit[0], unit[1])

        # Set the vertical axis values
        for idx in range(num_hist):
            if v_axis_is_temp:
                y_ws_axis.setValue(idx, float(temperatures[idx]))
            else:
                y_ws_axis.setValue(idx, float(run_numbers[idx][-3:]))

        # Add the new vertical axis to each workspace
        mtd[width_workspace].replaceAxis(1, y_ws_axis)

        xdat = list()
        ydat = list()
        edat = list()
        for idx in range(num_hist):
            x = mtd[width_workspace].readX(idx)
            y = mtd[width_workspace].readY(idx)
            e = mtd[width_workspace].readE(idx)
            if v_axis_is_temp:
                xdat.append(float(temperatures[idx]))
            else:
                xdat.append(float(run_numbers[idx][-3:]))
            ydat.append(y[5] / x[5])
            edat.append(e[5] / x[5])
        diffusion_workspace = self._moment_ws + '_diffusion'
        create_alg = self.createChildAlgorithm("CreateWorkspace", enableLogging=False)
        create_alg.setProperty("OutputWorkspace", diffusion_workspace)
        create_alg.setProperty("DataX", xdat)
        create_alg.setProperty("DataY", ydat)
        create_alg.setProperty("DataE", edat)
        create_alg.setProperty("NSpec", 1)
        create_alg.execute()
        mtd.addOrReplace(diffusion_workspace, create_alg.getProperty("OutputWorkspace").value)
        unitx = mtd[diffusion_workspace].getAxis(0).setUnit("Label")
        unitx.setLabel(unit[0], unit[1])

    def validateInputs(self):
        """
        Validates algorithm properties.
        """
        issues = dict()

        # Validate the instrument configuration by checking if a parameter file exists
        instrument_name = self.getPropertyValue('Instrument')
        analyser = self.getPropertyValue('Analyser')
        reflection = self.getPropertyValue('Reflection')

        ipf_filename = os.path.join(config['instrumentDefinition.directory'],
                                    instrument_name + '_' + analyser + '_' + reflection + '_Parameters.xml')

        if not os.path.exists(ipf_filename):
            error_message = 'Invalid instrument configuration'
            issues['Instrument'] = error_message
            issues['Analyser'] = error_message
            issues['Reflection'] = error_message

        # Validate spectra range
        spectra_range = self.getProperty('SpectraRange').value
        if len(spectra_range) != 2:
            issues['SpectraRange'] = 'Range must contain exactly two items'
        elif spectra_range[0] > spectra_range[1]:
            issues['SpectraRange'] = 'Range must be in format: lower,upper'

        # Validate ranges
        q_range = self.getProperty('QRange').value
        if q_range is not None:
            if len(q_range) != 3:
                issues['QRange'] = 'Range must contain exactly two items'
            elif q_range[0] > q_range[2]:
                issues['QRange'] = 'Range must be in format: lower,upper'
        energy_range = self.getProperty('EnergyRange').value
        if energy_range is not None:
            if len(energy_range) != 3:
                issues['EnergyRange'] = 'Range must contain exactly two items'
            elif energy_range[0] > energy_range[2]:
                issues['EnergyRange'] = 'Range must be in format: lower,upper'

        # Validate grouping method
        grouping_method = self.getPropertyValue('GroupingMethod')
        grouping_ws = _ws_or_none(self.getPropertyValue('GroupingWorkspace'))

        if grouping_method == 'Workspace' and grouping_ws is None:
            issues['GroupingWorkspace'] = 'Must select a grouping workspace for current GroupingWorkspace'

        return issues

    def _setup(self):
        """
        Gets algorithm properties.
        """

        # Get properties
        self._data_files = self.getProperty('InputFiles').value
        self._sum_files = False
        self._load_logs = self.getProperty('LoadLogFiles').value
        self._calibration_ws = ''

        self._instrument_name = self.getPropertyValue('Instrument')
        self._analyser = self.getPropertyValue('Analyser')
        self._reflection = self.getPropertyValue('Reflection')
        self._efixed = Property.EMPTY_DBL

        self._spectra_range = self.getProperty('SpectraRange').value
        self._background_range = ''
        self._rebin_string = ''
        self._detailed_balance = self.getProperty('DetailedBalance').value
        self._scale_factor = 1.0
        self._fold_multiple_frames = False
        self._q_range = self.getProperty('QRange').value
        self._energy_range = self.getProperty('EnergyRange').value

        self._grouping_method = self.getPropertyValue('GroupingMethod')
        self._grouping_ws = ''
        self._grouping_map_file = ''

        self._output_x_units = 'DeltaE'

        self._sample_log_name = self.getPropertyValue('SampleEnvironmentLogName')
        self._sample_log_value = self.getPropertyValue('SampleEnvironmentLogValue')

        self._red_ws = self.getPropertyValue('ReducedWorkspace')
        self._sqw_ws = self.getPropertyValue('SqwWorkspace')
        self._moment_ws = self.getProperty('MomentWorkspace').value

        # Disable sum files if there is only one file
        if len(self._data_files) == 1:
            if self._sum_files:
                logger.warning('SumFiles disabled when only one input file is provided.')
            self._sum_files = False

        # Get the IPF filename
        self._ipf_filename = os.path.join(config['instrumentDefinition.directory'],
                                          self._instrument_name + '_' + self._analyser + '_' + self._reflection + '_Parameters.xml')
        logger.information('Instrument parameter file: %s' % self._ipf_filename)

        # Warn when grouping options are to be ignored
        if self._grouping_method != 'Workspace' and self._grouping_ws is not None:
            logger.warning('GroupingWorkspace will be ignored by selected GroupingMethod')

        if self._grouping_method != 'File' and self._grouping_map_file is not None:
            logger.warning('MapFile will be ignored by selected GroupingMethod')

        # The list of workspaces being processed
        self._workspace_names = []

    def _get_temperature(self, ws_name):
        """
        Gets the sample temperature for a given workspace.

        @param ws_name Name of workspace
        @returns Temperature in Kelvin or None if not found
        """
        from IndirectCommon import getInstrRun

        instr, run_number = getInstrRun(ws_name)

        facility = config.getFacility()
        pad_num = facility.instrument(instr).zeroPadding(int(run_number))
        zero_padding = '0' * (pad_num - len(run_number))

        run_name = instr + zero_padding + run_number
        log_filename = run_name.upper() + '.log'

        run = mtd[ws_name].getRun()

        if self._sample_log_name in run:
            # Look for temperature in logs in workspace
            tmp = run[self._sample_log_name].value
            value_action = {'last_value': lambda x: x[len(x) - 1],
                            'average': lambda x: x.mean()
                            }
            temp = value_action[self._sample_log_value](tmp)
            logger.debug('Temperature %d K found for run: %s' % (temp, run_name))
            return temp

        else:
            # Logs not in workspace, try loading from file
            logger.information('Log parameter not found in workspace. Searching for log file.')
            log_path = FileFinder.getFullPath(log_filename)

            if log_path != '':
                # Get temperature from log file
                LoadLog(Workspace=ws_name, Filename=log_path)
                run_logs = mtd[ws_name].getRun()
                if self._sample_log_name in run_logs:
                    tmp = run_logs[self._sample_log_name].value
                    temp = tmp[len(tmp) - 1]
                    logger.debug('Temperature %d K found for run: %s' % (temp, run_name))
                    return temp
                else:
                    logger.warning('Log entry %s for run %s not found' % (self._sample_log_name, run_name))
            else:
                logger.warning('Log file for run %s not found' % run_name)

        # Can't find log file
        logger.warning('No temperature found for run: %s' % run_name)
        return None


# Register algorithm with Mantid
AlgorithmFactory.subscribe(SqwMomentsScan)
