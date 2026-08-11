# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX-License-Identifier: GPL-3.0+
import unittest

from mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.common.instrument_scope import (
    InstrumentScope,
    InstrumentScopeConsumer,
)


class _Consumer(InstrumentScopeConsumer):
    def __init__(self):
        self._instrument_scope = InstrumentScope()
        self.reactions = 0

    def _on_instrument_changed(self) -> None:
        self.reactions += 1


class InstrumentScopeTest(unittest.TestCase):
    def test_starts_on_the_first_instrument_in_the_combo_box(self):
        self.assertEqual(InstrumentScope().instrument, "ENGINX")

    def test_set_reports_whether_the_value_changed(self):
        scope = InstrumentScope()

        self.assertTrue(scope.set("IMAT"))
        self.assertFalse(scope.set("IMAT"))
        self.assertTrue(scope.set("ENGINX"))

    def test_set_from_index_maps_the_combo_box_position(self):
        scope = InstrumentScope()

        self.assertTrue(scope.set_from_index(1))
        self.assertEqual(scope.instrument, "IMAT")

        self.assertTrue(scope.set_from_index(0))
        self.assertEqual(scope.instrument, "ENGINX")


class InstrumentScopeConsumerTest(unittest.TestCase):
    def test_instrument_reads_through_to_the_scope(self):
        consumer = _Consumer()
        scope = InstrumentScope("IMAT")

        consumer.set_instrument_scope(scope)

        self.assertEqual(consumer.instrument, "IMAT")

    def test_consumers_sharing_a_scope_stay_in_step(self):
        shared = InstrumentScope()
        first, second = _Consumer(), _Consumer()
        first.set_instrument_scope(shared)
        second.set_instrument_scope(shared)

        first.set_instrument_override(1)

        self.assertEqual(second.instrument, "IMAT")
        self.assertEqual(shared.instrument, "IMAT")

    def test_assigning_instrument_writes_through_to_the_scope(self):
        shared = InstrumentScope()
        consumer = _Consumer()
        consumer.set_instrument_scope(shared)

        consumer.instrument = "IMAT"

        self.assertEqual(shared.instrument, "IMAT")

    def test_override_lets_the_consumer_react(self):
        consumer = _Consumer()

        consumer.set_instrument_override(1)

        self.assertEqual(consumer.instrument, "IMAT")
        self.assertEqual(consumer.reactions, 1)

    def test_consumers_react_even_when_the_instrument_is_unchanged(self):
        # the combo box only emits on a real change, and the tabs rebuild view state from the
        # instrument, so reacting unconditionally is the safe default
        consumer = _Consumer()

        consumer.set_instrument_override(0)

        self.assertEqual(consumer.reactions, 1)

    def test_consumers_are_independent_before_a_scope_is_shared(self):
        # a presenter constructed on its own (as in the tab unit tests) still works
        first, second = _Consumer(), _Consumer()

        first.set_instrument_override(1)

        self.assertEqual(first.instrument, "IMAT")
        self.assertEqual(second.instrument, "ENGINX")


if __name__ == "__main__":
    unittest.main()
