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
import re
import systemtesting
from mantid.simpleapi import *
from mantid import ConfigService


class ISISReflAutoreductionLoadTest(systemtesting.MantidSystemTest):
    # NOTE: When updating the run range used be sure to update the run_titles table below.
    # You may also find the regenerate functions useful.
    investigation_id = 1710262
    run_numbers = '44319'
    transmission_run_names = ['44297', '44296']
    run_file = '44319.nxs'
    trans_runs_file = 'ISISReflectometryAutoreductionTestTransRuns.nxs'
    runs_workspace = 'Runs'
    reference_result_file = 'ISISReflectometryAutoreductionWithLoadResult.nxs'
    result_workspace = 'Result'

    def __init__(self):
        super(ISISReflAutoreductionLoadTest, self).__init__()
        self.tolerance = 0.00000001

    def requiredFiles(self):
        return [self.reference_result_file, self.trans_runs_file, self.run_file]

    def validate(self):
        return (self.result_workspace, self.reference_result_file)

    def runTest(self):
        ConfigService.Instance().setString("default.instrument", "INTER")
        Load(self.trans_runs_file, OutputWorkspace=self.runs_workspace)
        CreateTransmissionWorkspaces(self.transmission_run_names[0],
                                     self.transmission_run_names[1],
                                     scale=False)
        workspaces_to_exclude_from_result = AnalysisDataService.Instance().getObjectNames()
        stitched_name = StitchedTransmissionWorkspaceName(self.transmission_run_names[0], self.transmission_run_names[1])
        Stitch1D(
            LHSWorkspace=TransmissionWorkspaceName(self.transmission_run_names[0]),
            RHSWorkspace=TransmissionWorkspaceName(self.transmission_run_names[1]),
            StartOverlap=10,
            EndOverlap=12,
            ScaleRHSWorkspace=False,
            OutputWorkspace=stitched_name)
        AutoReduce(self.run_file, [stitched_name, stitched_name], self.run_numbers)
        RemoveWorkspaces(workspaces_to_exclude_from_result)
        GroupWorkspaces(InputWorkspaces=AnalysisDataService.Instance().getObjectNames(),
                        OutputWorkspace=self.result_workspace)
        mtd[self.result_workspace].sortByName()

    @staticmethod
    def regenerateRunsFile():
        RegenerateRunsFile(ISISReflectometryAutoreductionTest.transmission_run_names,
                           ISISReflectometryAutoreductionTest.run_numbers)

    @staticmethod
    def run():
        test = ISISReflectometryAutoreductionTest()
        test.runTest()

    @staticmethod
    def regenerateReferenceFileByReducing():
        test = ISISReflectometryAutoreductionTest()
        test.runTest()
        SaveNexus(InputWorkspace=ISISReflectometryAutoreductionTest.result_workspace,
                  Filename=ISISReflectometryAutoreductionTest.reference_result_file)

    @staticmethod
    def regenerateReferenceFileFromDirectory(reference_file_directory):
        RegenerateReferenceFile(reference_file_directory, ISISReflectometryAutoreductionTest.reference_result_file)

    @staticmethod
    def regenerateRunTitles():
        RegenerateRunTitles(ISISReflectometryAutoreductionTest.investigation_id)


def RemoveWorkspaces(to_remove):
    for workspace_name in to_remove:
        AnalysisDataService.Instance().remove(workspace_name)


def WorkspaceName(file_path):
    return os.path.splitext(os.path.basename(file_path))[0]


def RegenerateReferenceFile(reference_file_directory, output_filename):
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


def RegenerateRunsFile(transmission_run_names, run_range):
    """This is used to generate the test input file from a range of run numbers
    and transmission runs."""
    from mantid.simpleapi import (Load, GroupWorkspaces)

    for run in transmission_run_names:
        Load('{}.raw'.format(run), OutputWorkspace=run)

    run_names = [str(run_number) for run_number in run_range]
    file_names = ["{}.raw".format(run_name) for run_name in run_names]

    for run_name, file_name in zip(run_names, file_names):
        Load(file_name, OutputWorkspace=run_name)

    GroupWorkspaces(InputWorkspaces=run_names + transmission_run_names, OutputWorkspace='Input')
    SaveNexus(InputWorkspace='Input', Filename='ISISReflectometryAutoreductionTestRuns.nxs')


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


def AutoReduce(runfile, transRun=[], runRange=[], ):
    runno = str(runRange)
    ReflectometryISISLoadAndProcess(
        InputRunList=runfile,
        FirstTransmissionRunList=transRun[1],
        thetaIn=0.7,
        StartOverlap=10,
        EndOverlap=12,
        OutputWorkspace=runno + '_IvsQ',
        OutputWorkspaceBinned=runno + '_IvsQ_binned')

    AnalysisDataService.Instance().remove('TOF_' + runno)


def TransmissionWorkspaceName(run):
    return "TRANS_{}".format(run)


def StitchedTransmissionWorkspaceName(run_number_1, run_number_2):
    return 'TRANS_{}_{}'.format(run_number_1, run_number_2)


def CreateTransmissionWorkspaces(run1, run2, scale=False):
    CreateTransmissionWorkspaceAuto(
        run1,
        OutputWorkspace=TransmissionWorkspaceName(run1),
        StartOverlap=10,
        EndOverlap=12)
    CreateTransmissionWorkspaceAuto(
        run2,
        OutputWorkspace=TransmissionWorkspaceName(run2),
        StartOverlap=10,
        EndOverlap=12)

# If you want to re-run the test and save the result as a reference...
#   ISISReflectometryAutoreductionTest.regenerateReferenceFileByReducing()

# or
# If you have workspaces in a folder to use as a reference...
#   ISISReflectometryAutoreductionTest.regenerateReferenceFileFromDirectory("Path/To/Folder")
