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
        self.assertEquals(type(wksp), MatrixWorkspace)
        self.assertEquals(wksp.id(), "Workspace2D")
        self.assertEquals(wksp.getNumberHistograms(), nhist)
        self.assertEquals(len(wksp.readX(0)), xlength)
        self.assertEquals(wksp.blocksize(), ylength)

    def test_creating_a_clean_workspace_is_correct_size_and_type(self):
        nhist = 2
        xlength = 3
        ylength = 4
        wksp = self._create_clean_workspace(nhist, xlength, ylength)
        self._verify(wksp, nhist, xlength, ylength)

    def test_creating_a_workspace_from_another_gives_one_of_same_size(self):
        nhist = 2
        xlength = 3
        ylength = 4
        clean = self._create_clean_workspace(nhist, xlength, ylength)
        copy = WorkspaceFactory.create(clean)
        self._verify(copy, nhist, xlength, ylength)

    def test_creating_a_workspace_from_another_with_different_size(self):
        clean = self._create_clean_workspace(nhist=2, xlength=3, ylength=4)
        nhist = 4
        xlength = 5
        ylength = 6
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
