"""Test suite for the crystal field calculations in the Inelastic/CrystalField package
"""
import unittest
import numpy as np

from mantid.simpleapi import *
from CrystalField.energies import energies

class CrystalFieldTests(unittest.TestCase):

      def _do_test_eigensystem(self, en, wf, ham):
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
            ev = np.real([E[i, i] for i in range(n)])
            emin = np.amin(ev)
            for i in range(n):
                  for j in range(n):
                        if (i == j):
                              self.assertAlmostEqual(np.real(E[i, j] - emin), en[i], 10)
                              self.assertAlmostEqual(np.imag(E[i, j]), 0.0, 10)
                        else:
                              self.assertAlmostEqual(np.real(E[i, j]), 0.0, 10)
                              self.assertAlmostEqual(np.imag(E[i, j]), 0.0, 10)

      def test_C2v(self):
            en, wf, ham = energies(1,  B20=0.3365, B22=7.4851, B40=0.4062, B42=-3.8296, B44=-2.3210)
            self._do_test_eigensystem(en, wf, ham)

      def test_C2(self):
            en, wf, ham = energies(1,  B20=0.3365, B22=7.4851, B40=0.4062, IB42=-3.8296, IB44=-2.3210)
            self._do_test_eigensystem(en, wf, ham)

      def test_C2v_mol(self):
            en, wf, ham = energies(1,  B20=0.3365, B22=7.4851, B40=0.4062, B42=-3.8296, B44=-2.3210, Bmol=[1, 2, 3])
            self._do_test_eigensystem(en, wf, ham)

      def test_C2v_ext(self):
            en, wf, ham = energies(1,  B20=0.3365, B22=7.4851, B40=0.4062, B42=-3.8296, B44=-2.3210, Bext=[1, 2, 3])
            self._do_test_eigensystem(en, wf, ham)


if __name__=="__main__":
    unittest.main()
