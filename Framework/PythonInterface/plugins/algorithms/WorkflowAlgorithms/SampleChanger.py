from mantid.simpleapi import *
from mantid.api import *
from mantid.kernel import *
from mantid import config
import sys
import math
import os.path
import numpy as np


class SampleChanger(DataProcessorAlgorithm):
    _plot = False
    _save = False
    _instrument = 'IRIS'
    _number_runs = 1
    _runs = None
    _temperature = None
    _q1_workspaces = None

    def category(self):
        return "Workflow\\MIDAS;PythonAlgorithms"

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

        self.declareProperty(name='FirstRun', defaultValue=1,
                             doc="First Sample runnumber.")
        self.declareProperty(name='LastRun', defaultValue=2,
                             doc="Last Sample runnumber.")
        self.declareProperty(name='NumberSamples', defaultValue=1,
                             doc="Increment for runnumber.")
        self.declareProperty(name='LoadLogFiles', defaultValue=True,
                             doc='Load log files when loading runs')

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

        self.declareProperty(name='MsdFit', defaultValue=False,
                             doc='Run msd fit')

        self.declareProperty(name='Save', defaultValue=False,
                             doc='Switch Save result to nxs file Off/On')
        self.declareProperty(name='Plot', defaultValue=False,
                             doc='Plot options')

    def PyExec(self):
        import mantidplot as mp
        workdir = config['defaultsave.directory']
        self._setup()

        q2_workspaces = []
        msd_plot = list()
        scan_alg = self.createChildAlgorithm("EnergyWindowScan", 0.05, 0.95)
        for numb in range(self._number_samples):
            run_numbers = []
            run_names = []
            first_run = self._run_first + numb
            for idx in range(self._number_runs):
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

            if self._msdfit:
                msd_ws = scan_ws + '_msd'
                msd_output_workspaces = [msd_ws, msd_ws + '_fit']
                msd_plot.append(msd_ws)

            if self._plot:
                mp.plotSpectrum(q1_ws, 0, error_bars=True)
                mp.plotSpectrum(q2_ws, 0, error_bars=True)
                mp.plotSpectrum(eisf_ws, 0, error_bars=True)

            if self._save:
                save_alg = self.createChildAlgorithm("SaveNexusProcessed", enableLogging=False)
                for ws in output_workspaces:
                    file_path = os.path.join(workdir, ws + '.nxs')
                    save_alg.setProperty("InputWorkspace", ws)
                    save_alg.setProperty("Filename", file_path)
                    save_alg.execute()
                    logger.information('Output file : %s' % file_path)
                if self._msdfit:
                    for ws in msd_output_workspaces:
                        file_path = os.path.join(workdir, ws + '.nxs')
                        save_alg.setProperty("InputWorkspace", ws)
                        save_alg.setProperty("Filename", file_path)
                        save_alg.execute()
                        logger.information('Output file : %s' % file_path)
        if self._plot:
            if self._msdfit:
                mp.plotSpectrum(msd_plot, 1, error_bars=True)

    def _setup(self):
        self._run_first = self.getProperty('FirstRun').value
        self._run_last = self.getProperty('LastRun').value
        self._number_samples = self.getProperty('NumberSamples').value
        self._number_runs = (self._run_last - self._run_first + 1) / self._number_samples
        logger.information('Number of runs : %i' % self._number_runs)
        logger.information('Number of scans : %i' % self._number_samples)

        self._sum_files = False
        self._load_logs = self.getProperty('LoadLogFiles').value
        self._calibration_ws = ''

        self._instrument_name = self.getPropertyValue('Instrument')
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

        self._msdfit = self.getProperty('MsdFit').value

        self._plot = self.getProperty('Plot').value
        self._save = self.getProperty('Save').value

    def _msd_fit(self, ws, ws_name):
        # Fit line to each of the spectra
        function = 'name=LinearBackground, A0=0, A1=0'
        input_params = [ws + ',i%d' % i for i in range(0, self._number_runs)]
        input_params = ';'.join(input_params)
        x = mtd[ws].readX(0)
        PlotPeakByLogValue(Input=input_params,
                           OutputWorkspace=ws_name,
                           Function=function,
                           StartX=x[0], EndX=x[-1],
                           FitType='Sequential',
                           CreateOutput=True)

        delete_alg = self.createChildAlgorithm("DeleteWorkspace", enableLogging=False)
        delete_alg.setProperty("Workspace", ws_name + '_NormalisedCovarianceMatrices')
        delete_alg.execute()
        delete_alg.setProperty("Workspace", ws_name + '_Parameters')
        delete_alg.execute()
        rename_alg = self.createChildAlgorithm("RenameWorkspace", enableLogging=False)
        rename_alg.setProperty("InputWorkspace", ws_name)
        rename_alg.setProperty("OutputWorkspace", ws_name + '_Parameters')
        rename_alg.execute()
        mtd.addOrReplace(ws_name + '_Parameters', rename_alg.getProperty("OutputWorkspace").value)

        params_table = mtd[ws_name + '_Parameters']

        # MSD value should be positive, but the fit output is negative
        msd = params_table.column('A1')
        for i, value in enumerate(msd):
            params_table.setCell('A1', i, value * -1)

        # Create workspaces for each of the parameters
        parameter_ws_group = []

        # A0 workspace
        msd_name = ws_name + '_A0'
        parameter_ws_group.append(msd_name)
        ConvertTableToMatrixWorkspace(ws_name + '_Parameters', OutputWorkspace=msd_name,
                                      ColumnX='axis-1', ColumnY='A0', ColumnE='A0_Err')
        xunit = mtd[msd_name].getAxis(0).setUnit('Label')
        xunit.setLabel('Temperature', 'K')

        # A1 workspace
        msd_name = ws_name + '_A1'
        parameter_ws_group.append(msd_name)
        ConvertTableToMatrixWorkspace(ws_name + '_Parameters', OutputWorkspace=msd_name,
                                      ColumnX='axis-1', ColumnY='A1', ColumnE='A1_Err')
        xunit = mtd[msd_name].getAxis(0).setUnit('Label')
        xunit.setLabel('Temperature', 'K')
        sort_alg = self.createChildAlgorithm("SortXAxis", enableLogging=False)
        sort_alg.setProperty("InputWorkspace", msd_name)
        sort_alg.setProperty("OutputWorkspace", msd_name)
        sort_alg.execute()
        mtd.addOrReplace(msd_name, sort_alg.getProperty("OutputWorkspace").value)

        # Group parameter workspaces
        group_alg = self.createChildAlgorithm("GroupWorkspaces", enableLogging=False)
        group_alg.setProperty("InputWorkspaces", ','.join(parameter_ws_group))
        group_alg.setProperty("OutputWorkspace", ws_name)
        group_alg.execute()
        mtd.addOrReplace(ws_name, group_alg.getProperty("OutputWorkspace").value)

        # Add sample logs to output workspace
        copy_log_alg = self.createChildAlgorithm("CopyLogs", enableLogging=False)
        copy_log_alg.setProperty("InputWorkspace", ws)
        copy_log_alg.setProperty("OutputWorkspace", ws_name)
        copy_log_alg.execute()
        copy_log_alg.setProperty("InputWorkspace", ws_name + '_A0')
        copy_log_alg.setProperty("OutputWorkspace", ws_name + '_Workspaces')
        copy_log_alg.execute()


AlgorithmFactory.subscribe(SampleChanger)  # Register algorithm with Mantid
