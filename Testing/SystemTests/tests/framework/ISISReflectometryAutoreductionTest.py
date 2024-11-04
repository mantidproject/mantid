# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""
System Test for ISIS Reflectometry autoreduction
Adapted from scripts provided by Max Skoda.
"""

from ISISReflectometryWorkflowBase import (
    ISISReflectometryWorkflowBase,
    setupInstrument,
    stitchedTransmissionWorkspaceName,
    stitchTransmissionWorkspaces,
    transmissionWorkspaceName,
)
import re
import itertools
import math
import systemtesting
from operator import itemgetter
from mantid.api import mtd, AnalysisDataService
from mantid.kernel import logger
from mantid.simpleapi import CreateTransmissionWorkspaceAuto, NRCalculateSlitResolution, SaveNexus, ReflectometryISISLoadAndProcess
from isis_reflectometry.combineMulti import combineDataMulti, getWorkspace


class ISISReflectometryAutoreductionTest(systemtesting.MantidSystemTest, ISISReflectometryWorkflowBase):
    # NOTE: When updating the run range used be sure to update the run_titles table below.
    investigation_id = 1710262
    run_numbers = range(44319, 44349)
    first_transmission_runs = ["44297"]
    second_transmission_runs = ["44296"]
    input_workspaces_file = "ISISReflectometryAutoreductionTestRuns.nxs"
    reference_file = "ISISReflectometryAutoreductionResult.nxs"

    def __init__(self):
        super(ISISReflectometryAutoreductionTest, self).__init__()
        self.tolerance = 1e-6

    def requiredFiles(self):
        return [self.reference_file, self.input_workspaces_file]

    def validate(self):
        return (self.result_workspace_name, self.reference_file)

    def runTest(self):
        self.setupTest()
        self.workspaces_to_exclude_from_result = AnalysisDataService.Instance().getObjectNames()

        stitched_name = stitchedTransmissionWorkspaceName(self.first_transmission_runs[0], self.second_transmission_runs[0])
        stitchTransmissionWorkspaces(self.first_transmission_runs, self.second_transmission_runs, [stitched_name], False)

        AutoReduce([stitched_name, stitched_name], self.run_numbers)

        self.finaliseResults()

    @staticmethod
    def regenerateReferenceFileByReducing():
        setupInstrument()
        test = ISISReflectometryAutoreductionTest()
        test.runTest()
        SaveNexus(InputWorkspace=self.reference_workspace_name, Filename=self.reference_file)


run_titles = [
    "44296~ Si transmission 0.943 0.466 40 30 _",
    "44297~ Si transmission 0.943 0.466 20 15 _",
    "44298~ S1 D2O th=0.7",
    "44299~ S1 D2O th=2.3",
    "44300~ S2 D2O th=0.7",
    "44301~ S2 D2O th=2.3",
    "44302~ S3 D2O th=0.7",
    "44303~ S3 D2O th=2.3",
    "44304~ S1 CMSi th=0.7",
    "44305~ S1 CMSi th=2.3",
    "44306~ S2 CMSi th=0.7",
    "44307~ S2 CMSi th=2.3",
    "44308~ S3 CMSi th=0.7",
    "44309~ S3 CMSi th=2.3",
    "44310~ S1 H2O th=0.7",
    "44311~ S1 H2O th=2.3",
    "44312~ S2 H2O th=0.7",
    "44313~ S2 H2O th=2.3",
    "44314~ S3 H2O th=0.7",
    "44315~ S3 H2O th=2.3",
    "44316~ Si transmission 0.943 0.466 40 30 _",
    "44317~ Si transmission 0.943 0.466 20 15 _",
    "44318~ S1 D2O refill th=0.7",
    "44319~ S1 D2O syringe fill th=0.7",
    "44320~ S4 D2O th=0.7",
    "44321~ S4 D2O th=2.3",
    "44322~ S4 CMSi th=0.7",
    "44323~ S4 CMSi th=2.3",
    "44324~ S2 D2O L3NP th=0.7",
    "44325~ S2 D2O L3NP th=2.3",
    "44326~ S3 D2O L3NP+AP th=0.7",
    "44327~ S2 D2O L3NP2nd th=0.7",
    "44328~ S2 D2O L3NP2nd th=2.3",
    "44329~ S3 D2O L3NP+AP2nd th=0.7",
    "44330~ S3 D2O L3NP+AP2nd th=2.3",
    "44331~ S4 H2O th=0.7",
    "44332~ S4 H2O th=2.3",
    "44333~ S2 D2O L3NP 3rd th=0.7",
    "44334~ S2 D2O L3NP 3rd th=2.3",
    "44335~ S3 D2O L3NP+AP 3rd th=0.7",
    "44336~ S3 D2O L3NP+AP 3rd th=2.3",
    "44337~ S2 D2O L3NP rinse th=0.7",
    "44338~ S2 D2O L3NP rinse th=2.3",
    "44339~ S3 D2O L3NP+AP rinse th=0.7",
    "44340~ S3 D2O L3NP+AP rinse th=2.3",
    "44341~ S2 CMSi L3NP rinse th=0.7",
    "44342~ S2 CMSi L3NP rinse th=2.3",
    "44343~ S3 CMSi L3NP+AP rinse th=0.7",
    "44344~ S3 CMSi L3NP+AP rinse th=2.3",
    "44345~ S1 D2O 3rd th=0.7",
    "44346~ S1 D2O 3rd th=2.3",
    "44347~ S2 H2O L3NP rinse th=0.7",
    "44348~ S2 H2O L3NP rinse th=2.3",
    "44349~ S3 H2O L3NP+AP rinse th=0.7",
    "44350~ S3 H2O L3NP+AP rinse th=2.3",
    "44351~ S2 D2O L3NP + ADD AP th=0.7",
    "44352~ S2 D2O L3NP + ADD AP th=2.3",
    "44353~ S4 D2O Si + ADD AP th=0.7",
    "44354~ S4 D2O Si + ADD AP th=2.3",
    "44355~ S1 D2O L3NP + BL th=0.7",
    "44356~ S1 D2O L3NP + BL th=2.3",
    "44357~ S2 D2O L3NP + ADD AP 2nd th=0.7",
    "44358~ S2 D2O L3NP + ADD AP 2nd th=2.3",
    "44359~ S4 D2O Si + ADD AP 2nd th=0.7",
    "44360~ S4 D2O Si + ADD AP 2nd th=2.3",
    "44361~ S1 D2O L3NP + BL 2nd th=0.7",
    "44362~ S1 D2O L3NP + BL 2nd th=2.3",
    "44363~ S2 D2O L3NP + ADD AP rinse th=0.7",
    "44364~ S2 D2O L3NP + ADD AP rinse th=2.3",
    "44365~ S4 D2O Si + ADD AP rinse th=0.7",
    "44366~ S4 D2O Si + ADD AP rinse th=2.3",
    "44367~ S2 CMSi L3NP + ADD AP th=0.7",
    "44368~ S2 CMSi L3NP + ADD AP th=2.3",
    "44369~ S4 CMSi Si + ADD AP th=0.7",
    "44370~ S4 CMSi Si + ADD AP th=2.3",
    "44371~ S2 H2O L3NP + ADD AP th=0.7",
    "44372~ S2 H2O L3NP + ADD AP th=2.3",
    "44373~ S4 H2O Si + ADD AP th=0.7",
    "44374~ S4 H2O Si + ADD AP th=2.3",
    "44375~ S2 D2O L3NP + ADD AP 2nd rinse th=0.7",
    "44376~ S2 D2O L3NP + ADD AP 2nd rinse th=2.3",
    "44377~ S4 D2O Si + ADD AP 2ndrinse th=0.7",
    "44378~ S4 D2O Si + ADD AP 2ndrinse th=2.3",
]


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
            runnos = runno.split("+")
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
                if not mtd.doesExist(runno + "_IvsQ"):
                    th = angle
                    if len(transRun) > 1 and angle > 2.25:
                        wq, wq_unbinned, wlam, wtrans = ReflectometryISISLoadAndProcess(
                            InputRunList=ws,
                            FirstTransmissionRunList=transRun[1],
                            thetaIn=angle,
                            StartOverlap=10,
                            EndOverlap=12,
                            OutputWorkspace=runno + "_IvsQ",
                            OutputWorkspaceBinned=runno + "_IvsQ_binned",
                        )
                    else:
                        wq, wq_unbinned, wlam, wtrans = ReflectometryISISLoadAndProcess(
                            InputRunList=ws,
                            FirstTransmissionRunList=transRun[0],
                            thetaIn=angle,
                            StartOverlap=10,
                            EndOverlap=12,
                            OutputWorkspace=runno + "_IvsQ",
                            OutputWorkspaceBinned=runno + "_IvsQ_binned",
                        )
                    mtd.remove("wlam")
                    mtd.remove("wtrans")
                else:
                    wq = mtd[runno + "_IvsQ"]
                    th = angle
                wq_list.append(runno + "_IvsQ")
                inst = wq.getInstrument()
                lmin = inst.getNumberParameter("LambdaMin")[0] + 1
                lmax = inst.getNumberParameter("LambdaMax")[0] - 2
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
                outputwksp = wq_list[0].split("_")[0] + "_" + wq_list[-1].split("_")[0][3:]
            else:
                outputwksp = wq_list[0].split("_")[0] + "_IvsQ_binned"

            if not mtd.doesExist(outputwksp):
                combineDataMulti(wq_list, outputwksp, overlapLow, overlapHigh, Qmin, Qmax, -dqq, 0, keep=True)

    return sortedList


def MakeTuples(rlist):
    # sort runs into tuples : run number, title, theta
    tup = ()
    for idx in rlist:
        split_title = re.split("th=|~", idx)
        if len(split_title) != 3:
            split_title = re.split("~", idx)
            if len(split_title) != 2:
                logger.warning("cannot transfer " + idx + " title is not in the right form ")
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
    for _key, group in itertools.groupby(tupsort, lambda x: x[1]):  # now group by title
        col = 0
        # for storing run_angle pairs all with the same title
        run_angle_pairs_of_title = list()
        for object in group:  # loop over all with equal title
            one_sample = []
            run_no = object[0]
            angle = object[-1]
            run_angle_pairs_of_title.append((run_no, angle))
            # print run_angle_pairs_of_title
        for angle_key, group in itertools.groupby(run_angle_pairs_of_title, lambda x: x[1]):
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


def CreateTransmissionWorkspaces(run1, run2, scale=False):
    CreateTransmissionWorkspaceAuto(run1, OutputWorkspace=transmissionWorkspaceName(run1), StartOverlap=10, EndOverlap=12)
    CreateTransmissionWorkspaceAuto(run2, OutputWorkspace=transmissionWorkspaceName(run2), StartOverlap=10, EndOverlap=12)
