"""Tests the construction of the various workspace
property types
"""
from __future__ import (absolute_import, division, print_function)
import unittest
from mantid.api import (IWorkspacePropertyWithIndex, WorkspacePropertyWithIndex,
                        IEventWorkspacePropertyWithIndex)
from mantid.dataobjects import (EventWorkspacePropertyWithIndex)


class WorkspacePropertiesWithIndexTest(unittest.TestCase):
    def _do_test(self, classtype):
        self._do_construction(classtype)

    def _do_construction(self, classtype):
        prop = classtype("InputWorkspace")
        self.assertTrue(isinstance(prop, IWorkspacePropertyWithIndex))
        self.assertEquals("InputWorkspace", prop.name)

    def test_MatrixWorkspacePropertyWithIndex_can_be_instantiated(self):
        self._do_test(WorkspacePropertyWithIndex)

    def test_IEventWorkspacePropertyWithIndex_can_be_instantiated(self):
        self._do_test(IEventWorkspacePropertyWithIndex)

    def test_EventWorkspacePropertyWithIndex_can_be_instantiated(self):
        self._do_test(EventWorkspacePropertyWithIndex)


if __name__ == "__main__":
    unittest.main()
