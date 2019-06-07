# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

import unittest
from mantid.api import (WorkspaceFactory, WorkspaceFactoryImpl, MatrixWorkspace,
                    ITableWorkspace, IPeaksWorkspace)

class WorkspaceFactoryTest(unittest.TestCase):


    def test_WorkspaceFactory_isinstance_of_WorkspaceFactoryImpl(self):
        self.assertTrue(isinstance(WorkspaceFactory, WorkspaceFactoryImpl))

    def _create_clean_workspace(self, nhist, xlength, ylength):
        return WorkspaceFactory.create("Workspace2D", NVectors=nhist,
                                       XLength=xlength, YLength=ylength)

    def _verify(self, wksp, nhist, xlength, ylength):
        self.assertTrue(isinstance(wksp, MatrixWorkspace))
        self.assertEqual(wksp.id(), "Workspace2D")
        self.assertEqual(wksp.getNumberHistograms(), nhist)
        self.assertEqual(len(wksp.readX(0)), xlength)
        self.assertEqual(wksp.blocksize(), ylength)

    def test_creating_a_clean_workspace_is_correct_size_and_type(self):
        nhist = 2
        xlength = 4
        ylength = 3
        wksp = self._create_clean_workspace(nhist, xlength, ylength)
        self._verify(wksp, nhist, xlength, ylength)

    def test_creating_a_workspace_from_another_gives_one_of_same_size(self):
        nhist = 2
        xlength = 4
        ylength = 3
        clean = self._create_clean_workspace(nhist, xlength, ylength)
        copy = WorkspaceFactory.create(clean)
        self._verify(copy, nhist, xlength, ylength)

    def test_creating_a_workspace_from_another_with_different_size(self):
        clean = self._create_clean_workspace(nhist=2, xlength=4, ylength=3)
        nhist = 4
        xlength = 6
        ylength = 5
        copy = WorkspaceFactory.create(clean, nhist, xlength, ylength)
        self._verify(copy, nhist, xlength, ylength)

    def test_creating_a_tableworkspace(self):
        table = WorkspaceFactory.createTable()
        self.assertTrue(isinstance(table, ITableWorkspace))

    def test_creating_a_peaksworkspace(self):
        peaks = WorkspaceFactory.createPeaks()
        self.assertTrue(isinstance(peaks, IPeaksWorkspace))

if __name__ == '__main__':
    unittest.main()
