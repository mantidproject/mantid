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
                              PDConvertReciprocalSpace)
import numpy as np


class PDConvertReciprocalSpaceTest(unittest.TestCase):
    def test_simple(self):
        input_x = np.array([0.1,0.2,0.3,0.4,0.5])
        input_y = np.array([1.,2,3,2,1])
        input_e = np.array([0.1,0.14,0.17,0.14,0.1])
        sq = CreateWorkspace(DataX=input_x,
                             DataY=input_y,
                             DataE=input_e,
                             UnitX='MomentumTransfer')
        SetSampleMaterial(InputWorkspace=sq, ChemicalFormula='Ar')
        fq = PDConvertReciprocalSpace(InputWorkspace=sq,
                                      To='F(Q)',
                                      From='S(Q)')
        x=fq.readX(0)
        y=fq.readY(0)
        e=fq.readE(0)
        self.assertTrue(np.array_equal(x, input_x))
        self.assertTrue(np.array_equal(y, input_x*(input_y-1)))
        self.assertTrue(np.array_equal(e, input_x*input_e))
        fkq = PDConvertReciprocalSpace(InputWorkspace=sq,
                                       To='FK(Q)',
                                       From='S(Q)')
        x=fkq.readX(0)
        y=fkq.readY(0)
        e=fkq.readE(0)
        bsq = sq.sample().getMaterial().cohScatterLengthSqrd()
        self.assertTrue(np.array_equal(x, input_x))
        self.assertTrue(np.allclose(y, bsq*(input_y-1)))
        self.assertTrue(np.allclose(e, bsq*input_e))

if __name__=="__main__":
    unittest.main()
