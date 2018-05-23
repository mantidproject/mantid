import os.path

from mantid import config
from mantid.api import *
from mantid.kernel import *
from mantid.simpleapi import *


class IndirectSampleChanger(DataProcessorAlgorithm):
    _plot = False
    _save = False
    _instrument = 'IRIS'
    _number_runs = 1
    _runs = None
    _temperature = None
    _q1_workspaces = None

    def category(self):
        return "Workflow\\MIDAS"

    def summary(self):
        return "Create elastic window scans for sample changer"

    def PyInit(self):
        self.declareProperty(name='Instrument', defaultValue='IRIS',
                             validator=StringListValidator(['IRIS', 'OSIRIS']),
                             doc='Instrument name')
        self.declareProperty(name='Analyser', defaultValue='',
                             validator=StringListValidator(['graphite', 'mica', 'fmica']),
                             doc='Analyser bank used during run.')
        self.declareProperty(name='Reflection', defaultValue='',
                             validator=StringListValidator(['002', '004', '006']),
                             doc='Reflection number for instrument setup during run.')

        self.declareProperty(name="FirstRun", defaultValue=-1,
                             validator=IntBoundedValidator(lower=0),
                             doc="First Sample run-number.")
        self.declareProperty(name='LastRun', defaultValue=-1,
                             validator=IntBoundedValidator(lower=0),
                             doc="Last Sample run-number.")
        self.declareProperty(name='NumberSamples', defaultValue=-1,
                             validator=IntBoundedValidator(lower=0),
                             doc="Increment for run-number.")

        self.declareProperty(IntArrayProperty(name='SpectraRange', values=[0, 1],
                                              validator=IntArrayLengthValidator(2)),
                             doc='Comma separated range of spectra number to use.')
        self.declareProperty(FloatArrayProperty(name='ElasticRange',
                                                validator=FloatArrayLengthValidator(2)),
                             doc='Range of background to subtract from raw data in time of flight.')
        self.declareProperty(FloatArrayProperty(name='InelasticRange',
                                                validator=FloatArrayLengthValidator(2)),
                             doc='Range of background to subtract from raw data in time of flight.')

        # Spectra grouping options
        self.declareProperty(name='GroupingMethod', defaultValue='Individual',
                             validator=StringListValidator(['Individual', 'All', 'File']),
                             doc='Method used to group spectra.')

        self.declareProperty(name='MsdFit', defaultValue=False,
                             doc='Run msd fit')

        self.declareProperty(name='Save', defaultValue=False,
                             doc='Switch Save result to nxs file Off/On')
        self.declareProperty(name='Plot', defaultValue=False,
                             doc='Plot options')

    def PyExec(self):
        from IndirectImport import import_mantidplot
        mp = import_mantidplot()
        workdir = config['defaultsave.directory']
        # self._setup()

        q2_workspaces = []
        scan_alg = self.createChildAlgorithm("EnergyWindowScan", 0.05, 0.95)
        for numb in range(self._number_samples):
            run_numbers = []
            run_names = []
            first_run = self._run_first + numb
            for idx in range(int(self._number_runs)):
                run = str(first_run + idx * self._number_samples)
                run_numbers.append(run)
                run_names.append(self._instrument + run)
            q0 = self._instrument.lower() + run_numbers[0] + '_to_' + run_numbers[-1] + '_s' + str(numb)
            output_ws = q0 + '_red'
            scan_ws = q0 + '_scan'
            scan_alg.setProperty('InputFiles', run_names)
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
            scan_alg.setProperty('msdFit', self._msdfit)
            scan_alg.setProperty('ReducedWorkspace', output_ws)
            scan_alg.setProperty('ScanWorkspace', scan_ws)
            scan_alg.execute()

            logger.information('OutputWorkspace : %s' % output_ws)
            logger.information('ScanWorkspace : %s' % scan_ws)

            q1_ws = scan_ws + '_el_eq1'
            q2_ws = scan_ws + '_el_eq2'
            q2_workspaces.append(q2_ws)
            eisf_ws = scan_ws + '_eisf'
            el_elt_ws = scan_ws + '_el_elt'
            inel_elt_ws = scan_ws + '_inel_elt'
            output_workspaces = [q1_ws, q2_ws, eisf_ws, el_elt_ws, inel_elt_ws]

            if self._plot:
                mp.plotSpectrum(q1_ws, 0, error_bars=True)
                mp.plotSpectrum(q2_ws, 0, error_bars=True)
                mp.plotSpectrum(eisf_ws, 0, error_bars=True)
                if self._msdfit:
                    mp.plotSpectrum(scan_ws + '_msd', 1, error_bars=True)

            if self._save:
                save_alg = self.createChildAlgorithm("SaveNexusProcessed", enableLogging=False)
                if self._msdfit:
                    output_workspaces.append(scan_ws + '_msd')
                    output_workspaces.append(scan_ws + '_msd_fit')
                for ws in output_workspaces:
                    file_path = os.path.join(workdir, ws + '.nxs')
                    save_alg.setProperty("InputWorkspace", ws)
                    save_alg.setProperty("Filename", file_path)
                    save_alg.execute()
                    logger.information('Output file : %s' % file_path)
                if self._msdfit:
                    for ws in [scan_ws + '_msd', scan_ws + '_msd_fit']:
                        file_path = os.path.join(workdir, ws + '.nxs')
                        save_alg.setProperty("InputWorkspace", ws)
                        save_alg.setProperty("Filename", file_path)
                        save_alg.execute()
                        logger.information('Output file : %s' % file_path)

    def _setup(self):
        self._run_first = self.getProperty('FirstRun').value
        self._run_last = self.getProperty('LastRun').value
        self._number_samples = self.getProperty('NumberSamples').value
        self._number_runs = (self._run_last - self._run_first + 1) / self._number_samples
        logger.information('Number of runs : %i' % self._number_runs)
        logger.information('Number of scans : %i' % self._number_samples)

        self._sum_files = False
        self._load_logs = True
        self._calibration_ws = ''

        self._instrument_name = self.getPropertyValue('Instrument')
        self._analyser = self.getPropertyValue('Analyser')
        self._reflection = self.getPropertyValue('Reflection')

        self._spectra_range = self.getProperty('SpectraRange').value
        self._elastic_range = self.getProperty('ElasticRange').value
        self._inelastic_range = self.getProperty('InelasticRange').value
        self._detailed_balance = Property.EMPTY_DBL

        self._grouping_method = self.getPropertyValue('GroupingMethod')
        self._grouping_ws = ''
        self._grouping_map_file = ''

        self._sample_log_name = 'Position'
        self._sample_log_value = 'last_value'

        self._msdfit = self.getProperty('MsdFit').value

        self._plot = self.getProperty('Plot').value
        self._save = self.getProperty('Save').value

    def validateInputs(self):
        self._setup()
        issues = dict()

        if self._run_first > self._run_last:
            issues["FirstRun"] = 'First run must be before last run'

        if self._number_runs < self._number_samples:
            issues["NumberSamples"] = 'There must be at least 1 run per sample'

        if self._spectra_range[0] > self._spectra_range[1]:
            issues['SpectraRange'] = 'Range must be in format: lower,upper'

        if self._msdfit and self._grouping_method == 'All':
            issues["MsdFit"] = 'Unable to perform MSDFit with "All" grouping method'

        return issues


AlgorithmFactory.subscribe(IndirectSampleChanger)  # Register algorithm with Mantid
