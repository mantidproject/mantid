# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)
import os
from mantid.simpleapi import *
from mantid import api
import unittest
from anAdsorbtionShape import anAdsorbtionShape 

class AdsorbtionShapesTest(unittest.TestCase):
    def __init__(self, methodName):
        return super(AdsorbtionShapesTest, self).__init__(methodName)

    def test_build_subst_dictionary(self):
        ash = anAdsorbtionShape(['V',10]);
        res = ash.material;
        self.assertEqual(res['ChemicalFormula'],'V')
        self.assertEqual(res['SampleNumberDensity'],10)


        ash.material = {'ChemicalFormula':'Al','SampleNumberDensity':0.5}
        res = ash.material;
        self.assertEqual(res['ChemicalFormula'],'Al')
        self.assertEqual(res['SampleNumberDensity'],0.5)


if __name__=="__main__":
    #unittest.main()
    ast = AdsorbtionShapesTest('test_build_subst_dictionary')
    ast.test_build_subst_dictionary()
