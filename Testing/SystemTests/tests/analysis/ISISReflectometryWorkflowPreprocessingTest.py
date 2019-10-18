# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
"""
System Test for ISIS Reflectometry autoreduction
Adapted from scripts provided by Max Skoda.
"""
from ISISReflectometryWorkflowBase import *


class ISISReflectometryWorkflowPreprocessing(ISISReflectometryWorkflowBase):
    '''
    Script to test that the ISIS Reflectometry workflow successfully performs
    required preprocessing of input runs and transmission runs before it
    performs the reduction, when those inputs are not already in the ADS.
    '''
    
    run_numbers = ['44319']
    first_transmission_run = ['44297']
    second_transmission_run = ['44296']
    input_runs_file = None

    reference_result_file = 'ISISReflectometryWorkflowPreprocessingResult.nxs'
    result_workspace = 'Result'

    def __init__(self):
        super(ISISReflectometryWorkflowPreprocessing, self).__init__()

    def runTest(self):
        setupTest(self)

        reduceRun(run_number = self.run_numbers, angle = 0.7,
                  first_trans_runs = self.first_transmission_run,
                  second_trans_runs = self.second_transmission_run)

        self.finaliseResults()
