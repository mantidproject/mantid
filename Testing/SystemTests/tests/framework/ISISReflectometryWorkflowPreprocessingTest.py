# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""
System Test for ISIS Reflectometry autoreduction
Adapted from scripts provided by Max Skoda.
"""

from ISISReflectometryWorkflowBase import reduceRun, setupInstrument, ISISReflectometryWorkflowBase
from mantid.simpleapi import SaveNexus
import systemtesting


class ISISReflectometryWorkflowPreprocessingTest(systemtesting.MantidSystemTest, ISISReflectometryWorkflowBase):
    """
    Script to test that the ISIS Reflectometry workflow successfully performs
    required preprocessing of input runs and transmission runs before it
    performs the reduction, when those inputs are not already in the ADS.
    """

    run_numbers = ["45222"]
    first_transmission_runs = ["45226"]
    second_transmission_runs = ["45227"]
    input_workspaces_file = "ISISReflectometryTestRuns.nxs"
    reference_file = "ISISReflectometryReducedRunsResult.nxs"

    def __init__(self):
        super(ISISReflectometryWorkflowPreprocessingTest, self).__init__()
        self.tolerance = 1e-6

    def requiredFiles(self):
        return [self.reference_file, self.input_workspaces_file]

    def validate(self):
        return (self.result_workspace_name, self.reference_file)

    def runTest(self):
        self.setupTest()
        reduceRun(
            run_number=self.run_numbers[0],
            angle=0.7,
            first_transmission_runs=self.first_transmission_runs,
            second_transmission_runs=self.second_transmission_runs,
        )
        self.finaliseResults()

    def regenerateReferenceFileByReducing(self):
        setupInstrument()
        test = ISISReflectometryWorkflowPreprocessingTest()
        test.runTest()
        SaveNexus(InputWorkspace=self.result_workspace_name, Filename=self.reference_file)
