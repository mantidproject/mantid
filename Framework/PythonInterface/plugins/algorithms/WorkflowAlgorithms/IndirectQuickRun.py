
from mantid.api import *
from mantid.kernel import *
from mantid import config

import os


class IndirectQuickRun(DataProcessorAlgorithm):
    _chopped_data = None
    _data_files = None
    _load_logs = None
    _calibration_ws = None
    _instrument_name = None
    _analyser = None
    _reflection = None
    _efixed = None
    _spectra_range = None
    _background_range = None
    _rebin_string = None
    _detailed_balance = None
    _grouping_method = None
    _grouping_ws = None
    _grouping_map_file = None
    _output_ws = None
    _ipf_filename = None
    _workspace_names = None

    def category(self):
        return 'Workflow\\Inelastic;Inelastic\\Indirect;Workflow\\MIDAS'

    def summary(self):
        return 'Runs an energy transfer reduction for an inelastic indirect geometry instrument.'

    def PyInit(self):
        # Input properties
        self.declareProperty(StringArrayProperty(name='RunNumbers'),
                             doc='List of input runs')

        self.declareProperty(name='LoadLogFiles', defaultValue=True,
                             doc='Load log files when loading runs')

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
        self.declareProperty(FloatArrayProperty(name='ElasticRange'),
                             doc='Range of background to subtract from raw data in time of flight.')
        self.declareProperty(FloatArrayProperty(name='InelasticRange'),
                             doc='Range of background to subtract from raw data in time of flight.')
        self.declareProperty(name='DetailedBalance', defaultValue=Property.EMPTY_DBL,
                             doc='')

        # Spectra grouping options
        self.declareProperty(name='GroupingMethod', defaultValue='Individual',
                             validator=StringListValidator(['Individual', 'All', 'File']),
                             doc='Method used to group spectra.')

        self.declareProperty(name='SampleEnvironmentLogName', defaultValue='sample',
                             doc='Name of the sample environment log entry')

        sampEnvLogVal_type = ['last_value', 'average']
        self.declareProperty('SampleEnvironmentLogValue', 'last_value',
                             StringListValidator(sampEnvLogVal_type),
                             doc='Value selection of the sample environment log entry')

        self.declareProperty(name='MSDFit', defaultValue=False,
                             doc='Perform an MSDFit, do not use with GroupingMethod as "All"')

        self.declareProperty(name='SumFiles', defaultValue=False,
                             doc='Toggle input file summing or sequential processing')

        self.declareProperty(name='Plot', defaultValue=False,
                             doc='Switch Plot Off/On')
        self.declareProperty(name='Save', defaultValue=False,
                             doc='Switch Save result to nxs file Off/On')

    # pylint: disable=too-many-locals
    def PyExec(self):
        setup_progress = Progress(self, 0.0, 0.05, 3)
        setup_progress.report('Getting Parameters')
        self._setup()

        scan_alg = self.createChildAlgorithm("EnergyWindowScan", 0.05, 0.95)
        scan_alg.setProperty('InputFiles', self._data_files)
        scan_alg.setProperty('LoadLogFiles', self._load_logs)
        scan_alg.setProperty('CalibrationWorkspace', self._calibration_ws)
        scan_alg.setProperty('Instrument', self._instrument_name)
        scan_alg.setProperty('Analyser', self._analyser)
        scan_alg.setProperty('Reflection', self._reflection)
        scan_alg.setProperty('SpectraRange', self._spectra_range)
        scan_alg.setProperty('ElasticRange', self._elastic_range)
        scan_alg.setProperty('InelasticRange', self._inelastic_range)
        scan_alg.setProperty('DetailedBalance', self._detailed_balance)
        scan_alg.setProperty('GroupingMethod', self._grouping_method)
        scan_alg.setProperty('SampleEnvironmentLogName', self._sample_log_name)
        scan_alg.setProperty('SampleEnvironmentLogValue', self._sample_log_value)
        scan_alg.setProperty('MSDFit', self._msdfit)
        scan_alg.setProperty('ReducedWorkspace', self._output_ws)
        scan_alg.setProperty('ScanWorkspace', self._scan_ws)
        scan_alg.execute()

        logger.information('OutputWorkspace : %s' % self._output_ws)
        logger.information('ScanWorkspace : %s' % self._scan_ws)

        post_progress = Progress(self, 0.95, 1.0, 3)
        if self._plot:
            post_progress.report('Plotting')
            self._plot_result()

        if self._save:
            post_progress.report('Saving')
            self._save_output()

        post_progress.report('Algorithm Complete')

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
        self._sum_files = self.getProperty('SumFiles').value
        self._load_logs = self.getProperty('LoadLogFiles').value
        self._calibration_ws = ''

        self._analyser = self.getPropertyValue('Analyser')
        self._reflection = self.getPropertyValue('Reflection')

        self._spectra_range = self.getProperty('SpectraRange').value
        self._elastic_range = self.getProperty('ElasticRange').value
        self._inelastic_range = self.getProperty('InelasticRange').value
        self._detailed_balance = self.getProperty('DetailedBalance').value

        self._grouping_method = self.getPropertyValue('GroupingMethod')
        self._grouping_ws = ''
        self._grouping_map_file = ''

        self._sample_log_name = self.getPropertyValue('SampleEnvironmentLogName')
        self._sample_log_value = self.getPropertyValue('SampleEnvironmentLogValue')

        self._msdfit = self.getProperty('msdFit').value
        if self._msdfit and (self._grouping_method == 'All'):
            logger.warning("MSDFit will not run if GroupingMethod is 'All'")
            self._msdfit = False

        self._output_ws = first_file + '_ew_scan_red'
        self._scan_ws = first_file + '_ew_scan'

        self._plot = self.getProperty('Plot').value
        self._save = self.getProperty('Save').value

        # Get the IPF filename
        self._ipf_filename = os.path.join(config['instrumentDefinition.directory'],
                                          self._instrument_name + '_' + self._analyser + '_' + self._reflection + '_Parameters.xml')
        logger.information('Instrument parameter file: %s' % self._ipf_filename)

        # Warn when grouping options are to be ignored
        if self._grouping_method != 'Workspace' and self._grouping_ws is not None:
            logger.warning('GroupingWorkspace will be ignored by selected GroupingMethod')

        if self._grouping_method != 'File' and self._grouping_map_file is not None:
            logger.warning('MapFile will be ignored by selected GroupingMethod')

    def _save_output(self):
        from mantid.simpleapi import SaveNexusProcessed
        workdir = config['defaultsave.directory']
        el_eq1_path = os.path.join(workdir, self._scan_ws + '_el_eq1.nxs')
        logger.information('Creating file : %s' % el_eq1_path)
        SaveNexusProcessed(InputWorkspace=self._scan_ws + '_el_eq1',
                           Filename=el_eq1_path)
        el_eq2_path = os.path.join(workdir, self._scan_ws + '_el_eq2.nxs')
        logger.information('Creating file : %s' % el_eq2_path)
        SaveNexusProcessed(InputWorkspace=self._scan_ws + '_el_eq2',
                           Filename=el_eq2_path)

        inel_eq1_path = os.path.join(workdir, self._scan_ws + '_inel_eq1.nxs')
        logger.information('Creating file : %s' % inel_eq1_path)
        SaveNexusProcessed(InputWorkspace=self._scan_ws + '_inel_eq1',
                           Filename=inel_eq1_path)
        inel_eq2_path = os.path.join(workdir, self._scan_ws + '_inel_eq2.nxs')
        logger.information('Creating file : %s' % inel_eq2_path)
        SaveNexusProcessed(InputWorkspace=self._scan_ws + '_inel_eq2',
                           Filename=inel_eq2_path)

        eisf_path = os.path.join(workdir, self._scan_ws + '_eisf.nxs')
        logger.information('Creating file : %s' % eisf_path)
        SaveNexusProcessed(InputWorkspace=self._scan_ws + '_eisf',
                           Filename=eisf_path)

        if self._msdfit:
            msd_path = os.path.join(workdir, self._scan_ws + '_msd.nxs')
            logger.information('Creating file : %s' % msd_path)
            SaveNexusProcessed(InputWorkspace=self._scan_ws + '_msd',
                               Filename=msd_path)
            msd_fit_path = os.path.join(workdir, self._scan_ws + '_msd_fit.nxs')
            logger.information('Creating file : %s' % msd_fit_path)
            SaveNexusProcessed(InputWorkspace=self._scan_ws + '_msd_fit',
                               Filename=msd_fit_path)

    def _plot_result(self):
        import mantidplot as mp
        mp.plotSpectrum(self._scan_ws + '_el_eq1', 0, error_bars=True)
        mp.plotSpectrum(self._scan_ws + '_inel_eq1', 0, error_bars=True)
        mp.plotSpectrum(self._scan_ws + '_el_eq2', 0, error_bars=True)
        mp.plotSpectrum(self._scan_ws + '_inel_eq2', 0, error_bars=True)
        mp.plotSpectrum(self._scan_ws + '_eisf', 0, error_bars=True)
        if self._msdfit:
            mp.plotSpectrum(self._scan_ws + '_msd', 1, error_bars=True)

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


# Register algorithm with Mantid
AlgorithmFactory.subscribe(IndirectQuickRun)
