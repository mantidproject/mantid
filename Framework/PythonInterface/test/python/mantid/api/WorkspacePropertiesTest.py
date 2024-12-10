# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""Tests the construction of the various workspace
property types
"""

import unittest
from mantid.api import (
    WorkspaceProperty,
    WorkspaceGroupProperty,
    MatrixWorkspaceProperty,
    IEventWorkspaceProperty,
    ITableWorkspaceProperty,
    IMDWorkspaceProperty,
    IMDHistoWorkspaceProperty,
    IMDEventWorkspaceProperty,
    PropertyMode,
    LockMode,
)
from mantid.kernel import Direction, Property


class WorkspacePropertiesTest(unittest.TestCase):
    def _do_test(self, classtype):
        self.assertTrue(issubclass(WorkspaceGroupProperty, Property))
        self._do_construction_with_name_default_direction(classtype)
        self._do_construction_with_name_default_direction_optional(classtype)

    def _do_construction_with_name_default_direction(self, classtype):
        prop = classtype("NoValidation", "test", Direction.Output)
        self.assertTrue(isinstance(prop, classtype))
        self.assertEqual("NoValidation", prop.name)
        self.assertEqual(Direction.Output, prop.direction)
        self.assertEqual("test", prop.valueAsStr)

    def _do_construction_with_name_default_direction_optional(self, classtype):
        prop = classtype("IsOptional", "test", Direction.Output, PropertyMode.Optional)
        self.assertTrue(isinstance(prop, classtype))
        self.assertEqual("IsOptional", prop.name)
        self.assertEqual(Direction.Output, prop.direction)
        self.assertTrue(prop.isOptional())
        self.assertEqual("test", prop.valueAsStr)

    def _do_construction_with_name_default_direction_optional_no_locking(self, classtype):
        prop = classtype("DoesNotLock", "test", Direction.Output, PropertyMode.Optional, LockMode.NoLock)
        self.assertTrue(isinstance(prop, classtype))
        self.assertEqual("DoesNotLock", prop.name)
        self.assertEqual(Direction.Output, prop.direction)
        self.assertTrue(prop.isOptional())
        self.assertFalse(prop.isLocking())
        self.assertEqual("test", prop.valueAsStr)

    def test_WorkspaceProperty_can_be_instantiated(self):
        self._do_test(WorkspaceProperty)

    def test_WorkspaceGroupProperty_can_be_instantiated(self):
        self._do_test(WorkspaceGroupProperty)

    def test_MatrixWorkspaceProperty_can_be_instantiated(self):
        self._do_test(MatrixWorkspaceProperty)

    def test_IEventWorkspaceProperty_can_be_instantiated(self):
        self._do_test(IEventWorkspaceProperty)

    def test_ITableWorkspaceProperty_can_be_instantiated(self):
        self._do_test(ITableWorkspaceProperty)

    def test_IMDHistoWorkspaceProperty_can_be_instantiated(self):
        self._do_test(IMDHistoWorkspaceProperty)

    def test_IMDEventWorkspaceProperty_can_be_instantiated(self):
        self._do_test(IMDEventWorkspaceProperty)

    def test_IMDWorkspaceProperty_can_be_instantiated(self):
        self._do_test(IMDWorkspaceProperty)


if __name__ == "__main__":
    unittest.main()
