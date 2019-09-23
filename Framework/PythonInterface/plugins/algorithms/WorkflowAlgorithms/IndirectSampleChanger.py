# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.api import (AlgorithmFactory, DataProcessorAlgorithm)
from mantid.kernel import (FloatArrayLengthValidator, FloatArrayProperty, IntArrayLengthValidator, IntArrayProperty,
                           IntBoundedValidator, StringListValidator)


class IndirectSampleChanger(DataProcessorAlgorithm):
    _instrument_name = None
    _number_runs = 1
    _runs = None
    _temperature = None
    _spectra_range = None
    _elastic_range = None
    _inelastic_range = None
    _total_range = None
    _sample_log_name = None
    _sample_log_value = None
    _msd_fit = False
    _width_fit = False
    _q1_workspaces = None
    _plot = False
    _save = False

    def category(self):
        return "Workflow\\MIDAS"

    def summary(self):
        return "Create elastic window scans for sample changer"

    def PyInit(self):
        self.declareProperty(name='Instrument', defaultValue='IRIS',
                             validator=StringListValidator(['IRIS', 'OSIRIS']),
                             doc='The name of the instrument.')
        self.declareProperty(name='Analyser', defaultValue='',
                             validator=StringListValidator(['graphite', 'mica', 'fmica']),
                             doc='The analyser bank used during run.')
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
                             doc='Comma separated range of spectra numbers to use.')
        self.declareProperty(FloatArrayProperty(name='ElasticRange',
                                                validator=FloatArrayLengthValidator(2)),
                             doc='Energy range for the elastic component.')
        self.declareProperty(FloatArrayProperty(name='InelasticRange',
                                                validator=FloatArrayLengthValidator(2)),
                             doc='Energy range for the inelastic component.')
        self.declareProperty(FloatArrayProperty(name='TotalRange',
                                                validator=FloatArrayLengthValidator(2)),
                             doc='Energy range for the total energy component.')

        self.declareProperty(name='SampleEnvironmentLogName', defaultValue='Position',
                             doc='Name of the sample environment log entry')

        sample_environment_log_values = ['last_value', 'average']
        self.declareProperty('SampleEnvironmentLogValue', 'last_value',
                             StringListValidator(sample_environment_log_values),
                             doc='Value selection of the sample environment log entry')

        self.declareProperty(name='MSDFit', defaultValue=False,
                             doc='Perform an MSDFit. Do not use with GroupingMethod as "All"')

        self.declareProperty(name='WidthFit', defaultValue=False,
                             doc='Perform a 2 peak width Fit. Do not use with GroupingMethod as "All"')

        self.declareProperty(name='Plot', defaultValue=False,
                             doc='True to plot the output data.')
        self.declareProperty(name='Save', defaultValue=False,
                             doc='True to save the output data.')

    def validateInputs(self):
        from IndirectReductionCommon import get_ipf_parameters_from_run

        self._setup()
        issues = dict()

        if self._run_first > self._run_last:
            issues["FirstRun"] = 'First run must be before last run'

        if self._number_runs < self._number_samples:
            issues["NumberSamples"] = 'There must be at least 1 run per sample'

        if self._spectra_range[0] > self._spectra_range[1]:
            issues['SpectraRange'] = 'Range must be in format: lower,upper'
        else:
            spectra_parameters = get_ipf_parameters_from_run(self._run_first, self._instrument_name, self._analyser,
                                                             self._reflection, ['spectra-min', 'spectra-max'])
            if 'spectra-min' in spectra_parameters and 'spectra-max' in spectra_parameters:
                if self._spectra_range[0] < spectra_parameters['spectra-min'] or \
                        self._spectra_range[1] > spectra_parameters['spectra-max']:
                    issues['SpectraRange'] = 'The spectra range must be between {0} and {1} for the {2} instrument'.format(
                        str(int(spectra_parameters['spectra-min'])), str(int(spectra_parameters['spectra-max'])),
                        self._instrument_name)

        return issues

    def _setup(self):
        self._run_first = self.getProperty('FirstRun').value
        self._run_last = self.getProperty('LastRun').value
        self._number_samples = self.getProperty('NumberSamples').value
        self._number_runs = int((self._run_last - self._run_first + 1) / self._number_samples)

        self._instrument_name = self.getPropertyValue('Instrument')
        self._analyser = self.getPropertyValue('Analyser')
        self._reflection = self.getPropertyValue('Reflection')

        self._spectra_range = self.getProperty('SpectraRange').value
        self._elastic_range = self.getProperty('ElasticRange').value
        self._inelastic_range = self.getProperty('InelasticRange').value
        self._total_range = self.getProperty('TotalRange').value

        self._sample_log_name = self.getPropertyValue('SampleEnvironmentLogName')
        self._sample_log_value = self.getPropertyValue('SampleEnvironmentLogValue')

        self._msd_fit = self.getProperty('MsdFit').value
        self._width_fit = self.getProperty('WidthFit').value

        self._plot = self.getProperty('Plot').value
        self._save = self.getProperty('Save').value

    def PyExec(self):
        self._setup()

        quick_run_alg = self.createChildAlgorithm("IndirectQuickRun", 0.05, 0.95)
        for num in range(self._number_samples):
            first_run = self._run_first + num
            run_numbers = [str(first_run + idx * self._number_samples) for idx in range(self._number_runs)]

            quick_run_alg.setProperty('InputFiles', run_numbers)
            quick_run_alg.setProperty('Instrument', self._instrument_name)
            quick_run_alg.setProperty('Analyser', self._analyser)
            quick_run_alg.setProperty('Reflection', self._reflection)
            quick_run_alg.setProperty('SpectraRange', self._spectra_range)
            quick_run_alg.setProperty('ElasticRange', self._elastic_range)
            quick_run_alg.setProperty('InelasticRange', self._inelastic_range)
            quick_run_alg.setProperty('TotalRange', self._total_range)
            quick_run_alg.setProperty('SampleEnvironmentLogName', self._sample_log_name)
            quick_run_alg.setProperty('SampleEnvironmentLogValue', self._sample_log_value)
            quick_run_alg.setProperty('MSDFit', self._msd_fit)
            quick_run_alg.setProperty('WidthFit', self._width_fit)
            quick_run_alg.setProperty('Plot', self._plot)
            quick_run_alg.setProperty('Save', self._save)
            quick_run_alg.execute()


AlgorithmFactory.subscribe(IndirectSampleChanger)  # Register algorithm with Mantid
