# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, print_function)

import unittest
from mantid.simpleapi import *
from mantid.api import *

class MeanTest(unittest.TestCase):

    def test_throws_if_non_existing_names(self):
        a = CreateWorkspace(DataX=[1,2,3],DataY=[1,2,3],DataE=[1,1,1],UnitX='TOF')
        try:
            c = Mean(Workspaces='a,b') # 'b' does not exist.
        except RuntimeError:
            pass
        else:
            self.fail("Should not have got here. Should throw without both workspace names indexing real workspaces in IDF.")
        finally:
            DeleteWorkspace(a)

    def test_throws_if_workspace_axis0_unequal(self):
        a = CreateWorkspace(DataX=[1,2,3],DataY=[1,2,3],DataE=[1,1,1],UnitX='TOF')
        b = CreateWorkspace(DataX=[1,2,3,4],DataY=[1,2,3,4],DataE=[1,1,1,1],UnitX='TOF')
        try:
            c = Mean(Workspaces='a,b') # 'a' and 'b' are different sizes.
        except RuntimeError:
            pass
        else:
            self.fail("Should not have got here. Should throw as axis0 is unequal in size between a and b.")
        finally:
            DeleteWorkspace(a)
            DeleteWorkspace(b)

    def test_throws_if_workspace_axis1_unequal(self):
        a = CreateWorkspace(DataX=[1,2,3,4],DataY=[1,2,3,4],DataE=[1,1,1,1],UnitX='TOF',NSpec=1)
        b = CreateWorkspace(DataX=[1,2,3,4],DataY=[1,2,3,4],DataE=[1,1,1,1],UnitX='TOF',NSpec=2)
        try:
            c = Mean(Workspaces='a,b') # 'a' and 'b' are different sizes.
        except RuntimeError:
            pass
        else:
            self.fail("Should not have got here. Should throw as axis1 is unequal in size between a and b.")
        finally:
            DeleteWorkspace(a)
            DeleteWorkspace(b)

    def test_throws_if_workspace_unorded(self):
        a = CreateWorkspace(DataX=[1,2,1,2],DataY=[1,2,3,4],DataE=[1,1,1,1],UnitX='TOF',NSpec=2)
        b = CreateWorkspace(DataX=[1,2,2,1],DataY=[1,2,3,4],DataE=[1,1,1,1],UnitX='TOF',NSpec=2)
        try:
            c = Mean(Workspaces='a,b') # 'a' and 'b' have different x data.
        except RuntimeError:
            pass
        else:
            self.fail("Should not have got here. Should throw as the x data is unsorted")
        finally:
            DeleteWorkspace(a)
            DeleteWorkspace(b)

    def test_mean(self):
        a = CreateWorkspace(DataX=[1,2,3],DataY=[1,2,3],DataE=[1,1,1],UnitX='TOF')
        b = CreateWorkspace(DataX=[1,2,3],DataY=[1,2,3],DataE=[1,1,1],UnitX='TOF')
        c = Mean(Workspaces='a,b')
        d = (a + b) / 2 # Do algorithm work manually for purposes of comparison.
        message = CompareWorkspaces(Workspace1=c, Workspace2=d)
        self.assertTrue(message[0])

        # Clean-up
        DeleteWorkspace(a)
        DeleteWorkspace(b)
        DeleteWorkspace(c)
        DeleteWorkspace(d)

if __name__ == '__main__':
    unittest.main()
