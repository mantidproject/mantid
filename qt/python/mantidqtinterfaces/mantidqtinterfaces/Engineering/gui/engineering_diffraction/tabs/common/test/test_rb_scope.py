# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.common.rb_scope import RbScope, RbScopeConsumer


class _Consumer(RbScopeConsumer):
    def __init__(self):
        self._rb_scope = RbScope()


class RbScopeTest(unittest.TestCase):
    def test_starts_empty(self):
        self.assertIsNone(RbScope().rb_num)

    def test_blank_entries_normalise_to_none(self):
        for blank in (None, "", "   ", "\t"):
            self.assertIsNone(RbScope(blank).rb_num, msg=repr(blank))

    def test_surrounding_whitespace_is_stripped(self):
        self.assertEqual(RbScope("  12345 ").rb_num, "12345")

    def test_set_reports_whether_the_value_changed(self):
        scope = RbScope()

        self.assertTrue(scope.set("12345"))
        # the RB field emits on every keystroke, so callers rely on this to avoid needless work
        self.assertFalse(scope.set("12345"))
        self.assertFalse(scope.set(" 12345 "), msg="normalised value is unchanged")
        self.assertTrue(scope.set("99999"))

    def test_clearing_returns_to_the_global_scope(self):
        scope = RbScope("12345")

        self.assertTrue(scope.set(""))
        self.assertIsNone(scope.rb_num)


class RbScopeConsumerTest(unittest.TestCase):
    def test_rb_num_reads_through_to_the_scope(self):
        consumer = _Consumer()
        scope = RbScope("12345")

        consumer.set_rb_scope(scope)

        self.assertEqual(consumer.rb_num, "12345")

    def test_consumers_sharing_a_scope_stay_in_step(self):
        shared = RbScope()
        first, second = _Consumer(), _Consumer()
        first.set_rb_scope(shared)
        second.set_rb_scope(shared)

        first.set_rb_num("12345")

        self.assertEqual(second.rb_num, "12345")
        self.assertEqual(shared.rb_num, "12345")

    def test_assigning_rb_num_writes_through_to_the_scope(self):
        shared = RbScope()
        consumer = _Consumer()
        consumer.set_rb_scope(shared)

        consumer.rb_num = "  12345 "

        self.assertEqual(shared.rb_num, "12345")

    def test_consumers_are_independent_before_a_scope_is_shared(self):
        # a presenter constructed on its own (as in the tab unit tests) still works
        first, second = _Consumer(), _Consumer()

        first.set_rb_num("12345")

        self.assertEqual(first.rb_num, "12345")
        self.assertIsNone(second.rb_num)


if __name__ == "__main__":
    unittest.main()
