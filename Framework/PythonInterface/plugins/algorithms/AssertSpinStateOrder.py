# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init,invalid-name

from mantid.api import AlgorithmFactory, PythonAlgorithm, WorkspaceGroupProperty
from mantid.simpleapi import DetermineSpinStateOrder
from mantid.kernel import logger, Direction


class AssertSpinStateOrder(PythonAlgorithm):
    def category(self):
        return "SANS"

    def name(self):
        return "AssertSpinStateOrder"

    def summary(self):
        return "Assert that the given Polarized SANS data has the given spin state order using DetermineSpinStateOrder."

    def PyInit(self):
        self.declareProperty(
            WorkspaceGroupProperty(name="InputWorkspace", defaultValue="", direction=Direction.Input),
            doc="Polarized SANS runs with 4 periods (workspace group with 4 entries).",
        )

        self.declareProperty(
            name="ExpectedSpinStates",
            defaultValue="",
            direction=Direction.Input,
            doc='Comma separate list of spin states (e.g "00,01,10,11") in the expected order of the group workspace periods.',
        )

        self.declareProperty(
            name="Reorder",
            defaultValue=False,
            direction=Direction.Input,
            doc="If set to true, if the workspace is has it's periods in a different order, "
            "reorder the workspace to match ExpectedSpinStates.",
        )

        self.declareProperty(
            name="Result",
            defaultValue=True,
            direction=Direction.Output,
            doc="Bool value stating whether the input workspace was found to have the same ordering as the ExpectSpinStates string",
        )

    def validateInputs(self):
        return {}

    def PyExec(self):
        group_ws = self.getProperty("InputWorkspace").value
        expected_spin_states = self.getProperty("ExpectedSpinStates").value
        found_spin_states = DetermineSpinStateOrder(group_ws)
        matching = expected_spin_states == found_spin_states
        self.setProperty("Result", matching)

        if not matching:
            logger.warning(
                f"Expected {group_ws.getName()} to have the spin state order '{expected_spin_states}'"
                f" but actually found 'f{found_spin_states}'"
            )

            if self.getProperty("Reorder").value:
                expected_spin_state_list = expected_spin_states.split(",")
                found_spin_state_list = found_spin_states.split(",")

                desired_order = [expected_spin_state_list.index(spin_state) for spin_state in found_spin_state_list]
                group_ws.reorder(desired_order)
                logger.warning(f"Reordered {group_ws.getName()} to the correct spin state order '{expected_spin_states}'")


AlgorithmFactory.subscribe(AssertSpinStateOrder)
