# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
"""
System Test for ISIS Reflectometry reduction
Adapted from scripts provided by Max Skoda.
"""
from __future__ import (print_function)
import systemtesting
from mantid.simpleapi import *
from mantid import ConfigService


class INTERReductionWithSlicingTest(systemtesting.MantidSystemTest):
    ''' TODO::Update comments
    Mantid Test Script for INTER:

    Tests:
    1. Event data time-slicing
    2. "Quickref" - used when the autoreduction unwantedly sums runs together.
    3. Scripted fitting of reduced data for NRW
    4. Linear detector reduction
    '''
    # Note: you may find the regenerate functions useful.

    run_numbers = ['45222']
    first_transmission_run_name = ['45226']
    second_transmission_run_name = ['45227']
    transmission_workspace_name = ['TRANS']
    runs_file = 'INTERReductionSlicingTestRuns.nxs'
    runs_workspace = 'Runs'
    reference_result_file = 'INTERReductionSlicingResult.nxs'
    result_workspace = 'Result'
    expected_fit_params={
        'Name': ['Theta', 'ScaleFactor', 'AirSLD', 'BulkSLD', 'Roughness', 'BackGround', 'Resolution',
                 'SLD_Layer0', 'd_Layer0', 'Rough_Layer0', 'Cost function value'],
        'Value': [2.2999999999999998, 1, 0, 0, 0, 7.9488100000033575e-06, 5, 8.2865399998548345e-07,
                  31.315399998680391, 0, 0.90910656437440196],
        'Error': [0, 0, 0, 0, 0, 1.1949723340559929e-07, 0, 2.2338118077681473e-08, 0.9921668942754267, 0, 0]}
    expected_fit_covariance={
        'Name': ['BackGround', 'SLD_Layer0', 'd_Layer0'],
        'BackGround':[100, -64.519943050096259, 60.825620358818924],
        'SLD_Layer0':[-64.519943050096032, 100, -96.330851077988683],
        'd_Layer0': [60.825555558936458, -96.33084065170361, 100]}

    def __init__(self):
        super(INTERReductionWithSlicingTest, self).__init__()
        self.tolerance = 1e-6

    def requiredFiles(self):
        return [self.reference_result_file, self.runs_file]

    def validate(self):
        return (self.result_workspace, self.reference_result_file)

    def runTest(self):
        setupInstrument()
        Load(self.runs_file, OutputWorkspace=self.runs_workspace)
        workspaces_to_exclude_from_result = AnalysisDataService.Instance().getObjectNames()
        createTransmissionWorkspaces(self.first_transmission_run_name,
                                     self.second_transmission_run_name,
                                     self.transmission_workspace_name)

        testEventDataTimeSlicing(self.run_numbers)
        removeWorkspaces(workspaces_to_exclude_from_result)
        GroupWorkspaces(InputWorkspaces=AnalysisDataService.Instance().getObjectNames(),
                        OutputWorkspace=self.result_workspace)
        mtd[self.result_workspace].sortByName()

    @staticmethod
    def regenerateRunsFile():
        setupInstrument()
        regenerateRunsFile(INTERReductionWithSlicingTest.first_transmission_run_name +
                           INTERReductionWithSlicingTest.second_transmission_run_name,
                           INTERReductionWithSlicingTest.run_numbers)

    @staticmethod
    def regenerateReferenceFileByReducing():
        setupInstrument()
        test = INTERReductionWithSlicingTest()
        test.runTest()
        SaveNexus(InputWorkspace=INTERReductionWithSlicingTest.result_workspace,
                  Filename=INTERReductionWithSlicingTest.reference_result_file)

    @staticmethod
    def regenerateReferenceFileFromDirectory(reference_file_directory):
        setupInstrument()
        regenerateReferenceFile(reference_file_directory, INTERReductionWithSlicingTest.reference_result_file)


def setupInstrument():
    configI = ConfigService.Instance()
    configI.setString("default.instrument", "INTER")
    configI.setString("default.facility", "ISIS")


def removeWorkspaces(to_remove):
    for workspace_name in to_remove:
        AnalysisDataService.Instance().remove(workspace_name)


def workspaceName(file_path):
    return os.path.splitext(os.path.basename(file_path))[0]


def regenerateReferenceFile(reference_file_directory, output_filename):
    '''Generate the reference file from a given folder of output workspaces'''
    files = os.listdir(reference_file_directory)
    workspace_names = []
    for file in files:
        workspace_name = WorkspaceName(file)
        Load(file, OutputWorkspace=workspace_name)
        workspace_names.append(workspace_name)

    output_workspace_name = 'Output'
    GroupWorkspaces(InputWorkspaces=workspace_names, OutputWorkspace=output_workspace_name)
    mtd[output_workspace_name].sortByName()
    SaveNexus(InputWorkspace=output_workspace_name, Filename=output_filename)


def regenerateRunsFile(transmission_run_names, run_numbers, event_run_numbers):
    '''Generate the test input file from a range of run numbers and transmission runs.'''
    # Load transmission runs
    for run in transmission_run_names:
        Load('{}.raw'.format(run), OutputWorkspace=run)
    # Load raw run files
    run_names = [str(run_number)+'.raw' for run_number in run_numbers]
    for run_name in run_names:
        Load(run_name, OutputWorkspace=run_name)
    # Load event workspaces
    event_run_names = [str(event_run_number) for event_run_number in event_run_numbers]
    for event_run_name in event_run_names:
        LoadEventNexus(event_run_name, OutputWorkspace=event_run_name, LoadMonitors=True)
    event_monitor_names = [str(run_number)+'_monitors' for run_number in event_run_numbers]
    # Group and save
    GroupWorkspaces(InputWorkspaces=run_names + transmission_run_names + event_run_names +
                    event_monitor_names,
                    OutputWorkspace='Input')
    SaveNexus(InputWorkspace='Input', Filename='INTERReductionTestRuns.nxs')


def createTransmissionWorkspaces(runs1, runs2, output_names):
    '''Create a transmission workspace for each pair of input runs with the given output names'''
    for run1, run2, name in zip(runs1, runs2, output_names):
        CreateTransmissionWorkspaceAuto(
            FirstTransmissionRun=run1,
            SecondTransmissionRun=run2,
            OutputWorkspace=name,
            StartOverlap=10,
            EndOverlap=12)
        # Delete debug workspaces
        DeleteWorkspace('TRANS_LAM_'+run1)
        DeleteWorkspace('TRANS_LAM_'+run2)


def eventRef(run_number, angle, time_interval = None , DB='TRANS'):
    ''' Perform reflectometry reduction on the run, slicing at the given duration '''
    run_name=str(run_number)
    # Reduce this run
    ReflectometryISISLoadAndProcess(InputRunList=run_name, FirstTransmissionRunList=DB,
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


# If you want to re-run the test and save the result as a reference...
#   INTERReductionWithSlicingTest.regenerateReferenceFileByReducing()

# or
# If you have workspaces in a folder to use as a reference...
#   INTERReductionWithSlicingTest.regenerateReferenceFileFromDirectory("Path/To/Folder")
