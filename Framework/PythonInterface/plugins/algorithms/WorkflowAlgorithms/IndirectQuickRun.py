from mantid.simpleapi import (mtd, GroupWorkspaces, IndirectTwoPeakFit)

from mantid.api import *
from mantid.kernel import *
from mantid import config

import os


class IndirectQuickRun(DataProcessorAlgorithm):
    _data_files = None
    _instrument_name = None
    _analyser = None
    _reflection = None
    _efixed = None
    _spectra_range = None
    _elastic_range = None
    _inelastic_range = None
    _total_range = None
    _sample_log_name = None
    _sample_log_value = None
    _msdfit = False
    _widthfit = False
    _output_ws = None
    _scan_ws = None
    _ipf_filename = None
    _plot = False
    _save = False

    def category(self):
        return 'Workflow\\Inelastic;Inelastic\\Indirect;Workflow\\MIDAS'

    def summary(self):
        return 'Runs an energy transfer reduction for an inelastic indirect geometry instrument.'

    def PyInit(self):
        # Input properties
        self.declareProperty(StringArrayProperty(name='RunNumbers'),
                             doc='List of input runs')

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
        self.declareProperty(FloatArrayProperty(name='ElasticRange',
                                                validator=FloatArrayLengthValidator(2)),
                             doc='Energy range for the total energy component.')
        self.declareProperty(FloatArrayProperty(name='InelasticRange',
                                                validator=FloatArrayLengthValidator(2)),
                             doc='Energy range for the total energy component.')
        self.declareProperty(FloatArrayProperty(name='TotalRange',
                                                validator=FloatArrayLengthValidator(2)),
                             doc='Energy range for the total energy component.')

        self.declareProperty(name='SampleEnvironmentLogName', defaultValue='sample',
                             doc='Name of the sample environment log entry')

        sampEnvLogVal_type = ['last_value', 'average']
        self.declareProperty('SampleEnvironmentLogValue', 'last_value',
                             StringListValidator(sampEnvLogVal_type),
                             doc='Value selection of the sample environment log entry')

        self.declareProperty(name='MSDFit', defaultValue=False,
                             doc='Perform an MSDFit, do not use with GroupingMethod as "All"')

        self.declareProperty(name='WidthFit', defaultValue=False,
                             doc='Perform a 2 peak width Fit, do not use with GroupingMethod as "All"')

        self.declareProperty(name='Plot', defaultValue=False,
                             doc='Switch Plot Off/On')
        self.declareProperty(name='Save', defaultValue=False,
                             doc='Switch Save result to nxs file Off/On')

    # pylint: disable=too-many-locals
    def PyExec(self):
        self._setup()

        scan_progress = Progress(self, 0.0, 0.05, 3)
        scan_progress.report('Running scan')
        scan_alg = self.createChildAlgorithm("EnergyWindowScan", 0.05, 0.95)
        scan_alg.setProperty('InputFiles', self._data_files)
        scan_alg.setProperty('LoadLogFiles', True)
        scan_alg.setProperty('CalibrationWorkspace', '')
        scan_alg.setProperty('Instrument', self._instrument_name)
        scan_alg.setProperty('Analyser', self._analyser)
        scan_alg.setProperty('Reflection', self._reflection)
        scan_alg.setProperty('SpectraRange', self._spectra_range)
        scan_alg.setProperty('ElasticRange', self._elastic_range)
        scan_alg.setProperty('InelasticRange', self._inelastic_range)
        scan_alg.setProperty('TotalRange', self._total_range)
        scan_alg.setProperty('DetailedBalance', Property.EMPTY_DBL)
        scan_alg.setProperty('GroupingMethod', 'Individual')
        scan_alg.setProperty('SampleEnvironmentLogName', self._sample_log_name)
        scan_alg.setProperty('SampleEnvironmentLogValue', self._sample_log_value)
        scan_alg.setProperty('MSDFit', self._msdfit)
        scan_alg.setProperty('ReducedWorkspace', self._output_ws)
        scan_alg.setProperty('ScanWorkspace', self._scan_ws)
        scan_alg.execute()

        logger.information('OutputWorkspace : %s' % self._output_ws)
        logger.information('ScanWorkspace : %s' % self._scan_ws)

        if self._widthfit:
            result_workspaces = list()
            chi_workspaces = list()
            temperatures = list()
        # Get input workspaces
            fit_progress = Progress(self, 0.0, 0.05, 3)
            input_workspace_names = mtd[self._output_ws].getNames()
            x = mtd[input_workspace_names[0]].readX(0)
            xmin = x[0]
            xmax = x[len(x) - 1]
            for input_ws in input_workspace_names:

                red_ws = input_ws[:-3] + 'red'
                # Get the sample temperature
                temp = self._get_temperature(red_ws)
                if temp is not None:
                    temperatures.append(temp)
                else:
                # Get the run number
                    run_no = self._get_InstrRun(input_ws)[1]
                    run_numbers.append(run_no)

                num_hist = mtd[input_ws].getNumberHistograms()
                logger.information('Reduced histograms : %i' % num_hist)
                result = input_ws[:-3] + 'fit'
                func = 'name=Lorentzian,Amplitude=1.0,PeakCentre=0.0,FWHM=0.01'
                func += ',constraint=(Amplitude>0.0,FWHM>0.0)'
                for idx in range(num_hist):
                    fit_progress.report('Fitting workspace: %s ; spectrum %i' % (input_ws, idx))
                    IndirectTwoPeakFit(SampleWorkspace=input_ws,
                                       EnergyMin=xmin,
                                       EnergyMax=xmax,
                                       Minimizer='Levenberg-Marquardt',
                                       MaxIterations=500,
                                       OutputName=result)
                result_workspaces.append(result + '_Result')
                chi_workspaces.append(result + '_ChiSq')
            self._group_ws(chi_workspaces, self._output_ws + '_ChiSq')
            logger.information('ChiSq Group Workspace : %s' % self._output_ws + '_ChiSq')
            self._group_ws(result_workspaces, self._output_ws + '_Result')
            logger.information('Result Group Workspace : %s' % self._output_ws + '_Result')

            fit_progress.report('Creating width Group workspace')
            width_name = self._output_ws + '_Width1'
            for index, width_ws in enumerate(result_workspaces):
                if index == 0:
                    self._extract(width_ws, width_name, 0)
                else:
                    self._extract(width_ws, '__spectrum', 0)
                    self._append(width_name, '__spectrum', width_name)

            numb_temp = len(temperatures)
            x_axis_is_temp = len(input_workspace_names) == numb_temp

            if x_axis_is_temp:
                logger.information('X axis is in temperature')
                unit = ('Temperature', 'K')
            else:
                logger.information('X axis is in run number')
                unit = ('Run No', 'last 3 digits')

            ax = NumericAxis.create(numb_temp)
            for idx in range(numb_temp):
                if x_axis_is_temp:
                    val = float(temperatures[idx])
                else:
                    val = float(run_numbers[idx][-3:])
                ax.setValue(idx, val)
            mtd[width_name].replaceAxis(1, ax)
            mtd[width_name].setYUnitLabel("Temperature")

            xdat = list()
            ydat = list()
            edat = list()
            num_hist = mtd[width_name].getNumberHistograms()
            for idx in range(num_hist):
                x = mtd[width_name].readX(idx)
                y = mtd[width_name].readY(idx)
                e = mtd[width_name].readE(idx)
                if x_axis_is_temp:
                    xdat.append(float(temperatures[idx]))
                else:
                    xdat.append(float(run_numbers[idx][-3:]))
                ydat.append(y[5] / x[5])
                edat.append(e[5] / x[5])

            diffusion_workspace = self._output_ws + '_Diffusion'
            fit_progress.report('Creating diffusion workspace: %s' % diffusion_workspace)
            create_alg = self.createChildAlgorithm("CreateWorkspace", enableLogging=False)
            create_alg.setProperty("OutputWorkspace", diffusion_workspace)
            create_alg.setProperty("DataX", xdat)
            create_alg.setProperty("DataY", ydat)
            create_alg.setProperty("DataE", edat)
            create_alg.setProperty("NSpec", 1)
            create_alg.setProperty("YUnitLabel", 'Diffusion')
            create_alg.execute()
            mtd.addOrReplace(diffusion_workspace, create_alg.getProperty("OutputWorkspace").value)
            unitx = mtd[diffusion_workspace].getAxis(0).setUnit("Label")
            unitx.setLabel(unit[0], unit[1])
            logger.information('Diffusion Workspace : %s' % diffusion_workspace)

        if self._plot:
            self._plot_result()

        if self._save:
            self._save_output()

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
        elastic_range = self.getProperty('ElasticRange').value
        if elastic_range is not None:
            if len(elastic_range) != 2:
                issues['ElasticRange'] = 'Range must contain exactly two items'
            elif elastic_range[0] > elastic_range[1]:
                issues['ElasticRange'] = 'Range must be in format: lower,upper'
        inelastic_range = self.getProperty('InelasticRange').value
        if inelastic_range is not None:
            if len(inelastic_range) != 2:
                issues['InelasticRange'] = 'Range must contain exactly two items'
            elif inelastic_range[0] > inelastic_range[1]:
                issues['InelasticRange'] = 'Range must be in format: lower,upper'
        total_range = self.getProperty('TotalRange').value
        if inelastic_range is not None:
            if len(total_range) != 2:
                issues['TotalRange'] = 'Range must contain exactly two items'
            elif total_range[0] > total_range[1]:
                issues['TotalRange'] = 'Range must be in format: lower,upper'

        return issues

    def _setup(self):
        """
        Gets algorithm properties.
        """

        # Get properties
        self._instrument_name = self.getPropertyValue('Instrument')

        runs = self.getProperty('RunNumbers').value
        self._data_files = []
        self._format_runs(runs)
        first_file = self._data_files[0]
        last_file = self._data_files[len(self._data_files)-1]

        self._analyser = self.getPropertyValue('Analyser')
        self._reflection = self.getPropertyValue('Reflection')

        self._spectra_range = self.getProperty('SpectraRange').value
        self._elastic_range = self.getProperty('ElasticRange').value
        self._inelastic_range = self.getProperty('InelasticRange').value
        self._total_range = self.getProperty('TotalRange').value

        self._sample_log_name = self.getPropertyValue('SampleEnvironmentLogName')
        self._sample_log_value = self.getPropertyValue('SampleEnvironmentLogValue')

        self._msdfit = self.getProperty('msdFit').value

        self._widthfit = self.getProperty('WidthFit').value

        self._output_ws = first_file + '-' + last_file + '_scan_red'
        self._scan_ws = first_file + '-' + last_file + '_scan'

        self._plot = self.getProperty('Plot').value
        self._save = self.getProperty('Save').value

        # Get the IPF filename
        self._ipf_filename = os.path.join(config['instrumentDefinition.directory'],
                                          self._instrument_name + '_' + self._analyser + '_' + self._reflection + '_Parameters.xml')
        logger.information('Instrument parameter file: %s' % self._ipf_filename)

    def _get_temperature(self, ws_name):
        """
        Gets the sample temperature for a given workspace.

        @param ws_name Name of workspace
        @returns Temperature in Kelvin or None if not found
        """
        instr, run_number = self._get_InstrRun(ws_name)

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

    def _get_InstrRun(self, ws_name):
        """
        Get the instrument name and run number from a workspace.

        @param ws_name - name of the workspace
        @return tuple of form (instrument, run number)
        """

        run_number = str(mtd[ws_name].getRunNumber())
        if run_number == '0':
            # Attempt to parse run number off of name
            match = re.match(r'([a-zA-Z]+)([0-9]+)', ws_name)
            if match:
                run_number = match.group(2)
            else:
                raise RuntimeError("Could not find run number associated with workspace.")

        instrument = mtd[ws_name].getInstrument().getName()
        if instrument != '':
            for facility in config.getFacilities():
                try:
                    instrument = facility.instrument(instrument).filePrefix(int(run_number))
                    instrument = instrument.lower()
                    break
                except RuntimeError:
                    continue

        return instrument, run_number

    def _extract(self, input_ws, output_ws, index):
        extract_alg = self.createChildAlgorithm("ExtractSingleSpectrum", enableLogging = False)
        extract_alg.setProperty("InputWorkspace", input_ws)
        extract_alg.setProperty("WorkspaceIndex", index)
        extract_alg.setProperty("OutputWorkspace", output_ws)
        extract_alg.execute()
        mtd.addOrReplace(output_ws, extract_alg.getProperty("OutputWorkspace").value)

    def _append(self, input1_ws, input2_ws, output_ws):
        append_alg = self.createChildAlgorithm("AppendSpectra", enableLogging = False)
        append_alg.setProperty("InputWorkspace1", input1_ws)
        append_alg.setProperty("InputWorkspace2", input2_ws)
        append_alg.setProperty("OutputWorkspace", output_ws)
        append_alg.execute()
        mtd.addOrReplace(output_ws, append_alg.getProperty("OutputWorkspace").value)

    def _save_output(self):
        from mantid.simpleapi import SaveNexusProcessed
        workdir = config['defaultsave.directory']
        el_eq1_path = os.path.join(workdir, self._scan_ws + '_el_eq1.nxs')
        logger.information('Creating file : %s' % el_eq1_path)
        self._save_group(self._scan_ws + '_el_eq1', el_eq1_path)
        el_eq2_path = os.path.join(workdir, self._scan_ws + '_el_eq2.nxs')
        logger.information('Creating file : %s' % el_eq2_path)
        self._save_group(self._scan_ws + '_el_eq2', el_eq2_path)

        inel_eq1_path = os.path.join(workdir, self._scan_ws + '_inel_eq1.nxs')
        logger.information('Creating file : %s' % inel_eq1_path)
        self._save_group(self._scan_ws + '_inel_eq1', inel_eq1_path)
        inel_eq2_path = os.path.join(workdir, self._scan_ws + '_inel_eq2.nxs')
        logger.information('Creating file : %s' % inel_eq2_path)
        self._save_group(self._scan_ws + '_inel_eq2', inel_eq2_path)

        total_eq1_path = os.path.join(workdir, self._scan_ws + '_total_eq1.nxs')
        logger.information('Creating file : %s' % total_eq1_path)
        self._save_group(self._scan_ws + '_inel_eq1', total_eq1_path)
        inel_eq2_path = os.path.join(workdir, self._scan_ws + '_total_eq2.nxs')
        logger.information('Creating file : %s' % _total_eq2)
        self._save_group(self._scan_ws + '_inel_eq2', _total_eq2)

        eisf_path = os.path.join(workdir, self._scan_ws + '_eisf.nxs')
        logger.information('Creating file : %s' % eisf_path)
        self._save_group(self._scan_ws + '_eisf', eisf_path)

        if self._msdfit:
            msd_path = os.path.join(workdir, self._scan_ws + '_msd.nxs')
            logger.information('Creating file : %s' % msd_path)
            self._save_group(self._scan_ws + '_msd', msd_path)
            msd_fit_path = os.path.join(workdir, self._scan_ws + '_msd_fit.nxs')
            logger.information('Creating file : %s' % msd_fit_path)
            self._save_group(self._scan_ws + '_msd_fit', msd_fit_path)

    def _plot_result(self):
        import mantidplot as mp
        mp.plotSpectrum(self._scan_ws + '_el_eq1', 0, error_bars=True)
        mp.plotSpectrum(self._scan_ws + '_inel_eq1', 0, error_bars=True)
        mp.plotSpectrum(self._scan_ws + '_total_eq1', 0, error_bars=True)
        mp.plotSpectrum(self._scan_ws + '_el_eq2', 0, error_bars=True)
        mp.plotSpectrum(self._scan_ws + '_inel_eq2', 0, error_bars=True)
        mp.plotSpectrum(self._scan_ws + '_total_eq2', 0, error_bars=True)
        mp.plotSpectrum(self._scan_ws + '_eisf', 0, error_bars=True)
        if self._msdfit:
            mp.plotSpectrum(self._scan_ws + '_msd', 1, error_bars=True)
        if self._widthfit:
            mp.plotSpectrum(self._output_ws + '_Diffusion', 0, error_bars=True)

    def _format_runs(self, runs):
        run_list = []
        for run in runs:
            if '-' in run:
                a, b = run.split('-')
                run_list.extend(range(int(a), int(b)+1))
            else:
                run_list.append(int(run))
        for idx in run_list:
            self._data_files.append(self._instrument_name.lower() + str(idx))

    def _group_ws(self, input_ws, output_ws):
        group_alg = self.createChildAlgorithm("GroupWorkspaces", enableLogging=False)
        group_alg.setProperty("InputWorkspaces", input_ws)
        group_alg.setProperty("OutputWorkspace", output_ws)
        group_alg.execute()
        mtd.addOrReplace(output_ws, group_alg.getProperty("OutputWorkspace").value)

    def _save_group(self, input_ws, filename):
        save_alg = self.createChildAlgorithm("GroupWorkspaces", enableLogging=False)
        save_alg.setProperty("InputWorkspace", input_ws)
        save_alg.setProperty("Filename", filename)
        save_alg.execute()
# Register algorithm with Mantid
AlgorithmFactory.subscribe(IndirectQuickRun)
