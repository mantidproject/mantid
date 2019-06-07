# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

import unittest
from mantid.simpleapi import (CreateWorkspace,
                              SetSampleMaterial,
                              PDConvertRealSpace)
import numpy as np


class PDConvertReciprocalSpaceTest(unittest.TestCase):
    def test_simple(self):
        input_x = np.array([0.1,0.2,0.3,0.4,0.5])
        input_y = np.array([1.,2,3,2,1])
        input_e = np.array([0.1,0.14,0.17,0.14,0.1])
        Gr = CreateWorkspace(DataX=input_x,
                             DataY=input_y,
                             DataE=input_e)
        SetSampleMaterial(InputWorkspace=Gr, ChemicalFormula='Ar')
        GKr = PDConvertRealSpace(InputWorkspace=Gr,
                                 To='GK(r)',
                                 From='G(r)')
        x=GKr.readX(0)
        y=GKr.readY(0)
        e=GKr.readE(0)

        bsq = Gr.sample().getMaterial().cohScatterLengthSqrd()
        rho = Gr.sample().getMaterial().numberDensity
        factor = bsq / (4. * np.pi *rho)
        self.assertTrue(np.array_equal(x, input_x))
        self.assertTrue(np.allclose(y, factor*input_y/input_x))
        self.assertTrue(np.allclose(e, factor*input_e/input_x))

if __name__=="__main__":
    unittest.main()
