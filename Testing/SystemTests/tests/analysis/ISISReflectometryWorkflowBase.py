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


class ISISReflectometryWorkflowBase(systemtesting.MantidSystemTest):
    ''' 
    Base class for testing the ISIS Reflectometry workflow algorithms
    
    You may find the regenerate functions useful:
    - If you want to re-run the test and save the result as a reference...
        INTERReductionWithSlicingTest.regenerateReferenceFileByReducing()
    - If you have workspaces in a folder to use as a reference...
        INTERReductionWithSlicingTest.regenerateReferenceFileFromDirectory("Path/To/Folder")
    '''

    # Default investigation used for generating run titles
    investigation_id = 1710262

    # Derived class should set these run numbers up as required
    event_run_numbers = None
    run_numbers = None
    first_transmission_run = None
    second_transmission_run = None
    transmission_workspace_name = None
    input_runs_file = None
    reference_file = None

    # Default names for input and output workspace groups
    input_runs_workspace_name = 'Runs'
    reference_workspace_name = 'Result'

    def __init__(self):
        super(ISISReflectometryWorkflowBase, self).__init__()
        self.tolerance = 1e-6

    def requiredFiles(self):
        return [self.reference_file, self.input_runs_file]

    def validate(self):
        return (self.reference_workspace_name, self.reference_file)

    def setupTest(self):
        '''Set up the instrument and any required workspaces ready for
        the start of the test'''
        setupInstrument()

        if self.input_runs_file is not None:
            Load(self.input_runs_file, OutputWorkspace=self.input_runs_workspace_name)

        self.workspaces_to_exclude_from_result = AnalysisDataService.Instance().getObjectNames()

    def finaliseResults(self):
        '''Clear interim workspaces and group required outputs into the final
        result ready for comparison with the reference'''
        removeWorkspaces(self.workspaces_to_exclude_from_result)
        GroupWorkspaces(InputWorkspaces=AnalysisDataService.Instance().getObjectNames(),
                        OutputWorkspace=self.reference_workspace_name)
        mtd[self.reference_workspace_name].sortByName()

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
        SaveNexus(InputWorkspace=INTERReductionWithSlicingTest.reference_workspace_name,
                  Filename=INTERReductionWithSlicingTest.reference_file)

    @staticmethod
    def regenerateReferenceFileFromDirectory(reference_file_directory):
        setupInstrument()
        regenerateReferenceFile(reference_file_directory, INTERReductionWithSlicingTest.reference_file)

    @staticmethod
    def regenerateRunTitles():
        RegenerateRunTitles(ISISReflectometryAutoreductionTest.investigation_id)


def setupInstrument():
    configI = ConfigService.Instance()
    configI.setString("default.instrument", "INTER")
    configI.setString("default.facility", "ISIS")


def removeWorkspaces(to_remove):
    for workspace_name in to_remove:
        AnalysisDataService.Instance().remove(workspace_name)


def workspaceName(file_path):
    return os.path.splitext(os.path.basename(file_path))[0]


def transmissionWorkspaceName(run):
    return "TRANS_{}".format(run)


def stitchedTransmissionWorkspaceName(run_number_1, run_number_2):
    return 'TRANS_{}_{}'.format(run_number_1, run_number_2)


def stitchTransmissionWorkspaces(runs1, runs2, output_names):
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


def reduceRun(run_number, angle, first_transmission_runs = [], second_transmission_runs = [],
              time_interval = None):
    ''' Perform reflectometry reduction on the run'''
    run_name=str(run_number)
    # Reduce this run
    ReflectometryISISLoadAndProcess(InputRunList=run_name,
                                    FirstTransmissionRunList=first_transmission_runs.join(),
                                    SecondTransmissionRunList=second_transmission_runs.join(),
                                    SliceWorkspace=True, TimeInterval=time_interval,
                                    OutputWorkspaceBinned=run_name + '_ref_binned',
                                    OutputWorkspace=run_name + '_ref',
                                    OutputWorkspaceWavelength=run_name + '_lam',
                                    Debug=True)
    # Delete interim workspaces
    DeleteWorkspace(run_name + '_ref')
    DeleteWorkspace(run_name + '_lam')
    DeleteWorkspace(run_name + '_sliced')


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

    
def RegenerateRunTitles(investigation_id):
    """Uses the old reflectometry gui python modules to generate the runs table from ICAT.
    A local copy of the table generated is stored in run_titles below.
    You may be able to use this script to update it."""
    # self.listMain.clear()

    # Use ICAT for a journal search based on the RB number

    active_session_id = None
    if CatalogManager.numberActiveSessions() == 0:
        # Execute the CatalogLoginDialog
        login_alg = CatalogLoginDialog()
        session_object = login_alg.getProperty("KeepAlive").value
        active_session_id = session_object.getPropertyValue("Session")

    # Fetch out an existing session id
    # This might be another catalog session, but at present there is
    # no way to tell.
    active_session_id = CatalogManager.getActiveSessions()[-1].getSessionId()

    search_alg = AlgorithmManager.create('CatalogGetDataFiles')
    search_alg.initialize()
    search_alg.setChild(True)  # Keeps the results table out of the ADS
    search_alg.setProperty('InvestigationId', str(investigation_id))
    search_alg.setProperty('Session', active_session_id)
    search_alg.setPropertyValue('OutputWorkspace', '_dummy')
    search_alg.execute()
    search_results = search_alg.getProperty('OutputWorkspace').value

    # self.__icat_file_map = {}
    # self.statusMain.clearMessage()
    runlist = []
    for row in search_results:
        file_name = row['Name']
        description = row['Description']
        run_number = re.search(r'[1-9]\d+', file_name).group()

        # Filter to only display and map raw files.
        if bool(re.search('(raw)$', file_name, re.IGNORECASE)):
            title = (run_number + '~ ' + description).strip()
            runlist.append(title)
    # self.SampleText.__icat_file_map[title] = #(file_id, run_number, file_name)
    # self.listMain.addItem(title)
    # self.listMain.sortItems()
    return runlist
    # del search_results
