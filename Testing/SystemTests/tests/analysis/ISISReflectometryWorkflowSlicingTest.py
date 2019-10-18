# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from ISISReflectometryWorkflowBase import *

class ISISReflectometryWorkflowSlicingTest(ISISReflectometryWorkflowBase):
    '''
    Test the ISIS Reflectometry workflow algorithms with event slicing
    done internally in the workflow algorithm
    '''
    event_run_numbers = ['45222']
    first_transmission_run = ['45226']
    second_transmission_run = ['45227']
    transmission_workspace_name = ['TRANS']
    input_runs_file = None
    runs_workspace = 'Runs'
    reference_file = 'ISISReflectometryWorkflowSlicingResult.nxs'
    result_workspace = 'Result'

    def __init__(self):
        super(ISISReflectometryWorkflowSlicingTest, self).__init__()
        self.tolerance = 1e-6

    def runTest(self):
        self.setupTest()
        eventRef(self.run_numbers, 0.5, 60, self.first_transmission_run,
                 self.second_transmission_run)
        self.finaliseResults()


def eventRef(run_number, angle, time_interval = None , DB='TRANS'):
    ''' Perform reflectometry reduction on the run, slicing at the given duration '''
    run_name=str(run_number)
    # Reduce this run
    ReflectometryISISLoadAndProcess(InputRunList=run_name,
                                    FirstTransmissionRunList=first_transmission_run,
                                    FirstTransmissionRunList=second_transmission_run,
                                    SliceWorkspace=True, TimeInterval=time_interval,
                                    OutputWorkspaceBinned=run_name + '_ref_binned',
                                    OutputWorkspace=run_name + '_ref',
                                    OutputWorkspaceWavelength=run_name + '_lam',
                                    Debug=True)
    # Delete interim workspaces
    DeleteWorkspace(run_name + '_ref')
    DeleteWorkspace(run_name + '_lam')
    DeleteWorkspace(run_name + '_sliced')


def testEventDataTimeSlicing(event_run_numbers):
    ''' Perform reduction of each run, slicing the run into 60 second time slices'''
    for run_number in event_run_numbers:
        eventRef(run_number, 0.5, 60, DB='TRANS')
