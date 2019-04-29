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
import itertools
import math
import systemtesting
from operator import itemgetter
from mantid.simpleapi import *
from mantid import ConfigService
from isis_reflectometry.combineMulti import combineDataMulti, getWorkspace


class ISISReflectometryAutoreductionTest(systemtesting.MantidSystemTest):
    # NOTE: When updating the run range used be sure to update the run_titles table below.
    # You may also find the regenerate functions useful.
    investigation_id = 1710262
    run_numbers = range(44319, 44349)
    transmission_run_names = ['44297', '44296']
    runs_file = 'ISISReflectometryAutoreductionTestRuns.nxs'
    runs_workspace = 'Runs'
    reference_result_file = 'ISISReflectometryAutoreductionResult.nxs'
    result_workspace = 'Result'

    def __init__(self):
        super(ISISReflectometryAutoreductionTest, self).__init__()
        self.tolerance = 0.00000001

    def requiredFiles(self):
        return [self.reference_result_file, self.runs_file]

    def validate(self):
        return (self.result_workspace, self.reference_result_file)

    def runTest(self):
        ConfigService.Instance().setString("default.instrument", "INTER")
        Load(self.runs_file, OutputWorkspace=self.runs_workspace)
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
        AutoReduce([stitched_name, stitched_name],
                   self.run_numbers)
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
    "This is used to generate the test input file from a range of run numbers"
    "and transmission runs."
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
    "Uses the old reflectometry gui python modules to generate the runs table from ICAT."
    "A local copy of the table generated is stored in run_titles below."
    "You may be able to use this script to update it."
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


run_titles = [
    '44296~ Si transmission 0.943 0.466 40 30 _',
    '44297~ Si transmission 0.943 0.466 20 15 _',
    '44298~ S1 D2O th=0.7',
    '44299~ S1 D2O th=2.3',
    '44300~ S2 D2O th=0.7',
    '44301~ S2 D2O th=2.3',
    '44302~ S3 D2O th=0.7',
    '44303~ S3 D2O th=2.3',
    '44304~ S1 CMSi th=0.7',
    '44305~ S1 CMSi th=2.3',
    '44306~ S2 CMSi th=0.7',
    '44307~ S2 CMSi th=2.3',
    '44308~ S3 CMSi th=0.7',
    '44309~ S3 CMSi th=2.3',
    '44310~ S1 H2O th=0.7',
    '44311~ S1 H2O th=2.3',
    '44312~ S2 H2O th=0.7',
    '44313~ S2 H2O th=2.3',
    '44314~ S3 H2O th=0.7',
    '44315~ S3 H2O th=2.3',
    '44316~ Si transmission 0.943 0.466 40 30 _',
    '44317~ Si transmission 0.943 0.466 20 15 _',
    '44318~ S1 D2O refill th=0.7',
    '44319~ S1 D2O syringe fill th=0.7',
    '44320~ S4 D2O th=0.7',
    '44321~ S4 D2O th=2.3',
    '44322~ S4 CMSi th=0.7',
    '44323~ S4 CMSi th=2.3',
    '44324~ S2 D2O L3NP th=0.7',
    '44325~ S2 D2O L3NP th=2.3',
    '44326~ S3 D2O L3NP+AP th=0.7',
    '44327~ S2 D2O L3NP2nd th=0.7',
    '44328~ S2 D2O L3NP2nd th=2.3',
    '44329~ S3 D2O L3NP+AP2nd th=0.7',
    '44330~ S3 D2O L3NP+AP2nd th=2.3',
    '44331~ S4 H2O th=0.7',
    '44332~ S4 H2O th=2.3',
    '44333~ S2 D2O L3NP 3rd th=0.7',
    '44334~ S2 D2O L3NP 3rd th=2.3',
    '44335~ S3 D2O L3NP+AP 3rd th=0.7',
    '44336~ S3 D2O L3NP+AP 3rd th=2.3',
    '44337~ S2 D2O L3NP rinse th=0.7',
    '44338~ S2 D2O L3NP rinse th=2.3',
    '44339~ S3 D2O L3NP+AP rinse th=0.7',
    '44340~ S3 D2O L3NP+AP rinse th=2.3',
    '44341~ S2 CMSi L3NP rinse th=0.7',
    '44342~ S2 CMSi L3NP rinse th=2.3',
    '44343~ S3 CMSi L3NP+AP rinse th=0.7',
    '44344~ S3 CMSi L3NP+AP rinse th=2.3',
    '44345~ S1 D2O 3rd th=0.7',
    '44346~ S1 D2O 3rd th=2.3',
    '44347~ S2 H2O L3NP rinse th=0.7',
    '44348~ S2 H2O L3NP rinse th=2.3',
    '44349~ S3 H2O L3NP+AP rinse th=0.7',
    '44350~ S3 H2O L3NP+AP rinse th=2.3',
    '44351~ S2 D2O L3NP + ADD AP th=0.7',
    '44352~ S2 D2O L3NP + ADD AP th=2.3',
    '44353~ S4 D2O Si + ADD AP th=0.7',
    '44354~ S4 D2O Si + ADD AP th=2.3',
    '44355~ S1 D2O L3NP + BL th=0.7',
    '44356~ S1 D2O L3NP + BL th=2.3',
    '44357~ S2 D2O L3NP + ADD AP 2nd th=0.7',
    '44358~ S2 D2O L3NP + ADD AP 2nd th=2.3',
    '44359~ S4 D2O Si + ADD AP 2nd th=0.7',
    '44360~ S4 D2O Si + ADD AP 2nd th=2.3',
    '44361~ S1 D2O L3NP + BL 2nd th=0.7',
    '44362~ S1 D2O L3NP + BL 2nd th=2.3',
    '44363~ S2 D2O L3NP + ADD AP rinse th=0.7',
    '44364~ S2 D2O L3NP + ADD AP rinse th=2.3',
    '44365~ S4 D2O Si + ADD AP rinse th=0.7',
    '44366~ S4 D2O Si + ADD AP rinse th=2.3',
    '44367~ S2 CMSi L3NP + ADD AP th=0.7',
    '44368~ S2 CMSi L3NP + ADD AP th=2.3',
    '44369~ S4 CMSi Si + ADD AP th=0.7',
    '44370~ S4 CMSi Si + ADD AP th=2.3',
    '44371~ S2 H2O L3NP + ADD AP th=0.7',
    '44372~ S2 H2O L3NP + ADD AP th=2.3',
    '44373~ S4 H2O Si + ADD AP th=0.7',
    '44374~ S4 H2O Si + ADD AP th=2.3',
    '44375~ S2 D2O L3NP + ADD AP 2nd rinse th=0.7',
    '44376~ S2 D2O L3NP + ADD AP 2nd rinse th=2.3',
    '44377~ S4 D2O Si + ADD AP 2ndrinse th=0.7',
    '44378~ S4 D2O Si + ADD AP 2ndrinse th=2.3']


def AutoReduce(transRun=[], runRange=[], oldList=[]):
    tupsort = MakeTuples(run_titles)
    sortedList = SortRuns(tupsort)

    newList = [item for item in sortedList if item not in oldList]

    for sample in newList:
        wq_list = []
        overlapLow = []
        overlapHigh = []
        for item in sample:
            runno = item[0]
            angle = item[1]
            runnos = runno.split('+')
            # check if runs have been added together
            runnos = [int(i) for i in runnos]

            try:
                angle = float(angle)
            except ValueError:
                angle = 0.0
                print("Could not determine theta! Skipping run.")

            if len(runRange) and not len(set(runRange) & set(runnos)):
                angle = 0.0  # will be skipped below

            if float(angle) > 0.0:
                ws = str(runno)
                # w1 = mtd[runno + '.raw']
                # spectra = w1.getRun().getLogData('nspectra').value
                if not mtd.doesExist(runno + '_IvsQ'):
                    th = angle
                    if len(transRun) > 1 and angle > 2.25:
                        wq, wq_binned = \
                            ReflectometryReductionOneAuto(
                                InputWorkspace=ws,
                                FirstTransmissionRun=transRun[1],
                                thetaIn=angle,
                                OutputWorkspace=runno + '_IvsQ',
                                OutputWorkspaceWavelength=runno + '_IvsLam',
                                OutputWorkspaceBinned=runno + '_IvsQ_binned')
                    else:
                        wq, wqbinned = \
                            ReflectometryReductionOneAuto(
                                InputWorkspace=ws,
                                FirstTransmissionRun=transRun[0],
                                thetaIn=angle,
                                OutputWorkspace=runno + '_IvsQ',
                                OutputWorkspaceWavelength=runno + '_IvsLam',
                                OutputWorkspaceBinned=runno + '_IvsQ_binned')
                else:
                    wq = mtd[runno + '_IvsQ']
                    th = angle
                wq_list.append(runno + '_IvsQ')
                inst = wq.getInstrument()
                lmin = inst.getNumberParameter('LambdaMin')[0] + 1
                lmax = inst.getNumberParameter('LambdaMax')[0] - 2
                qmin = 4 * math.pi / lmax * math.sin(th * math.pi / 180)
                qmax = 4 * math.pi / lmin * math.sin(th * math.pi / 180)
                overlapLow.append(qmin)
                overlapHigh.append(qmax)
                dqq = NRCalculateSlitResolution(Workspace=wq, TwoTheta=angle)

        if len(wq_list):
            w1 = getWorkspace(wq_list[0])
            w2 = getWorkspace(wq_list[-1])
            Qmin = min(w1.readX(0))
            Qmax = max(w2.readX(0))
            Qmax = 0.3

            # print(Qmin, Qmax, dqq)
            # print(overlapHigh)
            if len(wq_list) > 1:
                outputwksp = wq_list[0].split('_')[0] + '_' + wq_list[-1].split('_')[0][3:]
            else:
                outputwksp = wq_list[0].split('_')[0] + '_IvsQ_binned'

            if not mtd.doesExist(outputwksp):
                combineDataMulti(
                    wq_list,
                    outputwksp,
                    overlapLow,
                    overlapHigh,
                    Qmin,
                    Qmax,
                    -dqq,
                    0,
                    keep=True)

    return sortedList


def MakeTuples(rlist):
    # sort runs into tuples : run number, title, theta
    tup = ()
    for idx in rlist:
        split_title = re.split("th=|~", idx)
        if len(split_title) != 3:
            split_title = re.split("~", idx)
            if len(split_title) != 2:
                logger.warning(
                    'cannot transfer ' +
                    idx +
                    ' title is not in the right form ')
            else:
                theta = 0
                split_title.append(theta)  # Append a dummy theta value.
                tup = tup + (split_title,)
        else:
            # Tuple of lists containing(run number, title, theta)
            tup = tup + (split_title,)

    tupsort = sorted(tup, key=itemgetter(1, 2))
    return tupsort


def SortRuns(tupsort):
    # sort tuples of runs into groups belonging to one sample title
    row = 0
    complete_list = []
    for _key, group in itertools.groupby(
            tupsort, lambda x: x[1]):  # now group by title
        col = 0
        # for storing run_angle pairs all with the same title
        run_angle_pairs_of_title = list()
        for object in group:  # loop over all with equal title
            one_sample = []
            run_no = object[0]
            angle = object[-1]
            run_angle_pairs_of_title.append((run_no, angle))
            # print run_angle_pairs_of_title
        for angle_key, group in itertools.groupby(
                run_angle_pairs_of_title, lambda x: x[1]):
            runnumbers = "+".join(["%s" % pair[0] for pair in group])

            if col >= 11:
                col = 0
            else:
                one_sample.append((runnumbers, angle_key))
                print(one_sample)
            col = col + 5
        row = row + 1
        complete_list.append(one_sample)
    sortedList = sorted(complete_list, key=lambda runno: runno[0])
    return sortedList


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
