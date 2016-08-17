#pylint: disable=invalid-name,too-many-instance-attributes,too-many-branches,no-init
from IndirectImport import *

from mantid.api import (PythonAlgorithm, AlgorithmFactory, MatrixWorkspaceProperty, PropertyMode,
                        WorkspaceGroupProperty, Progress)
from mantid.kernel import StringListValidator, Direction
from mantid.simpleapi import *
from mantid import config, logger
import os
import numpy as np


class IndirectQuickRun(PythonAlgorithm):

    _instrument = None
    _analyser = None
    _reflection = None
    _run_numbers = None
    _calib_file = None
    _spectra_min = None 
    _spectra_max = None
    _eFixed = None

    

    def category(self):
        return "Workflow\\MIDAS"

    def summary(self):
        return "Runs a reduction and Elastic Window scan from multiple runs"
        
    def version(self):
        return 1

    def PyInit(self):
        ###==========================ISISIndirectEnergyTransfer inputs=======================###

        self.declareProperty(name='Instrument', defaultValue='IRIS',
                             validator=StringListValidator(['IRIS','OSIRIS', 'TOSCA']),
                             doc='The instrument')

        self.declareProperty(name='Analyser', defaultValue='graphite',
                             validator=StringListValidator(['graphite','mica', 'fmica']),
                             doc='The Analyser')

        self.declareProperty(name='Reflection', defaultValue='002',
                             validator=StringListValidator(['002','004']),
                             doc='The Reflection')
                             
        self.declareProperty(name='Runs', defaultValue='',
                             doc='Run numbers')
                             
        self.declareProperty(name='SumFiles', defaultValue=False,
                             doc='Sum input fun numbers')
                             
        self.declareProperty(name='CalibrationFile', defaultValue='',
                             doc='Optional calibration file')

        self.declareProperty(name='SpectraMin', defaultValue=3,
                             doc='Minimum spectra to load')
                             
        self.declareProperty(name='SpectraMax', defaultValue=50,
                             doc='Maximum spectra to load')
                             
        self.declareProperty(name='EFixed', defaultValue=1.845,
                             doc='eFixed value used when loading')

        ###============================ElasticWindowMultiple input===========================###
        self.declareProperty(name='IntegrationRangeStart', defaultValue=-0.2,
                             doc='Start of the integration range for Elwin')

        self.declareProperty(name='IntegrationRangeEnd', defaultValue=0.2,
                             doc='End of integration range for Elwin')

        self.declareProperty(name='BackgroundRangeStart', defaultValue='',
                             doc='Start of the bacground range for Elwin')

        self.declareProperty(name='BackgroundRangeEnd', defaultValue='',
                             doc='End of the background range for Elwin')

        self.declareProperty(name='SampleEnvironmentLogName', defaultValue='',
                             doc='Sample Enviroment Log Name for Elwin')

        self.declareProperty(name='SampleEnvironmentLogValue', defaultValue='',
                             doc='Sample Environment Log Value for Elwin')
                             
        ###=============================Output workspaces====================================###
        
        self.declareProperty(WorkspaceProperty('OutputInQ', '', Direction.Output),
                             doc='Output workspace in Q')

        self.declareProperty(WorkspaceProperty('OutputInQSquared', '', Direction.Output),
                             doc='Output workspace in Q Squared')

        self.declareProperty(WorkspaceProperty('OutputELF', '', Direction.Output,
                                               PropertyMode.Optional),
                             doc='Output workspace ELF')

        self.declareProperty(WorkspaceProperty('OutputELT', '', Direction.Output,
                                               PropertyMode.Optional),
                             doc='Output workspace ELT')



###============================= Validation functions ===============================### 
    def validateInputs(self):
        self._get_properties()
        run_number_str = self._parse_run_numbers()
        issues = dict()
        issues = self._validate_inst_setup(self._instrument, self._analyser,
                                           self._reflection, 
                                           self._spectra_min, 
                                           self._spectra_max,
                                           issues)
        issues = self._validate_run_numbers_are_ints(run_number_str, issues)
        issues = self._validate_run_numbers_for_instrument(self._spectra_min, 
                                                           self._spectra_max,
                                                           self._instrument,
                                                           issues)
        return issues


    def _validate_inst_setup(self, inst, ana, ref, spec_min, spec_max, issues):
        """
        Validates that the instrument/Analyser/Reflection is valid
        inst        :: Instrument name
        ana         :: Instrument analyser
        ref         :: Instrument reflection
        spec_min    :: Spectra Minimum 
        spec_max    :: Spectra Maximum
        issues      :: The dictionary issues for the validator
        """

        # All possible variants are valid for IRIS
        # OSIRIS
        if inst == 'OSIRIS':
            if ana != 'graphite':
                issues['Analyser'] = 'OSIRIS may only have graphite analysers'

        # TOSCA
        if inst == 'TOSCA':
            if ana != 'graphite':
                issues['Analyser'] = 'TOSCA may only have graphite analysers'
            if ref != '002':
                issues['Reflection'] = 'TOSCA may only have 002 reflection'

        return issues


    def _validate_run_numbers_are_ints(self, run_number_str, issues):
        """
        Parse string list to int list
        Ensure that run numbers were successfully parsed
        """
        # Ensure that the all run numbers can be parsed as integers
        run_number_list = []
        for number in run_number_str:
            try:
                run_number_list.append(int(number))
            except ValueError:
                issues['Runs'] = 'The run number entered could not be successfully parsed.'\
                                 'run numbers must be entered as a :\n'\
                                 'Range: 1- 2 \n'\
                                 'Comma list: 1,2'\
                                 'Combination: 1-2,3,4,5-6'
        self._run_numbers = run_number_list
        return issues

    def _validate_run_numbers_for_instrument(self, spec_min, spec_max, instrument, issues):
        """
        Ensure that the run number is valid for the instrument selected
        """
        #TODO: Implement
        return issues

###============================== Set up functions ==============================###
    def _get_properties(self):
        # ISISIndirectEnergyTransfer properties
        self._instrument = self.getPropertyValue('Instrument')
        self._analyser = self.getPropertyValue('Analyser')
        self._reflection = self.getPropertyValue('Reflection')
        self._run_numbers = self.getPropertyValue('Runs')
        self._sum_files = self.getPropertyValue('SumFiles')
        self._calib_file = self.getPropertyValue('CalibrationFile')
        self._spectra_min = self.getPropertyValue('SpectraMin') 
        self._spectra_max = self.getPropertyValue('SpectraMax')
        self._eFixed = self.getPropertyValue('EFixed')
        # Elwin properties 
        self._integration_start = self.getProperty('IntegrationRangeStart').value
        self._integration_end = self.getProperty('IntegrationRangeEnd').value
        self._background_start = self.getProperty('BackgroundRangeStart').value
        self._background_end = self.getProperty('BackgroundRangeEnd').value
        self._SE_log_name = self.getPropertyValue('SampleEnvironmentLogName')
        self._SE_log_value = self.getPropertyValue('SampleEnvironmentLogValue')


    def _parse_run_numbers(self):
        """
        Parses run numbers given in a range of formats
        """
        run_number_list = []
        comma_sep_list = self._run_numbers.split(',')
        for element in comma_sep_list:
            if '-' in element:
                range_string = element.split('-')
                run_number_list.append(range(int(range_string[0]), int(range_string[1])))
            else:
                run_number_list.append(element)
        return run_number_list


###============================== Execution functions ===========================###
    def PyExec(self):
        reduced_ws_group = self._reduce_data()
        self._run_elwin(reduced_ws_group)


    def _reduce_data(self):
        """
        Uses ISISIndirectEnergyTransfer to reduce the raw data 
        """
        reduce_alg = self.createChildAlgorithm('ISISIndirectEnergyTransfer')
        reduce_alg.setProperty('InputFiles', self._run_numbers)
        reduce_alg.setProperty('SumFiles', self._sum_files)
        reduce_alg.setProperty('LoadLogFiles', False)
        reduce_alg.setProperty('CalibrationWorkspace', self._calib_file)
        reduce_alg.setProperty('Instrument', self._instrument)
        reduce_alg.setProperty('Analyser', self._analyser)
        reduce_alg.setProperty('Reflection', self._reflection)
        reduce_alg.setProperty('Efixed', self._eFixed)
        reduce_alg.setProperty('SpectraRange', self._spectra_range)
        reduce_alg.execute()
        
        return reduce_alg.getProperty('OutputWorkspace').value


    def _run_elwin(self, reduced_ws_group):
        """
        Runs reduced files through Elwin 
        """
        elwin_alg = self.createChildAlgorithm('ElasticWindowMultiple')
        elwin_alg.setProperty('InputWorksapces', reduced_ws_group)
        elwin_alg.setProperty('IntegrationRangeStart', self._integration_start)
        elwin_alg.setProperty('IntegrationRangeEnd', self._integration_end)
        elwin_alg.setProperty('BackgroundRangeStart', self._background_start)
        elwin_alg.setProperty('BackgroundRangeEnd', self._background_end)
        elwin_alg.setProperty('SampleEnvironmentLogName', self._SE_log_name)
        elwin_alg.setProperty('SampleEnvironmentLogValue', self._SE_log_value)
        elwin_alg.setProperty('OutputInQ', self.getProperty('OutputInQ').value)
        elwin_alg.setProperty('OutputInQSquared', self.getProperty('OutputInQSquared').value)
        elwin_alg.setProperty('OutputELF', self.getProperty('OutputELF').value)
        elwin_alg.setProperty('OutputELT', self.getProperty('OutputELT').value)
        elwin_alg.execute()
        
        # Set output of quick run to elwin output
        self.setProperty('OutputInQ', elwin_alg.getProperty('OutputInQ').value)
        self.setProperty('OutputInQSquared', elwin_alg.getProperty('OutputInQSquared').value)
        self.setProperty('OutputELF', elwin_alg.getProperty('OutputELF').value)
        self.setProperty('OutputELT', elwin_alg.getProperty('OutputELT').value)


AlgorithmFactory.subscribe(IndirectQuickRun)
