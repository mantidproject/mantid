"""Test suite for the crystal field calculations in the Inelastic/CrystalField package
"""
import unittest
import numpy as np

from mantid.simpleapi import *
from CrystalField.energies import energies

class CrystalFieldTests(unittest.TestCase):

    def test_energies(self):
      en,wf, ham = energies(1,  B20=0.3365, B22=7.4851, B40=0.4062, B42=-3.8296, B44=-2.3210)
      n = len(en)
      wf_ctr = np.conj(wf.transpose())
      I = np.tensordot(wf_ctr, wf, axes = 1)
      for i in range(n):
            re = np.real(I[i, i])
            im = np.imag(I[i, i])
            self.assertAlmostEqual(re, 1.0, 10)
            self.assertAlmostEqual(im, 0.0, 10)
      tmp = np.tensordot(wf_ctr, ham, axes = 1)
      E = np.tensordot(tmp, wf, axes = 1)
      E = np.real([E[i, i] for i in range(n)])
      E -= np.amin(E)
      dlt = E - en
      for i in range(n):
            self.assertAlmostEqual(dlt[i], 0.0, 10)


if __name__=="__main__":
    unittest.main()
