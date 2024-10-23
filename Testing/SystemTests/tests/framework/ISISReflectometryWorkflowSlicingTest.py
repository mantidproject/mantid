# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from ISISReflectometryWorkflowBase import reduceRun, setupInstrument, ISISReflectometryWorkflowBase
from mantid.simpleapi import DeleteWorkspace, SaveNexus
import systemtesting


class ISISReflectometryWorkflowSlicingTest(systemtesting.MantidSystemTest, ISISReflectometryWorkflowBase):
    """
    Test the ISIS Reflectometry workflow algorithms with event slicing
    done internally in the workflow algorithm
    """

    run_numbers = ["45222"]
    first_transmission_runs = ["45226"]
    second_transmission_runs = ["45227"]
    transmission_workspace_name = ["TRANS"]
    input_workspaces_file = "ISISReflectometryEventTestRuns.nxs"
    reference_file = "ISISReflectometryWorkflowSlicingResult.nxs"

    def __init__(self):
        super(ISISReflectometryWorkflowSlicingTest, self).__init__()
        self.tolerance = 1e-6

    def requiredFiles(self):
        return [self.reference_file, self.input_workspaces_file]

    def validate(self):
        return (self.result_workspace_name, self.reference_file)

    def runTest(self):
        self.setupTest()
        reduceRun(self.run_numbers[0], 0.5, self.first_transmission_runs, self.second_transmission_runs, time_interval=60)
        # Delete the interim transmission workspaces. These are currently output
        # for input groups (i.e. when we're slicing) even when Debug is not on.
        DeleteWorkspace("TRANS_LAM_45226")
        DeleteWorkspace("TRANS_LAM_45227")
        self.finaliseResults()

    @staticmethod
    def regenerateReferenceFileByReducing():
        setupInstrument()
        test = ISISReflectometryWorkflowSlicingTest()
        test.runTest()
        SaveNexus(InputWorkspace=self.result_workspace_name, Filename=self.reference_file)
