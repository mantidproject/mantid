# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from MultiPlotting.gridspec_engine import defaultGridspecGrid


class DefaultGridSpecTest(unittest.TestCase):

    def test_someSquares(self):
        roots = [2, 3, 4, 5, 123]
        for root in roots:
            result = defaultGridspecGrid(root*root)
            self.assertEquals(result, [root, root])
 
    def test_notSquares(self):
        roots = [3, 4, 5, 123]
        for root in roots:
            result = defaultGridspecGrid(root*root-1)
            self.assertEquals(result, [root, root])

    def test_specialCaseOne(self):
        number = 1
        result = defaultGridspecGrid(number)
        self.assertEquals(result, [1, 1])

    def test_specialCaseTwo(self):
        number = 2
        result = defaultGridspecGrid(number)
        self.assertEquals(result, [1, 2])

    def test_specialCaseThree(self):
        number = 3
        result = defaultGridspecGrid(number)
        self.assertEquals(result, [3, 1])

    def test_smallerGrid(self):
        number = 5
        result = defaultGridspecGrid(number)
        self.assertEquals(result, [2, 3])


if __name__ == "__main__":
    unittest.main()
