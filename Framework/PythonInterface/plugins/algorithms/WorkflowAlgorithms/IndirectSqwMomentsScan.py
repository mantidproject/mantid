
from mantid.kernel import *
from mantid.api import *
from mantid.simpleapi import *
from mantid import config

import os


class RunSQWscan(DataProcessorAlgorithm):
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
                             doc='Comma separated list of input files')

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
        self.declareProperty(FloatArrayProperty(name='QRange'),
                             doc='Range of background to subtract from raw data in time of flight.')
        self.declareProperty(FloatArrayProperty(name='EnergyRange'),
                             doc='Range of background to subtract from raw data in time of flight.')
        self.declareProperty(name='DetailedBalance', defaultValue=Property.EMPTY_DBL,
                             doc='')

        self.declareProperty(name='SampleEnvironmentLogName', defaultValue='sample',
                             doc='Name of the sample environment log entry')

        sampEnvLogVal_type = ['last_value', 'average']
        self.declareProperty('SampleEnvironmentLogValue', 'last_value',
                             StringListValidator(sampEnvLogVal_type),
                             doc='Value selection of the sample environment log entry')

        self.declareProperty(name='Plot', defaultValue=False,
                             doc='Switch Plot Off/On')
        self.declareProperty(name='Save', defaultValue=False,
                             doc='Switch Save result to nxs file Off/On')

    def PyExec(self):
        setup_progress = Progress(self, 0.0, 0.05, 3)
        setup_progress.report('Getting Parameters')
        self._setup()

        scan_alg = self.createChildAlgorithm("SqwMomentsScan", 0.05, 0.95)
        scan_alg.setProperty('InputFiles', self._data_files)
        scan_alg.setProperty('LoadLogFiles', self._load_logs)
        scan_alg.setProperty('CalibrationWorkspace', self._calibration_ws)
        scan_alg.setProperty('Instrument', self._instrument_name)
        scan_alg.setProperty('Analyser', self._analyser)
        scan_alg.setProperty('Reflection', self._reflection)
        scan_alg.setProperty('SpectraRange', self._spectra_range)
        scan_alg.setProperty('QRange', self._q_range)
        scan_alg.setProperty('EnergyRange', self._energy_range)
        scan_alg.setProperty('DetailedBalance', self._detailed_balance)
        scan_alg.setProperty('GroupingMethod', self._grouping_method)
        scan_alg.setProperty('SampleEnvironmentLogName', self._sample_log_name)
        scan_alg.setProperty('SampleEnvironmentLogValue', self._sample_log_value)
        scan_alg.setProperty('ReducedWorkspace', self._output_ws)
        scan_alg.setProperty('SqwWorkspace', self._sqw_ws)
        scan_alg.setProperty('MomentWorkspace', self._moment_ws)
        scan_alg.execute()

        logger.information('ReducedWorkspace : %s' % self._output_ws)
        logger.information('ScanWorkspace : %s' % self._sqw_ws)
        logger.information('MomentWorkspace : %s' % self._moment_ws)

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

        return issues

    def _setup(self):
        """
        Gets algorithm properties.
        """

        # Get properties
        self._instrument_name = self.getPropertyValue('Instrument')

        runs = self.getProperty('RunNumbers').value
        self._data_files = []
        for idx in range(len(runs)):
            self._data_files.append(self._instrument_name.lower() + runs[idx])
        first_file = self._data_files[0]
        self._sum_files = False
        self._load_logs = self.getProperty('LoadLogFiles').value
        self._calibration_ws = ''

        self._analyser = self.getPropertyValue('Analyser')
        self._reflection = self.getPropertyValue('Reflection')

        self._spectra_range = self.getProperty('SpectraRange').value
        self._q_range = self.getProperty('QRange').value
        self._energy_range = self.getProperty('EnergyRange').value
        self._detailed_balance = self.getProperty('DetailedBalance').value

        self._grouping_method = 'Individual'
        self._grouping_ws = ''
        self._grouping_map_file = ''

        self._sample_log_name = self.getPropertyValue('SampleEnvironmentLogName')
        self._sample_log_value = self.getPropertyValue('SampleEnvironmentLogValue')

        self._output_ws = first_file + '_sqw_scan_red'
        self._sqw_ws = first_file + '_sqw_scan'
        self._moment_ws = first_file + '_sqw_scan_mom'

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
        workdir = config['defaultsave.directory']
        first_file = self._data_files[0]
        red_path = os.path.join(workdir, self._output_ws + '.nxs')
        logger.information('Creating red file : %s' % red_path)
        SaveNexusProcessed(InputWorkspace=self._output_ws,
                           Filename=red_path,
                           EnableLogging=False)
        sqw_path = os.path.join(workdir, self._sqw_ws + '.nxs')
        logger.information('Creating sqw file : %s' % sqw_path)
        SaveNexusProcessed(InputWorkspace=self._sqw_ws,
                           Filename=sqw_path,
                           EnableLogging=False)
        mom_path = os.path.join(workdir, self._moment_ws + '.nxs')
        logger.information('Creating mom file : %s' % mom_path)
        SaveNexusProcessed(InputWorkspace=self._moment_ws,
                           Filename=mom_path,
                           EnableLogging=False)
        width_path = os.path.join(workdir, self._moment_ws + '_width.nxs')
        logger.information('Creating width file : %s' % width_path)
        SaveNexusProcessed(InputWorkspace=self._moment_ws + '_width',
                           Filename=width_path,
                           EnableLogging=False)
        diff_path = os.path.join(workdir, self._moment_ws + '_diffusion.nxs')
        logger.information('Creating diffusion file : %s' % diff_path)
        SaveNexusProcessed(InputWorkspace=self._moment_ws + '_diffusion',
                           Filename=diff_path,
                           EnableLogging=False)

    def _plot_result(self):
        import mantidplot as mp
        nhist = mtd[self._moment_ws + '_width'].getNumberHistograms()
        width_plot = mp.plotSpectrum(self._moment_ws + '_width', 0, error_bars=True)
        diff_plot = mp.plotSpectrum(self._moment_ws + '_diffusion', 0, error_bars=True)


# Register algorithm with Mantid
AlgorithmFactory.subscribe(RunSQWscan)
