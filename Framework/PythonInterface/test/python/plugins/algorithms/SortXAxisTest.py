import unittest
from mantid.kernel import *
from mantid.api import *
from mantid.simpleapi import *

class SortXAxisTest(unittest.TestCase):

    def test_x_ascending(self):
        dataX = [1, 2, 3] # In ascending order, so y and e will need to be reversed.
        dataY = [1, 2, 3]
        dataE = [1, 2, 3]
        unsortedws = CreateWorkspace(DataX=dataX,DataY=dataY,DataE=dataE,UnitX='TOF',Distribution=True)
        # Run the algorithm
        sortedws = SortXAxis(InputWorkspace=unsortedws)
        sortedX = sortedws.readX(0)
        sortedY = sortedws.readY(0)
        sortedE = sortedws.readE(0)
        # Check the resulting data values. Sorting operation should have resulted in no changes
        self.assertEqual(dataX, sortedX.tolist())
        self.assertEqual(dataY, sortedY.tolist())
        self.assertEqual(dataE, sortedE.tolist())
        DeleteWorkspace(unsortedws)
        DeleteWorkspace(sortedws)


    def test_x_descending(self):
        dataX = [3, 2, 1] # In descending order, so y and e will need to be reversed.
        dataY = [1, 2, 3]
        dataE = [1, 2, 3]
        unsortedws = CreateWorkspace(DataX=dataX,DataY=dataY,DataE=dataE,UnitX='TOF',Distribution=True)
        # Run the algorithm
        sortedws = SortXAxis(InputWorkspace=unsortedws)
        sortedX = sortedws.readX(0)
        sortedY = sortedws.readY(0)
        sortedE = sortedws.readE(0)
        # Check the resulting data values.
        self.assertEqual(sorted(dataX), sortedX.tolist())
        dataY.reverse()
        dataE.reverse()
        self.assertEqual(dataY, sortedY.tolist())
        self.assertEqual(dataE, sortedE.tolist())
        DeleteWorkspace(unsortedws)
        DeleteWorkspace(sortedws)

    def test_on_multiple_spectrum(self):
        dataX = [3, 2, 1, 3, 2, 1] # In descending order, so y and e will need to be reversed.
        dataY = [1, 2, 3, 1, 2, 3]
        dataE = [1, 2, 3, 1, 2, 3]
        unsortedws = CreateWorkspace(DataX=dataX,DataY=dataY,DataE=dataE,UnitX='TOF',Distribution=True, NSpec=2)
        dataY.reverse()
        dataE.reverse()
        # Run the algorithm
        sortedws = SortXAxis(InputWorkspace=unsortedws)
        # Check the resulting data values for 1st spectrum.
        sortedX = sortedws.readX(0)
        sortedY = sortedws.readY(0)
        sortedE = sortedws.readE(0)
        self.assertEqual(sorted(dataX[0:3]), sortedX.tolist())
        self.assertEqual(dataY[0:3], sortedY.tolist())
        self.assertEqual(dataE[0:3], sortedE.tolist())
        # Check the resulting data values for 2nd spectrum.
        sortedX = sortedws.readX(1)
        sortedY = sortedws.readY(1)
        sortedE = sortedws.readE(1)
        self.assertEqual(sorted(dataX[3:]), sortedX.tolist())
        self.assertEqual(dataY[3:], sortedY.tolist())
        self.assertEqual(dataE[3:], sortedE.tolist())
        DeleteWorkspace(unsortedws)
        DeleteWorkspace(sortedws)


    def test_sorts_x_histogram_ascending(self):
        dataX = [1, 2, 3, 4]
        dataY = [1, 2, 3]
        dataE = [1, 2, 3]
        unsortedws = CreateWorkspace(DataX=dataX,DataY=dataY,DataE=dataE,UnitX='TOF',Distribution=False)
        # Run the algorithm
        sortedws = SortXAxis(InputWorkspace=unsortedws)
        sortedX = sortedws.readX(0)
        sortedY = sortedws.readY(0)
        sortedE = sortedws.readE(0)
        # Check the resulting data values. Sorting operation should have resulted in no changes
        self.assertEqual(dataX, sortedX.tolist())
        self.assertEqual(dataY, sortedY.tolist())
        self.assertEqual(dataE, sortedE.tolist())

        DeleteWorkspace(unsortedws)
        DeleteWorkspace(sortedws)

    def test_sorts_x_histogram_descending(self):
        dataX = [4, 3, 2, 1]
        dataY = [1, 2, 3]
        dataE = [1, 2, 3]
        unsortedws = CreateWorkspace(DataX=dataX,DataY=dataY,DataE=dataE,UnitX='TOF',Distribution=False)
        # Run the algorithm
        sortedws = SortXAxis(InputWorkspace=unsortedws)
        sortedX = sortedws.readX(0)
        sortedY = sortedws.readY(0)
        sortedE = sortedws.readE(0)
        # Check the resulting data values. Sorting operation should have resulted in no changes
        self.assertEqual(sorted(dataX), sortedX.tolist())
        dataY.reverse()
        dataE.reverse()
        self.assertEqual(dataY, sortedY.tolist())
        self.assertEqual(dataE, sortedE.tolist())

        DeleteWorkspace(unsortedws)
        DeleteWorkspace(sortedws)



if __name__ == '__main__':
    unittest.main()