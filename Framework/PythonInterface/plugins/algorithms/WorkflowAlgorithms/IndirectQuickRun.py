# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=too-many-locals
from mantid.api import (AlgorithmFactory, AnalysisDataService, DataProcessorAlgorithm, MatrixWorkspace, NumericAxis,
                        Progress)
from mantid.simpleapi import (CreateWorkspace, DeleteWorkspace, GroupWorkspaces, mtd, IndirectTwoPeakFit,
                              SaveNexusProcessed)
from mantid.kernel import (FloatArrayLengthValidator, FloatArrayProperty, IntArrayMandatoryValidator, IntArrayProperty,
                           Property, StringArrayProperty, StringListValidator)

from mantid import config, logger

import os


def exists_in_ads(workspace_name):
    return AnalysisDataService.doesExist(workspace_name)


def save_workspace(workspace_name, filename):
    SaveNexusProcessed(InputWorkspace=workspace_name, Filename=filename, EnableLogging=False)


def save_workspaces_in_group(group_name, file_path):
    workspace_group = mtd[group_name]
    for workspace_name in workspace_group.getNames():
        save_workspace(workspace_name, file_path + workspace_name + '.nxs')


def group_workspaces(input_workspaces, group_name):
    GroupWorkspaces(InputWorkspaces=input_workspaces, OutputWorkspace=group_name, EnableLogging=False)


def create_workspace(x_data, y_data, e_data, number_of_histograms, y_label, output_name):
    CreateWorkspace(DataX=x_data, DataY=y_data, DataE=e_data, NSpec=number_of_histograms, YUnitLabel=y_label,
                    OutputWorkspace=output_name, EnableLogging=False)


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
    _msd_fit = False
    _width_fit = False
    _output_ws = None
    _scan_ws = None
    _ipf_filename = None
    _plot = False
    _save = False

    _temperatures = None

    def category(self):
        return 'Workflow\\Inelastic;Inelastic\\Indirect;Workflow\\MIDAS'

    def summary(self):
        return 'Runs an energy transfer reduction for an inelastic indirect geometry instrument.'

    def PyInit(self):
        # Input properties
        self.declareProperty(StringArrayProperty(name='InputFiles'),
                             doc='A comma separated list of run numbers.')

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
                             doc='Energy range for the elastic component.')
        self.declareProperty(FloatArrayProperty(name='InelasticRange',
                                                validator=FloatArrayLengthValidator(2)),
                             doc='Energy range for the inelastic component.')
        self.declareProperty(FloatArrayProperty(name='TotalRange',
                                                validator=FloatArrayLengthValidator(2)),
                             doc='Energy range for the total energy component.')

        self.declareProperty(name='SampleEnvironmentLogName', defaultValue='sample',
                             doc='Name of the sample environment log entry')

        sample_environment_log_values = ['last_value', 'average']
        self.declareProperty('SampleEnvironmentLogValue', 'last_value',
                             StringListValidator(sample_environment_log_values),
                             doc='Value selection of the sample environment log entry')

        self.declareProperty(name='MSDFit', defaultValue=False,
                             doc='Perform an MSDFit.')

        self.declareProperty(name='WidthFit', defaultValue=False,
                             doc='Perform a 2 peak width Fit.')

        self.declareProperty(name='Plot', defaultValue=False,
                             doc='Switch Plot Off/On')
        self.declareProperty(name='Save', defaultValue=False,
                             doc='Switch Save result to nxs file Off/On')

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
        self._instrument_name = self.getPropertyValue('Instrument')

        runs = self.getProperty('InputFiles').value
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

        self._msd_fit = self.getProperty('MSDFit').value

        self._width_fit = self.getProperty('WidthFit').value

        self._output_ws = first_file + '_to_' + last_file + '_scan_red'
        self._scan_ws = first_file + '_to_' + last_file + '_scan'

        self._plot = self.getProperty('Plot').value
        self._save = self.getProperty('Save').value

        # Get the IPF filename
        self._ipf_filename = os.path.join(config['instrumentDefinition.directory'],
                                          self._instrument_name + '_' + self._analyser + '_' + self._reflection + '_Parameters.xml')
        logger.information('Instrument parameter file: %s' % self._ipf_filename)

    def PyExec(self):
        self._setup()

        progress_end = 0.3 if self._width_fit else 1.0
        progress_tracker = Progress(self, 0.0, progress_end, 1)
        progress_tracker.report('Running EnergyWindowScan...')
        self._energy_window_scan()
        self._group_energy_window_scan_output()

        if self._width_fit:
            self._temperatures = self._get_temperatures()
            self._perform_width_fit()

        if self._plot:
            self._plot_output()

        if self._save:
            self._save_output()

    def _energy_window_scan(self):
        scan_algorithm = self.createChildAlgorithm("EnergyWindowScan", 0.0, 0.3)
        scan_algorithm.setProperty('InputFiles', self._data_files)
        scan_algorithm.setProperty('Instrument', self._instrument_name)
        scan_algorithm.setProperty('Analyser', self._analyser)
        scan_algorithm.setProperty('Reflection', self._reflection)
        scan_algorithm.setProperty('SpectraRange', self._spectra_range)
        scan_algorithm.setProperty('ElasticRange', self._elastic_range)
        scan_algorithm.setProperty('InelasticRange', self._inelastic_range)
        scan_algorithm.setProperty('TotalRange', self._total_range)
        scan_algorithm.setProperty('DetailedBalance', Property.EMPTY_DBL)
        scan_algorithm.setProperty('SampleEnvironmentLogName', self._sample_log_name)
        scan_algorithm.setProperty('SampleEnvironmentLogValue', self._sample_log_value)
        scan_algorithm.setProperty('MSDFit', self._msd_fit)
        scan_algorithm.setProperty('ReducedWorkspace', self._output_ws)
        scan_algorithm.setProperty('ScanWorkspace', self._scan_ws)
        scan_algorithm.execute()

    def _group_energy_window_scan_output(self):
        """
        Group the output workspaces from the ElasticWindowScan algorithm.
        """
        suffixes = ['_el_elf', '_inel_elf', '_total_elf', '_el_elt', '_inel_elt', '_total_elt', '_el_eq1', '_inel_eq1',
                    '_total_eq1', '_el_eq2', '_inel_eq2', '_total_eq2']
        energy_window_scan_workspaces = [self._scan_ws + suffix for suffix in suffixes
                                         if exists_in_ads(self._scan_ws + suffix)]

        group_workspaces(energy_window_scan_workspaces, self._scan_ws + '_q')

    def _get_temperatures(self):
        elf_workspace_name = self._scan_ws + '_total_elf'

        if exists_in_ads(elf_workspace_name):
            elf_workspace = mtd[elf_workspace_name]
            if isinstance(elf_workspace, MatrixWorkspace):
                return elf_workspace.dataX(0)
        return None

    def _perform_width_fit(self):
        input_workspace_names = mtd[self._output_ws].getNames()
        x = mtd[input_workspace_names[0]].readX(0)

        # Perform the two peak fits on the input workspaces
        result_workspaces, chi_workspaces, run_numbers = self._perform_two_peak_fits(input_workspace_names, x[0],
                                                                                     x[len(x) - 1])

        # Find the units of the x axis
        x_axis_is_temperature = len(input_workspace_names) == len(self._temperatures)

        # Create the width workspace
        width_name = self._output_ws + '_Width1'
        self._create_width_workspace(result_workspaces, self._temperatures, run_numbers, width_name, x_axis_is_temperature)

        # Create the diffusion workspace
        diffusion_name = self._output_ws + '_Diffusion'
        self._create_diffusion_workspace(mtd[width_name], self._temperatures, run_numbers, diffusion_name, x_axis_is_temperature)

        # Group the width fit workspaces
        group_workspaces(result_workspaces + chi_workspaces + [width_name] + [diffusion_name], self._output_ws + '_Width_Fit')

    def _perform_two_peak_fits(self, workspace_names, x_min, x_max):
        result_workspaces = []
        chi_workspaces = []
        run_numbers = []
        for workspace_name in workspace_names:
            number_of_histograms = mtd[workspace_name].getNumberHistograms()

            progress_tracker = Progress(self, 0.3, 1.0, number_of_histograms+1)
            progress_tracker.report('Finding temperature for {0}...'.format(workspace_name))

            if self._temperatures is None:
                run_number = str(mtd[workspace_name].getRunNumber())
                run_numbers.append(run_number)

            result = workspace_name[:-3] + 'fit'

            for index in range(number_of_histograms):
                progress_tracker.report('Fitting {0}-sp{1}...'.format(workspace_name, index))
                IndirectTwoPeakFit(SampleWorkspace=workspace_name,
                                   EnergyMin=x_min,
                                   EnergyMax=x_max,
                                   OutputName=result)

            result_workspaces.append(result + '_Result')
            chi_workspaces.append(result + '_ChiSq')
        return result_workspaces, chi_workspaces, run_numbers

    def _create_width_workspace(self, result_workspaces, temperatures, run_numbers, output_name, x_axis_is_temperature):
        self._extract(result_workspaces[0], 0, output_name)
        for index, workspace in enumerate(result_workspaces[1:]):
            self._extract(workspace, 0, '__spectrum')
            self._append(output_name, '__spectrum', output_name)

        DeleteWorkspace(Workspace='__spectrum')
        self._format_width_workspace(mtd[output_name], temperatures, run_numbers, x_axis_is_temperature)

    @staticmethod
    def _format_width_workspace(width_workspace, temperatures, run_numbers, x_axis_is_temperature):
        number_of_temperatures = len(temperatures)
        axis = NumericAxis.create(number_of_temperatures)
        for index in range(number_of_temperatures):
            # The slice here is to make the plot versus number less cluttered/messy when using 5 or more digits.
            value = float(temperatures[index]) if x_axis_is_temperature else float(run_numbers[index][-3:])
            axis.setValue(index, value)
        width_workspace.replaceAxis(1, axis)
        width_workspace.setYUnitLabel("Temperature")

    def _create_diffusion_workspace(self, width_workspace, temperatures, run_numbers, output_name, x_axis_is_temperature):
        x_data = []
        y_data = []
        e_data = []
        for index in range(width_workspace.getNumberHistograms()):
            x = width_workspace.readX(index)
            y = width_workspace.readY(index)
            e = width_workspace.readE(index)
            # The slice here is to make the plot versus number less cluttered/messy when using 5 or more digits.
            x_data.append(float(temperatures[index])) if x_axis_is_temperature else x_data.append(float(run_numbers[index][-3:]))
            # The 5th spectrum is taken here as only a single data point is needed for the fit and the first is often
            # unreliable.
            y_data.append(y[5] / x[5])
            e_data.append(e[5] / x[5])

        self._create_and_format_diffusion_workspace(x_data, y_data, e_data, output_name, x_axis_is_temperature)

    @staticmethod
    def _create_and_format_diffusion_workspace(x_data, y_data, e_data, output_name, x_axis_is_temperature):
        create_workspace(x_data, y_data, e_data, 1, 'Diffusion', output_name)

        unit = ('Temperature', 'K') if x_axis_is_temperature else ('Run No', 'last 3 digits')
        x_axis = mtd[output_name].getAxis(0).setUnit("Label")
        x_axis.setLabel(unit[0], unit[1])

    def _extract(self, input_ws, index, output_ws):
        extract_alg = self.createChildAlgorithm("ExtractSingleSpectrum", enableLogging=False)
        extract_alg.setProperty("InputWorkspace", input_ws)
        extract_alg.setProperty("WorkspaceIndex", index)
        extract_alg.setProperty("OutputWorkspace", output_ws)
        extract_alg.execute()
        mtd.addOrReplace(output_ws, extract_alg.getProperty("OutputWorkspace").value)

    def _append(self, input1_ws, input2_ws, output_ws):
        append_alg = self.createChildAlgorithm("AppendSpectra", enableLogging=False)
        append_alg.setProperty("InputWorkspace1", input1_ws)
        append_alg.setProperty("InputWorkspace2", input2_ws)
        append_alg.setProperty("OutputWorkspace", output_ws)
        append_alg.execute()
        mtd.addOrReplace(output_ws, append_alg.getProperty("OutputWorkspace").value)

    def _save_output(self):
        save_directory = config['defaultsave.directory']

        save_workspaces_in_group(self._scan_ws + '_eq1', save_directory)
        save_workspaces_in_group(self._scan_ws + '_eq2', save_directory)
        save_workspace(self._scan_ws + '_eisf', save_directory + self._scan_ws + '_eisf.nxs')

        if self._msd_fit:
            save_workspace(self._scan_ws + '_msd', save_directory + self._scan_ws + '_msd.nxs')
            save_workspaces_in_group(self._scan_ws + '_msd_fit', save_directory)

        if self._width_fit:
            save_workspace(self._scan_ws + '_red_Diffusion', save_directory + self._scan_ws + '_red_Diffusion.nxs')
            save_workspace(self._scan_ws + '_red_Width1', save_directory + self._scan_ws + '_red_Width1.nxs')

    def _plot_output(self):
        workspace_names = [self._scan_ws + '_el_eq1', self._scan_ws + '_inel_eq1', self._scan_ws + '_total_eq1',
                           self._scan_ws + '_el_eq2', self._scan_ws + '_inel_eq2', self._scan_ws + '_total_eq2',
                           self._scan_ws + '_eisf']
        try:
            from mantidplot import plotSpectrum
            for workspace_name in workspace_names:
                plotSpectrum(workspace_name, 0, error_bars=True)

            if self._msd_fit:
                plotSpectrum(self._scan_ws + '_msd', 1, error_bars=True)
            if self._width_fit:
                plotSpectrum(self._output_ws + '_Diffusion', 0, error_bars=True)
        except ImportError:
            from mantidqt.plotting.functions import plot
            plot(workspace_names, wksp_indices=[0]*len(workspace_names), errors=True)
            if self._msd_fit:
                plot([self._scan_ws + '_msd'], wksp_indices=[1], errors=True)
            if self._width_fit:
                plot([self._output_ws + '_Diffusion'], wksp_indices=[0], errors=True)

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
