# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
"""Test suite for the crystal field calculations in the Inelastic/CrystalField package
"""
from __future__ import (absolute_import, division, print_function)
import unittest
import numpy as np
from scipy.constants import physical_constants

# Import mantid to setup the python paths to the bundled scripts
import mantid
from CrystalField.energies import energies
from mantid.simpleapi import CalculateChiSquared, EvaluateFunction, mtd

c_mbsr = 79.5774715459  # Conversion from barn to mb/sr


class CrystalFieldTests(unittest.TestCase):

    def _do_test_eigensystem(self, en, wf, ham):
        n = len(en)
        wf_ctr = np.conj(wf.transpose())
        I = np.tensordot(wf_ctr, wf, axes=1)
        for i in range(n):
            re = np.real(I[i, i])
            im = np.imag(I[i, i])
            self.assertAlmostEqual(re, 1.0, 10)
            self.assertAlmostEqual(im, 0.0, 10)
        tmp = np.tensordot(wf_ctr, ham, axes=1)
        E = np.tensordot(tmp, wf, axes=1)
        ev = np.real([E[i, i] for i in range(n)])
        emin = np.amin(ev)
        for i in range(n):
            for j in range(n):
                if i == j:
                    self.assertAlmostEqual(np.real(E[i, j] - emin), en[i], 10)
                    self.assertAlmostEqual(np.imag(E[i, j]), 0.0, 10)
                else:
                    self.assertAlmostEqual(np.real(E[i, j]), 0.0, 10)
                    self.assertAlmostEqual(np.imag(E[i, j]), 0.0, 10)

    def test_C2v(self):
        en, wf, ham = energies(1, B20=0.3365, B22=7.4851, B40=0.4062, B42=-3.8296, B44=-2.3210)
        self._do_test_eigensystem(en, wf, ham)

    def test_C2(self):
        en, wf, ham = energies(1, B20=0.3365, B22=7.4851, B40=0.4062, IB42=-3.8296, IB44=-2.3210)
        self._do_test_eigensystem(en, wf, ham)

    def test_C2v_mol(self):
        en, wf, ham = energies(1, B20=0.3365, B22=7.4851, B40=0.4062, B42=-3.8296, B44=-2.3210,
                               BmolX=1.0, BmolY=2.0, BmolZ=3.0)
        self._do_test_eigensystem(en, wf, ham)

    def test_C2v_ext(self):
        en, wf, ham = energies(1, B20=0.3365, B22=7.4851, B40=0.4062, B42=-3.8296, B44=-2.3210,
                               BextX=1.0, BextY=2.0, BextZ=3.0)
        self._do_test_eigensystem(en, wf, ham)

    def test_upd3(self):
        # Parameters are from Phys Rev B 89 235114 / arXiv:1403.4785, originally calculated using McPhase
        # Ion is U4+ which is equivalent to Pr3+ (5f2 instead of 4f2)
        en, wf, ham = energies(2, B20=0.035, B40=-0.012, B43=-0.027, B60=-0.00012, B63=0.0025, B66=0.0068)
        self._do_test_eigensystem(en, wf, ham)
        expectedEigenvalues = [0, 0, 4.36, 9.64, 9.64, 20.65, 30.03, 30.03, 52.50]
        emin = np.amin(en)
        for i in range(9):
            self.assertAlmostEqual(expectedEigenvalues[i], en[i], 1)
        # Now test the eigenvectors by computing the dipole transition matrix elements. Use the magnetic field
        #   terms but divide by gJ*uB (gJ=0.8 for U4+/Pr3+ and uB=0.05788 meV/T)
        _, _, hx = energies(2, BextX=1.0)
        _, _, hy = energies(2, BextY=1.0)
        _, _, hz = energies(2, BextZ=1.0)
        ix = np.dot(np.conj(np.transpose(wf)), np.dot(hx, wf))
        iy = np.dot(np.conj(np.transpose(wf)), np.dot(hy, wf))
        iz = np.dot(np.conj(np.transpose(wf)), np.dot(hz, wf))
        gJuB = 0.8 * physical_constants['Bohr magneton in eV/T'][0] * 1000.
        trans = np.multiply(ix, np.conj(ix)) + np.multiply(iy, np.conj(iy)) + np.multiply(iz, np.conj(iz))
        # For some reason, in the paper I also divided the matrix elements by a factor of 4. (not sure why)
        trans = trans / (gJuB ** 2) / 4
        expectedDipoleTM = [0, 0, 0.31, 1.24 / 2., 1.24 / 2., 2.04, 3.4 / 2., 3.4 / 2., 0]
        for i in range(2, 9):
            self.assertAlmostEqual(expectedDipoleTM[i], trans[i, 0] + trans[i, 1], 1)

    def test_api_CrystalField(self):
        from CrystalField import CrystalField
        cf = CrystalField('Pr', 'C2v', B20=0.035, B40=-0.012, B43=-0.027, B60=-0.00012, B63=0.0025, B66=0.0068)
        def set_ion(x):
            cf.Ion = x
        self.assertRaises(RuntimeError, set_ion, 'He')
        def set_symmetry(x):
            cf.Symmetry = x
        self.assertRaises(RuntimeError, set_symmetry, 'G')

    def test_api_CrystalField_eigensystem(self):
        from CrystalField import CrystalField
        cf = CrystalField('Ce', 'C2v', B20=0.035, B40=-0.012, B43=-0.027, B60=-0.00012, B63=0.0025, B66=0.0068)

        cf._calcEigensystem()
        ev = cf._eigenvalues
        self.assertAlmostEqual(ev[0], 0.0, 10)
        self.assertAlmostEqual(ev[1], 0.0, 10)
        self.assertAlmostEqual(ev[2], 1.44393213, 8)
        self.assertAlmostEqual(ev[3], 1.44393213, 8)
        self.assertAlmostEqual(ev[4], 3.85696607, 8)
        self.assertAlmostEqual(ev[5], 3.85696607, 8)

    def test_api_CrystalField_peaks_list(self):
        from CrystalField import CrystalField
        cf = CrystalField('Ce', 'C2v', B20=0.035, B40=-0.012, B43=-0.027, B60=-0.00012, B63=0.0025, B66=0.0068,
                          Temperature=44.0)
        pl = cf.getPeakList()
        self.assertEqual(pl.shape, (2, 7))
        self.assertAlmostEqual(pl[0, 0], 0.0, 10)
        self.assertAlmostEqual(pl[1, 0], 1.99118947*c_mbsr, 6)
        self.assertAlmostEqual(pl[0, 1], 3.85696607, 8)
        self.assertAlmostEqual(pl[1, 1], 0.86130642*c_mbsr, 6)
        self.assertAlmostEqual(pl[0, 2], 2.41303393, 8)
        self.assertAlmostEqual(pl[1, 2], 0.37963778*c_mbsr, 6)

    def test_api_CrystalField_peaks_list_2(self):
        from CrystalField import CrystalField
        cf = CrystalField('Ce', 'C2v', B20=0.035, B40=-0.012, B43=-0.027, B60=-0.00012, B63=0.0025, B66=0.0068,
                          Temperature=[44.0, 50.0])
        pl1 = cf.getPeakList()
        self.assertEqual(pl1.shape, (2, 7))
        self.assertAlmostEqual(pl1[0, 0], 0.0, 10)
        self.assertAlmostEqual(pl1[1, 0], 1.99118947*c_mbsr, 6)
        self.assertAlmostEqual(pl1[0, 1], 3.85696607, 8)
        self.assertAlmostEqual(pl1[1, 1], 0.86130642*c_mbsr, 6)
        self.assertAlmostEqual(pl1[0, 2], 2.41303393, 8)
        self.assertAlmostEqual(pl1[1, 2], 0.37963778*c_mbsr, 6)

        pl2 = cf.getPeakList(1)
        self.assertEqual(pl2.shape, (2, 7))
        self.assertAlmostEqual(pl2[0, 0], 0.0, 10)
        self.assertAlmostEqual(pl2[1, 0], 1.97812511*c_mbsr, 6)
        self.assertAlmostEqual(pl2[0, 1], 3.85696607, 8)
        self.assertAlmostEqual(pl2[1, 1], 0.82930948*c_mbsr, 6)
        self.assertAlmostEqual(pl2[0, 2], 2.41303393, 8)
        self.assertAlmostEqual(pl2[1, 2], 0.38262684*c_mbsr, 6)

    def test_api_CrystalField_spectrum(self):
        from CrystalField import CrystalField
        cf = CrystalField('Ce', 'C2v', B20=0.035, B40=-0.012, B43=-0.027, B60=-0.00012, B63=0.0025, B66=0.0068,
                          Temperature=[4.0, 50.0], FWHM=[0.1, 0.2], ToleranceIntensity=0.001*c_mbsr)
        x, y = cf.getSpectrum(0)
        y = y / c_mbsr
        self.assertAlmostEqual(y[60], 5.5233309477919823, 8)
        self.assertAlmostEqual(y[61], 10.116727004063931, 8)
        self.assertAlmostEqual(y[62], 12.177082168362135, 8)
        self.assertAlmostEqual(y[63], 7.6398117443793403, 8)
        self.assertAlmostEqual(y[64], 4.0801494675760672, 8)
        x, y = cf.getSpectrum(1)
        y = y / c_mbsr
        self.assertAlmostEqual(y[45], 0.29821516329781927, 8)
        self.assertAlmostEqual(y[46], 0.46179337379270108, 8)
        self.assertAlmostEqual(y[47], 0.66074332157852089, 8)
        self.assertAlmostEqual(y[48], 0.69469960124931895, 8)
        self.assertAlmostEqual(y[49], 0.51366004798691856, 8)

    def test_api_CrystalField_spectrum_from_list(self):
        from CrystalField import CrystalField
        cf = CrystalField('Ce', 'C2v', B20=0.035, B40=-0.012, B43=-0.027, B60=-0.00012, B63=0.0025, B66=0.0068,
                          Temperature=[4.0, 50.0], FWHM=[0.1, 0.2])

        r = [0.0, 1.45, 2.4, 3.0, 3.85]
        x, y = cf.getSpectrum(0, r)
        y = y / c_mbsr
        self.assertEqual(x[0], 0.0)
        self.assertEqual(x[1], 1.45)
        self.assertEqual(x[2], 2.4)
        self.assertEqual(x[3], 3.0)
        self.assertEqual(x[4], 3.85)

        self.assertAlmostEqual(y[0], 12.474945990071641, 6)
        self.assertAlmostEqual(y[1], 1.190159993510953, 6)
        self.assertAlmostEqual(y[2], 0.12278465143339329, 6)
        self.assertAlmostEqual(y[3], 0.042940202606241519, 6)
        self.assertAlmostEqual(y[4], 10.83716957556323, 6)

        x, y = cf.getSpectrum(1, r)
        y = y / c_mbsr
        self.assertEqual(x[0], 0.0)
        self.assertEqual(x[1], 1.45)
        self.assertEqual(x[2], 2.4)
        self.assertEqual(x[3], 3.0)
        self.assertEqual(x[4], 3.85)

        self.assertAlmostEqual(y[0], 6.3046623789675627, 8)
        self.assertAlmostEqual(y[1], 0.33121840136135056, 8)
        self.assertAlmostEqual(y[2], 1.2246810731541884, 8)
        self.assertAlmostEqual(y[3], 0.078540347981549338, 8)
        self.assertAlmostEqual(y[4], 2.6380494258301161, 8)

    def test_api_CrystalField_spectrum_0(self):
        from CrystalField import CrystalField
        cf = CrystalField('Ce', 'C2v', B20=0.035, B40=-0.012, B43=-0.027, B60=-0.00012, B63=0.0025, B66=0.0068,
                          Temperature=4.0, FWHM=0.1, ToleranceIntensity=0.001*c_mbsr)
        x, y = cf.getSpectrum()
        y = y / c_mbsr
        self.assertAlmostEqual(y[60], 5.52333486, 8)
        self.assertAlmostEqual(y[61], 10.11673418, 8)
        self.assertAlmostEqual(y[62], 12.1770908, 8)
        self.assertAlmostEqual(y[63], 7.63981716, 8)
        self.assertAlmostEqual(y[64], 4.08015236, 8)

    def test_api_CrystalField_spectrum_ws(self):
        from CrystalField import CrystalField
        from mantid.simpleapi import CreateWorkspace
        cf = CrystalField('Ce', 'C2v', B20=0.035, B40=-0.012, B43=-0.027, B60=-0.00012, B63=0.0025, B66=0.0068,
                          Temperature=[4.0, 50.0], FWHM=[0.1, 0.2])

        x = np.linspace(0.0, 2.0, 30)
        y = np.zeros_like(x)
        e = np.ones_like(x)
        workspace = CreateWorkspace(x, y, e)

        x, y = cf.getSpectrum(0, workspace)
        y = y / c_mbsr
        self.assertAlmostEqual(y[0], 12.474945990071641, 6)
        self.assertAlmostEqual(y[1], 4.3004130214544389, 6)
        self.assertAlmostEqual(y[2], 1.4523079303712476, 6)
        self.assertAlmostEqual(y[3], 0.6922657279528992, 6)
        self.assertAlmostEqual(y[4], 0.40107924259746491, 6)
        self.assertAlmostEqual(y[15], 0.050129858433581413, 6)
        self.assertAlmostEqual(y[16], 0.054427788297191478, 6)
        x, y = cf.getSpectrum(1, workspace)
        y = y / c_mbsr
        self.assertAlmostEqual(y[0], 6.3046623789675627, 6)
        self.assertAlmostEqual(y[1], 4.2753024205094912, 6)
        self.assertAlmostEqual(y[2], 2.1778204115683644, 6)
        self.assertAlmostEqual(y[3], 1.2011173460849718, 6)
        self.assertAlmostEqual(y[4], 0.74036730921135963, 6)
        x, y = cf.getSpectrum(workspace)
        y = y / c_mbsr
        self.assertAlmostEqual(y[0], 12.474945990071641, 6)
        self.assertAlmostEqual(y[1], 4.3004130214544389, 6)
        workspace = CreateWorkspace(x, y, e, 2)
        x, y = cf.getSpectrum(workspace, 1)
        y = y / c_mbsr
        self.assertAlmostEqual(y[0], 0.050129858433581413, 6)
        self.assertAlmostEqual(y[1], 0.054427788297191478, 6)

    def test_api_CrystalField_spectrum_peaks(self):
        from CrystalField import CrystalField, PeaksFunction
        cf = CrystalField('Ce', 'C2v', B20=0.035, B40=-0.012, B43=-0.027, B60=-0.00012, B63=0.0025, B66=0.0068,
                          Temperature=10.0, FWHM=0.1)
        cf.PeakShape = 'Gaussian'
        cf.peaks.param[1]['Sigma'] = 0.05
        cf.peaks.param[2]['Sigma'] = 0.1
        cf.peaks.param[3]['Sigma'] = 0.2
        cf.peaks.param[4]['Sigma'] = 0.3

        self.assertEqual(cf.peaks.param[1]['Sigma'], 0.05)
        self.assertEqual(cf.peaks.param[2]['Sigma'], 0.1)
        self.assertEqual(cf.peaks.param[3]['Sigma'], 0.2)
        self.assertEqual(cf.peaks.param[4]['Sigma'], 0.3)

        self.assertEqual(cf.function.getParameterValue('f1.Sigma'), 0.05)
        self.assertEqual(cf.function.getParameterValue('f2.Sigma'), 0.1)
        self.assertEqual(cf.function.getParameterValue('f3.Sigma'), 0.2)
        self.assertEqual(cf.function.getParameterValue('f4.Sigma'), 0.3)

        x, y = cf.getSpectrum()
        y = y / c_mbsr
        self.assertAlmostEqual(y[123], 0.067679792127989441, 8)
        self.assertAlmostEqual(y[124], 0.099056455104978708, 8)

    def test_api_CrystalField_spectrum_peaks_multi(self):
        from CrystalField import CrystalField, PeaksFunction
        cf = CrystalField('Ce', 'C2v', B20=0.035, B40=-0.012, B43=-0.027, B60=-0.00012, B63=0.0025, B66=0.0068,
                          Temperature=[10.0, 10.0], FWHM=[1.0, 1.0])
        cf.PeakShape = 'Gaussian'
        cf.peaks[0].param[1]['Sigma'] = 0.1
        cf.peaks[0].param[2]['Sigma'] = 0.2
        cf.peaks[0].param[3]['Sigma'] = 0.3

        self.assertEqual(cf.function.getParameterValue('f0.f2.Sigma'), 0.1)
        self.assertEqual(cf.function.getParameterValue('f0.f3.Sigma'), 0.2)
        self.assertEqual(cf.function.getParameterValue('f0.f4.Sigma'), 0.3)

        x0, y0 = cf.getSpectrum()
        x1, y1 = cf.getSpectrum(1)
        y0 = y0 / c_mbsr
        y1 = y1 / c_mbsr
        self.assertAlmostEqual(y0[139], 0.069849134145611211, 8)
        self.assertAlmostEqual(y0[142], 0.049105825374702927, 8)
        self.assertAlmostEqual(y1[139], 0.17385222868511149, 8)
        self.assertAlmostEqual(y1[142], 0.17671738547959939, 8)

    def test_api_CrystalField_spectrum_background(self):
        from CrystalField import CrystalField, PeaksFunction, Background, Function
        cf = CrystalField('Ce', 'C2v', B20=0.035, B40=-0.012, B43=-0.027, B60=-0.00012, B63=0.0025, B66=0.0068,
                          Temperature=10.0, FWHM=0.1)
        cf.PeakShape = 'Gaussian'
        cf.peaks.param[1]['Sigma'] = 0.1
        cf.peaks.param[2]['Sigma'] = 0.2
        cf.peaks.param[3]['Sigma'] = 0.3
        cf.background = Background(peak=Function('PseudoVoigt', Height=10*c_mbsr, FWHM=1, Mixing=0.5),
                                   background=Function('LinearBackground', A0=1.0*c_mbsr, A1=0.1*c_mbsr))
        self.assertEqual(cf.background.peak.param['Mixing'], 0.5)
        self.assertAlmostEqual(cf.background.background.param['A0'], 1.0*c_mbsr, 4)
        self.assertEqual(cf.peaks.param[1]['Sigma'], 0.1)
        self.assertEqual(cf.peaks.param[2]['Sigma'], 0.2)
        self.assertEqual(cf.peaks.param[3]['Sigma'], 0.3)
        self.assertEqual(cf.function.getParameterValue('f1.f1.Sigma'), 0.1)
        self.assertEqual(cf.function.getParameterValue('f1.f2.Sigma'), 0.2)
        self.assertEqual(cf.function.getParameterValue('f1.f3.Sigma'), 0.3)

        x, y = cf.getSpectrum()
        y = y / c_mbsr
        self.assertAlmostEqual(y[80], 2.5853144348907442, 8)
        self.assertAlmostEqual(y[90], 6.6726254910965057, 8)

    def test_api_CrystalField_spectrum_background_no_peak(self):
        from CrystalField import CrystalField, PeaksFunction, Background, Function
        cf = CrystalField('Ce', 'C2v', B20=0.035, B40=-0.012, B43=-0.027, B60=-0.00012, B63=0.0025, B66=0.0068,
                          Temperature=10.0, FWHM=0.1)
        cf.PeakShape = 'Gaussian'
        cf.peaks.param[1]['Sigma'] = 0.1
        cf.peaks.param[2]['Sigma'] = 0.2
        cf.peaks.param[3]['Sigma'] = 0.3
        cf.background = Background(background=Function('LinearBackground', A0=1.0*c_mbsr, A1=0.1*c_mbsr))
        self.assertAlmostEqual(cf.background.background.param['A0'], 1.0*c_mbsr, 4)
        self.assertAlmostEqual(cf.background.background.param['A1'], 0.1*c_mbsr, 4)
        self.assertEqual(cf.peaks.param[1]['Sigma'], 0.1)
        self.assertEqual(cf.peaks.param[2]['Sigma'], 0.2)
        self.assertEqual(cf.peaks.param[3]['Sigma'], 0.3)
        self.assertEqual(cf.function.getParameterValue('f1.f1.Sigma'), 0.1)
        self.assertEqual(cf.function.getParameterValue('f1.f2.Sigma'), 0.2)
        self.assertEqual(cf.function.getParameterValue('f1.f3.Sigma'), 0.3)

        x, y = cf.getSpectrum()
        y = y / c_mbsr
        self.assertAlmostEqual(y[80], 0.90929378650114456, 8)
        self.assertAlmostEqual(y[90], 0.95580997734199358, 8)

    def test_api_CrystalField_spectrum_background_no_background(self):
            from CrystalField import CrystalField, PeaksFunction, Background, Function
            cf = CrystalField('Ce', 'C2v', B20=0.035, B40=-0.012, B43=-0.027, B60=-0.00012, B63=0.0025, B66=0.0068,
                              Temperature=10.0, FWHM=0.1)
            cf.PeakShape = 'Gaussian'
            cf.peaks.param[1]['Sigma'] = 0.1
            cf.peaks.param[2]['Sigma'] = 0.2
            cf.peaks.param[3]['Sigma'] = 0.3
            cf.background = Background(peak=Function('PseudoVoigt', Height=10*c_mbsr, FWHM=1, Mixing=0.5))
            self.assertEqual(cf.background.peak.param['Mixing'], 0.5)
            self.assertEqual(cf.peaks.param[1]['Sigma'], 0.1)
            self.assertEqual(cf.peaks.param[2]['Sigma'], 0.2)
            self.assertEqual(cf.peaks.param[3]['Sigma'], 0.3)
            self.assertEqual(cf.function.getParameterValue('f1.f1.Sigma'), 0.1)
            self.assertEqual(cf.function.getParameterValue('f1.f2.Sigma'), 0.2)
            self.assertEqual(cf.function.getParameterValue('f1.f3.Sigma'), 0.3)

            x, y = cf.getSpectrum()
            y = y / c_mbsr
            self.assertAlmostEqual(y[80], 1.6760206483896094, 8)
            self.assertAlmostEqual(y[90], 5.7168155143063295, 8)

    def test_api_CrystalField_multi_spectrum_background(self):
        from CrystalField import CrystalField, PeaksFunction, Background, Function
        cf = CrystalField('Ce', 'C2v', B20=0.035, B40=-0.012, B43=-0.027, B60=-0.00012, B63=0.0025, B66=0.0068,
                          Temperature=[10.0, 10.0], FWHM=1.0)
        cf.PeakShape = 'Gaussian'
        cf.background = Background(peak=Function('Gaussian', Height=10*c_mbsr, Sigma=1),
                                   background=Function('FlatBackground', A0=1.0*c_mbsr))

        cf.peaks[0].param[1]['Sigma'] = 0.1
        cf.peaks[0].param[2]['Sigma'] = 0.2
        cf.peaks[0].param[3]['Sigma'] = 0.3
        cf.peaks[1].param[1]['Sigma'] = 1.1
        cf.peaks[1].param[2]['Sigma'] = 1.2
        cf.peaks[1].param[3]['Sigma'] = 1.3

        cf.background[0].peak.param['Sigma'] = 0.3
        cf.background[1].peak.param['Sigma'] = 0.4
        cf.background[1].background.param['A0'] = 2*c_mbsr

        self.assertEqual(cf.function.getParameterValue('f0.f0.f0.Sigma'), 0.3)
        self.assertEqual(cf.function.getParameterValue('f1.f0.f0.Sigma'), 0.4)
        self.assertEqual(cf.function.getParameterValue('f1.f0.f1.A0'), 2*c_mbsr)

        self.assertEqual(cf.background[0].peak.param['Sigma'], 0.3)
        self.assertEqual(cf.background[1].peak.param['Sigma'], 0.4)
        self.assertEqual(cf.background[1].background.param['A0'], 2 * c_mbsr)

        self.assertEqual(cf.function.getParameterValue('f0.f2.Sigma'), 0.1)
        self.assertEqual(cf.function.getParameterValue('f0.f3.Sigma'), 0.2)
        self.assertEqual(cf.function.getParameterValue('f0.f4.Sigma'), 0.3)
        self.assertEqual(cf.peaks[0].param[1]['Sigma'], 0.1)
        self.assertEqual(cf.peaks[0].param[2]['Sigma'], 0.2)
        self.assertEqual(cf.peaks[0].param[3]['Sigma'], 0.3)
        self.assertEqual(cf.peaks[1].param[1]['Sigma'], 1.1)
        self.assertEqual(cf.peaks[1].param[2]['Sigma'], 1.2)
        self.assertEqual(cf.peaks[1].param[3]['Sigma'], 1.3)

        x0, y0 = cf.getSpectrum()
        x1, y1 = cf.getSpectrum(1)
        # Original test was for FOCUS convention - intensity in barn.
        # Now use ISIS convention with intensity in milibarn/steradian
        y0 = y0 / c_mbsr
        y1 = y1 / c_mbsr
        self.assertAlmostEqual(y0[100], 13.005373133922404, 8)
        self.assertAlmostEqual(y0[120], 1.2693402982862221, 8)
        self.assertAlmostEqual(y0[139], 1.0698495632540335, 8)
        self.assertAlmostEqual(y0[150], 1.1702576101920288, 8)
        self.assertAlmostEqual(y1[100], 14.133257594622378, 8)
        self.assertAlmostEqual(y1[120], 3.0240871164367849, 8)
        self.assertAlmostEqual(y1[139], 2.5819042190621113, 8)
        self.assertAlmostEqual(y1[150], 2.8754340499592388, 8)

    def test_api_CrystalField_multi_spectrum_background_no_peak(self):
        from CrystalField import CrystalField, PeaksFunction, Background, Function
        cf = CrystalField('Ce', 'C2v', B20=0.035, B40=-0.012, B43=-0.027, B60=-0.00012, B63=0.0025, B66=0.0068,
                          Temperature=[10.0, 10.0], FWHM=1.0)
        cf.PeakShape = 'Gaussian'
        cf.background = Background(background=Function('FlatBackground', A0=1.0*c_mbsr))

        cf.peaks[0].param[1]['Sigma'] = 0.1
        cf.peaks[0].param[2]['Sigma'] = 0.2
        cf.peaks[0].param[3]['Sigma'] = 0.3
        cf.peaks[1].param[1]['Sigma'] = 1.1
        cf.peaks[1].param[2]['Sigma'] = 1.2
        cf.peaks[1].param[3]['Sigma'] = 1.3

        cf.background[0].background.param['A0'] = c_mbsr
        cf.background[1].background.param['A0'] = 2 * c_mbsr

        self.assertEqual(cf.function.getParameterValue('f0.f0.A0'), c_mbsr)
        self.assertEqual(cf.function.getParameterValue('f1.f0.A0'), 2 * c_mbsr)

        self.assertEqual(cf.background[0].background.param['A0'], c_mbsr)
        self.assertEqual(cf.background[1].background.param['A0'], 2 * c_mbsr)

        self.assertEqual(cf.function.getParameterValue('f0.f2.Sigma'), 0.1)
        self.assertEqual(cf.function.getParameterValue('f0.f3.Sigma'), 0.2)
        self.assertEqual(cf.function.getParameterValue('f0.f4.Sigma'), 0.3)
        self.assertEqual(cf.peaks[0].param[1]['Sigma'], 0.1)
        self.assertEqual(cf.peaks[0].param[2]['Sigma'], 0.2)
        self.assertEqual(cf.peaks[0].param[3]['Sigma'], 0.3)
        self.assertEqual(cf.peaks[1].param[1]['Sigma'], 1.1)
        self.assertEqual(cf.peaks[1].param[2]['Sigma'], 1.2)
        self.assertEqual(cf.peaks[1].param[3]['Sigma'], 1.3)

        x0, y0 = cf.getSpectrum()
        x1, y1 = cf.getSpectrum(1)
        # Original test was for FOCUS convention - intensity in barn.
        # Now use ISIS convention with intensity in milibarn/steradian
        y0 = y0 / c_mbsr
        y1 = y1 / c_mbsr
        self.assertAlmostEqual(y0[100], 3.0353766022416497, 8)
        self.assertAlmostEqual(y0[120], 1.2053599984285959, 8)
        self.assertAlmostEqual(y0[139], 1.0698494917103774, 8)
        self.assertAlmostEqual(y0[150], 1.1702576101915432, 8)
        self.assertAlmostEqual(y1[100], 4.150144076581511, 8)
        self.assertAlmostEqual(y1[120], 2.4407748685435036, 8)
        self.assertAlmostEqual(y1[139], 2.5816422823759626, 8)
        self.assertAlmostEqual(y1[150], 2.8754337256352809, 8)

    def test_api_CrystalField_single_multi_check(self):
        from CrystalField import CrystalField
        cf = CrystalField('Ce', 'C2v', B20=0.035, Temperature=[10.0, 10.0], FWHM=1.0)
        self.assertEqual(cf.FWHM[0], 1.0)
        self.assertEqual(cf.FWHM[1], 1.0)
        self.assertRaises(RuntimeError, CrystalField, 'Ce', 'C2v', B20=0.035, Temperature=[5, 10], FWHM=[0.5, 1, 2])
        cf = CrystalField('Ce', 'C2v', B20=0.035, Temperature=[5, 10], FWHM=[0.5, 1])

        def set_intensity_scaling(cf, value):
            cf.IntensityScaling = value
        self.assertRaises(ValueError, set_intensity_scaling, cf, [1, 2, 3, 4])
        cf = CrystalField('Ce', 'C2v', B20=0.035, B40=-0.012, B43=-0.027, B60=-0.00012, B63=0.0025, B66=0.0068,
                          Temperature=[4.0], FWHM=0.1, ToleranceIntensity=0.001*c_mbsr)
        cf.IntensityScaling = [1]
        x, y = cf.getSpectrum()
        y /= c_mbsr
        # self.assertAlmostEqual(y[60], 5.52333486, 8)

    def test_api_CrystalField_physical_properties(self):
        from CrystalField import CrystalField
        cf = CrystalField('Ce', 'C2v', B20=0.37737, B22=3.9770, B40=-0.031787, B42=-0.11611, B44=-0.12544)
        # Test Heat capacity calculations
        TCv, Cv = cf.getHeatCapacity()
        self.assertAlmostEqual(TCv[150], 151, 4)
        self.assertAlmostEqual(Cv[100], 4.2264, 3)
        self.assertAlmostEqual(Cv[150], 5.9218, 3)
        self.assertAlmostEqual(Cv[200], 5.4599, 3)

        # Test susceptibility calculations
        Tchi_powder, chi_powder = cf.getSusceptibility(np.linspace(1,300,50), Hdir='powder')
        self.assertAlmostEqual(Tchi_powder[10], 62.02, 2)
        self.assertAlmostEqual(chi_powder[5], 1.92026e-2, 6)
        self.assertAlmostEqual(chi_powder[10], 1.03471e-2, 6)
        self.assertAlmostEqual(chi_powder[15], 0.73004e-2, 6)

        # Test M(T) calculations
        Tmt_powder, mt_powder = cf.getMagneticMoment(1., Temperature=np.linspace(1,300,50), Hdir='powder', Unit='cgs')
        self.assertAlmostEqual(chi_powder[5], mt_powder[5], 6)
        self.assertAlmostEqual(chi_powder[10], mt_powder[10], 6)
        self.assertAlmostEqual(chi_powder[15], mt_powder[15], 6)
        _, invmt_powder_SI = cf.getMagneticMoment(1., Temperature=np.linspace(1,300,50), Hdir='powder', Unit='SI', Inverse=True)
        self.assertAlmostEqual(chi_powder[5] * 10, 1 / invmt_powder_SI[5], 2)
        self.assertAlmostEqual(chi_powder[10] * 10, 1 / invmt_powder_SI[10], 2)
        self.assertAlmostEqual(chi_powder[15] * 10, 1 / invmt_powder_SI[15], 2)

        # Test M(H) calculations
        Hmag_SI, mag_SI = cf.getMagneticMoment(np.linspace(0,30,15), Temperature=10, Hdir=[0,1,-1], Unit='SI')
        self.assertAlmostEqual(mag_SI[1], 1.8139, 3)
        self.assertAlmostEqual(mag_SI[5], 6.7859, 3)
        self.assertAlmostEqual(mag_SI[9], 8.2705, 3)
        _, mag_bohr = cf.getMagneticMoment(np.linspace(0,30,15), Temperature=10, Hdir=[0,1,-1], Unit='bohr')
        self.assertAlmostEqual(mag_SI[1] / 5.5849, mag_bohr[1], 3)
        self.assertAlmostEqual(mag_SI[5] / 5.5849, mag_bohr[5], 3)
        self.assertAlmostEqual(mag_SI[9] / 5.5849, mag_bohr[9], 3)

    def test_api_CrystalField_multi_spectrum_background_no_background(self):
        from CrystalField import CrystalField, PeaksFunction, Background, Function
        cf = CrystalField('Ce', 'C2v', B20=0.035, B40=-0.012, B43=-0.027, B60=-0.00012, B63=0.0025, B66=0.0068,
                          Temperature=[10.0, 10.0], FWHM=1.0)
        cf.PeakShape = 'Gaussian'
        cf.background = Background(peak=Function('Gaussian', Height=10*c_mbsr, Sigma=1))

        cf.peaks[0].param[1]['Sigma'] = 0.1
        cf.peaks[0].param[2]['Sigma'] = 0.2
        cf.peaks[0].param[3]['Sigma'] = 0.3
        cf.peaks[1].param[1]['Sigma'] = 1.1
        cf.peaks[1].param[2]['Sigma'] = 1.2
        cf.peaks[1].param[3]['Sigma'] = 1.3

        cf.background[0].peak.param['Sigma'] = 0.3
        cf.background[1].peak.param['Sigma'] = 0.4

        self.assertEqual(cf.function.getParameterValue('f0.f0.Sigma'), 0.3)
        self.assertEqual(cf.function.getParameterValue('f1.f0.Sigma'), 0.4)

        self.assertEqual(cf.background[0].peak.param['Sigma'], 0.3)
        self.assertEqual(cf.background[1].peak.param['Sigma'], 0.4)

        self.assertEqual(cf.function.getParameterValue('f0.f2.Sigma'), 0.1)
        self.assertEqual(cf.function.getParameterValue('f0.f3.Sigma'), 0.2)
        self.assertEqual(cf.function.getParameterValue('f0.f4.Sigma'), 0.3)
        self.assertEqual(cf.peaks[0].param[1]['Sigma'], 0.1)
        self.assertEqual(cf.peaks[0].param[2]['Sigma'], 0.2)
        self.assertEqual(cf.peaks[0].param[3]['Sigma'], 0.3)
        self.assertEqual(cf.peaks[1].param[1]['Sigma'], 1.1)
        self.assertEqual(cf.peaks[1].param[2]['Sigma'], 1.2)
        self.assertEqual(cf.peaks[1].param[3]['Sigma'], 1.3)

        x0, y0 = cf.getSpectrum()
        x1, y1 = cf.getSpectrum(1)
        # Original test was for FOCUS convention - intensity in barn.
        # Now use ISIS convention with intensity in milibarn/steradian
        y0 = y0 / c_mbsr
        y1 = y1 / c_mbsr
        self.assertAlmostEqual(y0[100], 12.005372776357635, 8)
        self.assertAlmostEqual(y0[120], 0.26933994072145595, 8)
        self.assertAlmostEqual(y0[139], 0.069849205689267363, 8)
        self.assertAlmostEqual(y0[150], 0.17025725262726249, 8)
        self.assertAlmostEqual(y1[100], 12.133256879492841, 8)
        self.assertAlmostEqual(y1[120], 1.0240864013072524, 8)
        self.assertAlmostEqual(y1[139], 0.58190350393257906, 8)
        self.assertAlmostEqual(y1[150], 0.87543333482970631, 8)


class CrystalFieldFitTest(unittest.TestCase):

    def test_CrystalFieldFit(self):
        from CrystalField.fitting import makeWorkspace
        from CrystalField import CrystalField, CrystalFieldFit, Background, Function
        origin = CrystalField('Ce', 'C2v', B20=0.37737, B22=3.9770, B40=-0.031787, B42=-0.11611, B44=-0.12544,
                  Temperature=44.0, FWHM=1.1)
        origin.background = Background(peak=Function('Gaussian', Height=10*c_mbsr, Sigma=1),
                            background=Function('LinearBackground', A0=1.0, A1=0.01))
        x, y = origin.getSpectrum()
        ws = makeWorkspace(x, y)

        cf = CrystalField('Ce', 'C2v', B20=0.37, B22=3.97, B40=-0.0317, B42=-0.116, B44=-0.12,
                      Temperature=44.0, FWHM=1.0)
        cf.background = Background(peak=Function('Gaussian', Height=10*c_mbsr, Sigma=1),
                        background=Function('LinearBackground', A0=1.0, A1=0.01))
        cf.ties(B20=0.37737, IntensityScaling=1)
        fit = CrystalFieldFit(cf, InputWorkspace=ws)
        fit.fit()
        self.assertAlmostEqual(cf.background.peak.param['PeakCentre'], 7.62501442212e-10, 8)
        self.assertAlmostEqual(cf.background.peak.param['Sigma'], 1.00000000277, 8)
        self.assertAlmostEqual(cf.background.peak.param['Height'], 9.99999983559*c_mbsr, 3)
        self.assertAlmostEqual(cf.background.background.param['A1'], 0.0100000014282, 4)
        self.assertAlmostEqual(cf.background.background.param['A0'], 0.999999976941, 4)
        self.assertEqual(cf['IB63'], 0.0)
        self.assertEqual(cf['IB62'], 0.0)
        self.assertEqual(cf['IB61'], 0.0)
        self.assertEqual(cf['IB66'], 0.0)
        self.assertEqual(cf['IB65'], 0.0)
        self.assertEqual(cf['IB64'], 0.0)
        self.assertEqual(cf['IB41'], 0.0)
        self.assertEqual(cf['IB43'], 0.0)
        self.assertEqual(cf['IB42'], 0.0)
        self.assertEqual(cf['IB44'], 0.0)
        self.assertEqual(cf['B21'], 0.0)
        self.assertEqual(cf['IB22'], 0.0)
        self.assertEqual(cf['IB21'], 0.0)
        self.assertEqual(cf['BextX'], 0.0)
        self.assertEqual(cf['BextY'], 0.0)
        self.assertEqual(cf['BextZ'], 0.0)
        self.assertEqual(cf['BmolY'], 0.0)
        self.assertEqual(cf['BmolX'], 0.0)
        self.assertEqual(cf['BmolZ'], 0.0)
        self.assertEqual(cf['B66'], 0.0)
        self.assertEqual(cf['B63'], 0.0)
        self.assertEqual(cf['B65'], 0.0)
        self.assertEqual(cf['B62'], 0.0)
        self.assertEqual(cf['B61'], 0.0)
        self.assertEqual(cf['B60'], 0.0)
        self.assertEqual(cf['B41'], 0.0)
        self.assertEqual(cf['B43'], 0.0)
        self.assertEqual(cf['B64'], 0.0)
        self.assertAlmostEqual(cf['B20'], 0.37737, 4)
        self.assertAlmostEqual(cf['B22'], 3.97700087765, 4)
        self.assertAlmostEqual(cf['B40'], -0.0317867635188, 4)
        self.assertAlmostEqual(cf['B42'], -0.116110640723, 4)
        self.assertAlmostEqual(cf['B44'], -0.125439939584, 4)
        self.assertAlmostEqual(cf.peaks.param[0]['PeakCentre'], 0.0, 8)
        self.assertAlmostEqual(cf.peaks.param[0]['FWHM'], 1.10000009456, 4)
        self.assertAlmostEqual(cf.peaks.param[0]['Amplitude'], 2.74936658109*c_mbsr, 2)
        self.assertAlmostEqual(cf.peaks.param[1]['PeakCentre'], 29.3261118837, 4)
        self.assertAlmostEqual(cf.peaks.param[1]['FWHM'], 1.10000293773, 4)
        self.assertAlmostEqual(cf.peaks.param[1]['Amplitude'], 0.720400223007*c_mbsr, 2)
        self.assertAlmostEqual(cf.peaks.param[2]['PeakCentre'], 44.341248146, 4)
        self.assertAlmostEqual(cf.peaks.param[2]['FWHM'], 1.10000812804, 4)
        self.assertAlmostEqual(cf.peaks.param[2]['Amplitude'], 0.429808829601*c_mbsr, 2)

    def test_CrystalFieldFit_multi_spectrum(self):
        from CrystalField.fitting import makeWorkspace
        from CrystalField import CrystalField, CrystalFieldFit, Background, Function
        origin = CrystalField('Ce', 'C2v', B20=0.37737, B22=3.9770, B40=-0.031787, B42=-0.11611, B44=-0.12544,
                              Temperature=[44.0, 50], FWHM=[1.1, 0.9])
        origin.PeakShape = 'Lorentzian'
        origin.peaks[0].param[1]['FWHM'] = 1.22
        origin.background = Background(peak=Function('Gaussian', Height=10, Sigma=0.3),
                                       background=Function('FlatBackground', A0=1.0))
        origin.background[1].peak.param['Sigma'] = 0.8
        origin.background[1].background.param['A0'] = 1.1

        origin.peaks[0].param[0]['FWHM'] = 1.11
        origin.peaks[1].param[1]['FWHM'] = 1.12

        fun = origin.function

        self.assertEqual(fun.getParameterValue('f0.f0.f0.Sigma'), 0.3)
        self.assertEqual(fun.getParameterValue('f0.f0.f1.A0'), 1.0)
        self.assertEqual(fun.getParameterValue('f1.f0.f0.Sigma'), 0.8)
        self.assertEqual(fun.getParameterValue('f1.f0.f1.A0'), 1.1)

        self.assertEqual(fun.getParameterValue('f0.f1.FWHM'), 1.11)
        self.assertEqual(fun.getParameterValue('f0.f2.FWHM'), 1.1)
        self.assertEqual(fun.getParameterValue('f0.f3.FWHM'), 1.1)

        self.assertEqual(fun.getParameterValue('f1.f1.FWHM'), 0.9)
        self.assertEqual(fun.getParameterValue('f1.f2.FWHM'), 1.12)
        self.assertEqual(fun.getParameterValue('f1.f3.FWHM'), 0.9)

        cf = CrystalField('Ce', 'C2v', B20=0.37737, B22=3.9770, B40=-0.031787, B42=-0.11611, B44=-0.12544,
                          Temperature=[44.0, 50], FWHM=[1.1, 0.9])
        cf.PeakShape = 'Lorentzian'
        cf.peaks[0].param[0]['FWHM'] = 1.11
        cf.peaks[1].param[1]['FWHM'] = 1.12
        cf.background = Background(peak=Function('Gaussian', Height=10, Sigma=0.3),
                                   background=Function('FlatBackground', A0=1.0))
        cf.ties(IntensityScaling0=1.0, IntensityScaling1=1.0)

        ws0 = makeWorkspace(*origin.getSpectrum(0))
        ws1 = makeWorkspace(*origin.getSpectrum(1))

        chi2 = CalculateChiSquared(cf.makeMultiSpectrumFunction(), InputWorkspace=ws0, InputWorkspace_1=ws1)[1]

        fit = CrystalFieldFit(cf, InputWorkspace=[ws0, ws1], MaxIterations=10)
        fit.fit()

        self.assertTrue(cf.chi2 < chi2)

        # Fit outputs are different on different platforms.
        # The following assertions are not for testing but to illustrate
        # how to get the parameters.
        self.assertNotEqual(cf.background[0].peak.param['PeakCentre'], 0.0)
        self.assertNotEqual(cf.background[0].peak.param['Sigma'], 0.0)
        self.assertNotEqual(cf.background[0].peak.param['Height'], 0.0)
        self.assertNotEqual(cf.background[0].background.param['A0'], 0.0)

        self.assertNotEqual(cf.background[1].peak.param['PeakCentre'], 0.0)
        self.assertNotEqual(cf.background[1].peak.param['Sigma'], 0.0)
        self.assertNotEqual(cf.background[1].peak.param['Height'], 0.0)
        self.assertNotEqual(cf.background[1].background.param['A0'], 0.0)

        self.assertNotEqual(cf.peaks[0].param[1]['PeakCentre'], 0.0)
        self.assertNotEqual(cf.peaks[0].param[1]['FWHM'], 0.0)
        self.assertNotEqual(cf.peaks[0].param[1]['Amplitude'], 0.0)

        self.assertNotEqual(cf.peaks[0].param[2]['PeakCentre'], 0.0)
        self.assertNotEqual(cf.peaks[0].param[2]['FWHM'], 0.0)
        self.assertNotEqual(cf.peaks[0].param[2]['Amplitude'], 0.0)

        self.assertEqual(cf.peaks[0].param[3]['PeakCentre'], 0.0)
        self.assertNotEqual(cf.peaks[0].param[3]['FWHM'], 0.0)
        self.assertEqual(cf.peaks[0].param[3]['Amplitude'], 0.0)

        self.assertNotEqual(cf.peaks[1].param[1]['PeakCentre'], 0.0)
        self.assertNotEqual(cf.peaks[1].param[1]['FWHM'], 0.0)
        self.assertNotEqual(cf.peaks[1].param[1]['Amplitude'], 0.0)

        self.assertNotEqual(cf.peaks[1].param[2]['PeakCentre'], 0.0)
        self.assertNotEqual(cf.peaks[1].param[2]['FWHM'], 0.0)
        self.assertNotEqual(cf.peaks[1].param[2]['Amplitude'], 0.0)

        self.assertEqual(cf.peaks[1].param[3]['PeakCentre'], 0.0)
        self.assertNotEqual(cf.peaks[1].param[3]['FWHM'], 0.0)
        self.assertEqual(cf.peaks[1].param[3]['Amplitude'], 0.0)

    def test_CrystalFieldFit_multi_spectrum_simple_background(self):
        from CrystalField.fitting import makeWorkspace
        from CrystalField import CrystalField, CrystalFieldFit, Background, Function
        origin = CrystalField('Ce', 'C2v', B20=0.37737, B22=3.9770, B40=-0.031787, B42=-0.11611, B44=-0.12544,
                              Temperature=[44.0, 50], FWHM=[1.1, 0.9])
        origin.PeakShape = 'Lorentzian'
        origin.background = Background(background=Function('FlatBackground', A0=1.0))
        origin.background[1].background.param['A0'] = 1.2
        origin.peaks[0].param[0]['FWHM'] = 1.11
        origin.peaks[1].param[1]['FWHM'] = 1.12

        fun = origin.function

        self.assertEqual(fun.getParameterValue('f1.f0.A0'), 1.2)

        self.assertEqual(fun.getParameterValue('f0.f1.FWHM'), 1.11)
        self.assertEqual(fun.getParameterValue('f0.f2.FWHM'), 1.1)
        self.assertEqual(fun.getParameterValue('f0.f3.FWHM'), 1.1)

        self.assertEqual(fun.getParameterValue('f1.f1.FWHM'), 0.9)
        self.assertEqual(fun.getParameterValue('f1.f2.FWHM'), 1.12)
        self.assertEqual(fun.getParameterValue('f1.f3.FWHM'), 0.9)

        cf = CrystalField('Ce', 'C2v', B20=0.37737, B22=3.9770, B40=-0.031787, B42=-0.11611, B44=-0.12544,
                          Temperature=[44.0, 50], FWHM=[1.1, 0.9])
        cf.PeakShape = 'Lorentzian'
        cf.background = Background(background=Function('FlatBackground', A0=0.9))
        cf.peaks[0].param[0]['FWHM'] = 1.11
        cf.peaks[1].param[1]['FWHM'] = 1.12
        cf.ties(IntensityScaling0=1.0, IntensityScaling1=1.0)

        ws0 = makeWorkspace(*origin.getSpectrum(0))
        ws1 = makeWorkspace(*origin.getSpectrum(1))

        fit = CrystalFieldFit(cf, InputWorkspace=[ws0, ws1])
        fit.fit()

        self.assertAlmostEqual(cf.background[0].background.param['A0'], 1.0, 4)
        self.assertAlmostEqual(cf.background[1].background.param['A0'], 1.2, 4)

    def test_CrystalFieldFit_multi_spectrum_peak_background(self):
        from CrystalField.fitting import makeWorkspace
        from CrystalField import CrystalField, CrystalFieldFit, Background, Function
        origin = CrystalField('Ce', 'C2v', B20=0.37737, B22=3.9770, B40=-0.031787, B42=-0.11611, B44=-0.12544,
                              Temperature=[44.0, 50], FWHM=[1.1, 0.9])
        origin.PeakShape = 'Lorentzian'
        origin.background = Background(peak=Function('Gaussian', Height=10, Sigma=0.3))
        origin.background[1].peak.param['Sigma'] = 0.8
        origin.peaks[0].param[0]['FWHM'] = 1.11
        origin.peaks[1].param[1]['FWHM'] = 1.12

        fun = origin.function

        self.assertEqual(fun.getParameterValue('f0.f0.Sigma'), 0.3)
        self.assertEqual(fun.getParameterValue('f1.f0.Sigma'), 0.8)

        self.assertEqual(fun.getParameterValue('f0.f1.FWHM'), 1.11)
        self.assertEqual(fun.getParameterValue('f0.f2.FWHM'), 1.1)
        self.assertEqual(fun.getParameterValue('f0.f3.FWHM'), 1.1)

        self.assertEqual(fun.getParameterValue('f1.f1.FWHM'), 0.9)
        self.assertEqual(fun.getParameterValue('f1.f2.FWHM'), 1.12)
        self.assertEqual(fun.getParameterValue('f1.f3.FWHM'), 0.9)

        cf = CrystalField('Ce', 'C2v', B20=0.37737, B22=3.9770, B40=-0.031787, B42=-0.11611, B44=-0.12544,
                          Temperature=[44.0, 50], FWHM=[1.1, 0.9])
        cf.PeakShape = 'Lorentzian'
        cf.background = Background(peak=Function('Gaussian', Height=10, Sigma=0.3))
        cf.peaks[0].param[0]['FWHM'] = 1.11
        cf.peaks[1].param[1]['FWHM'] = 1.12
        cf.ties(IntensityScaling0=1.0, IntensityScaling1=1.0)

        ws0 = makeWorkspace(*origin.getSpectrum(0))
        ws1 = makeWorkspace(*origin.getSpectrum(1))

        chi2 = CalculateChiSquared(cf.makeMultiSpectrumFunction(), InputWorkspace=ws0, InputWorkspace_1=ws1)[1]

        fit = CrystalFieldFit(cf, InputWorkspace=[ws0, ws1])
        fit.fit()

        self.assertTrue(cf.chi2 < chi2)

        # Fit outputs are different on different platforms.
        # The following assertions are not for testing but to illustrate
        # how to get the parameters.
        self.assertNotEqual(cf.background[0].peak.param['PeakCentre'], 0.0)
        self.assertNotEqual(cf.background[0].peak.param['Sigma'], 0.0)
        self.assertNotEqual(cf.background[0].peak.param['Height'], 0.0)

        self.assertNotEqual(cf.background[1].peak.param['PeakCentre'], 0.0)
        self.assertNotEqual(cf.background[1].peak.param['Sigma'], 0.0)
        self.assertNotEqual(cf.background[1].peak.param['Height'], 0.0)

    def test_fit_single_multi_check(self):
        from CrystalField.fitting import makeWorkspace
        from CrystalField import CrystalField, CrystalFieldFit
        cf = CrystalField('Nd', 'C2v', B20=-0.4, Temperature=[5, 10], FWHM=[0.5, 1])
        x, y = cf.getSpectrum()
        ws = makeWorkspace(x, y)
        cf.IntensityScaling = [1, 2]
        fit = CrystalFieldFit(Model=cf, InputWorkspace=[ws], MaxIterations=200)
        self.assertRaises(ValueError, fit.fit)
        cf = CrystalField('Nd', 'C2v', B20=-0.4, Temperature=[5], FWHM=0.5, IntensityScaling=[1])
        fit = CrystalFieldFit(Model=cf, InputWorkspace=ws, MaxIterations=1)
        self.assertEquals(fit.check_consistency(), None)
        cf = CrystalField('Nd', 'C2v', B20=-0.4, Temperature=[5], FWHM=0.5, IntensityScaling = 1)
        fit = CrystalFieldFit(Model=cf, InputWorkspace=[ws], MaxIterations=1)
        self.assertEquals(fit.check_consistency(), None)

    def test_constraints_single_spectrum(self):
        from CrystalField import CrystalField, CrystalFieldFit, Background, Function
        from mantid.simpleapi import FunctionFactory

        cf = CrystalField('Ce', 'C2v', B20=0.37737, B22=3.9770, B40=-0.031787, B42=-0.11611, B44=-0.12544,
                          Temperature=50, FWHM=0.9)
        cf.PeakShape = 'Lorentzian'
        cf.background = Background(peak=Function('Gaussian', Height=10.0, Sigma=0.3),
                                   background=Function('LinearBackground', A0=1.0))

        cf.ties(B40='B20/2')
        cf.constraints('IntensityScaling > 0', 'B22 < 4')
        cf.peaks.constraints('f0.FWHM < 2.2', 'f1.FWHM >= 0.1')
        cf.peaks.ties({'f2.FWHM': '2*f1.FWHM', 'f3.FWHM': '2*f2.FWHM'})
        cf.background.peak.ties(Height=10.1)
        cf.background.peak.constraints('Sigma > 0')
        cf.background.background.ties(A0=0.1)
        cf.background.background.constraints('A1 > 0')

        s = cf.makeSpectrumFunction()
        self.assertTrue('0<IntensityScaling' in s)
        self.assertTrue('B22<4' in s)
        self.assertTrue('0<f0.f0.Sigma' in s)
        self.assertTrue('0<f0.f1.A1' in s)
        self.assertTrue('Height=10.1' in s)
        self.assertTrue('A0=0.1' in s)
        self.assertTrue('f0.FWHM<2.2' in s)
        self.assertTrue('0.1<f1.FWHM' in s)
        self.assertTrue('f2.FWHM=2*f1.FWHM' in s)
        self.assertTrue('f3.FWHM=2*f2.FWHM' in s)

        # Test that ties and constraints are correctly defined
        fun = FunctionFactory.createInitialized(s)
        self.assertTrue(fun is not None)

    def test_all_peak_ties_single_spectrum(self):
        from CrystalField import CrystalField, CrystalFieldFit, Background, Function
        from mantid.simpleapi import FunctionFactory

        cf = CrystalField('Ce', 'C2v', B20=0.37737, B22=3.9770, B40=-0.031787, B42=-0.11611, B44=-0.12544,
                          Temperature=50, FWHM=0.9)
        cf.peaks.tieAll(' FWHM=2.1', 3)

        s = cf.makeSpectrumFunction()
        self.assertTrue('f0.FWHM=2.1' in s)
        self.assertTrue('f1.FWHM=2.1' in s)
        self.assertTrue('f2.FWHM=2.1' in s)
        self.assertTrue('f3.FWHM=2.1' not in s)

        # Test that ties and constraints are correctly defined
        fun = FunctionFactory.createInitialized(s)
        self.assertTrue(fun is not None)

    def test_all_peak_ties_single_spectrum_range(self):
        from CrystalField import CrystalField, CrystalFieldFit, Background, Function
        from mantid.simpleapi import FunctionFactory

        cf = CrystalField('Ce', 'C2v', B20=0.37737, B22=3.9770, B40=-0.031787, B42=-0.11611, B44=-0.12544,
                          Temperature=50, FWHM=0.9)
        cf.peaks.tieAll('FWHM=f0.FWHM', 1, 4)

        s = cf.makeSpectrumFunction()
        self.assertTrue('f0.FWHM=f0.FWHM' not in s)
        self.assertTrue('f1.FWHM=f0.FWHM' in s)
        self.assertTrue('f2.FWHM=f0.FWHM' in s)
        self.assertTrue('f3.FWHM=f0.FWHM' in s)
        self.assertTrue('f4.FWHM=f0.FWHM' in s)
        self.assertTrue('f5.FWHM=f0.FWHM' not in s)

        # Test that ties and constraints are correctly defined
        fun = FunctionFactory.createInitialized(s)
        self.assertTrue(fun is not None)

    def test_all_peak_constraints_single_spectrum(self):
        from CrystalField import CrystalField, CrystalFieldFit, Background, Function
        from mantid.simpleapi import FunctionFactory

        cf = CrystalField('Ce', 'C2v', B20=0.37737, B22=3.9770, B40=-0.031787, B42=-0.11611, B44=-0.12544,
                          Temperature=50, FWHM=0.9)
        cf.peaks.constrainAll('0.1 < FWHM <=2.1', 3)

        s = cf.makeSpectrumFunction()
        self.assertTrue('0.1<f0.FWHM<2.1' in s)
        self.assertTrue('0.1<f1.FWHM<2.1' in s)
        self.assertTrue('0.1<f2.FWHM<2.1' in s)
        self.assertTrue('0.1<f3.FWHM<2.1' not in s)

        # Test that ties and constraints are correctly defined
        fun = FunctionFactory.createInitialized(s)
        self.assertTrue(fun is not None)

    def test_all_peak_constraints_single_spectrum_range(self):
        from CrystalField import CrystalField, CrystalFieldFit, Background, Function
        from mantid.simpleapi import FunctionFactory

        cf = CrystalField('Ce', 'C2v', B20=0.37737, B22=3.9770, B40=-0.031787, B42=-0.11611, B44=-0.12544,
                          Temperature=50, FWHM=0.9)
        cf.peaks.constrainAll('0.1 < FWHM <=2.1', 1, 2)

        s = cf.makeSpectrumFunction()
        self.assertTrue('0.1<f0.FWHM<2.1' not in s)
        self.assertTrue('0.1<f1.FWHM<2.1' in s)
        self.assertTrue('0.1<f2.FWHM<2.1' in s)
        self.assertTrue('0.1<f3.FWHM<2.1' not in s)

        # Test that ties and constraints are correctly defined
        fun = FunctionFactory.createInitialized(s)
        self.assertTrue(fun is not None)

    def test_constraints_multi_spectrum(self):
        from CrystalField import CrystalField, CrystalFieldFit, Background, Function
        from mantid.simpleapi import FunctionFactory

        cf = CrystalField('Ce', 'C2v', B20=0.37737, B22=3.9770, B40=-0.031787, B42=-0.11611, B44=-0.12544,
                          Temperature=[44.0, 50], FWHM=[1.1, 0.9])
        cf.PeakShape = 'Lorentzian'
        cf.background = Background(peak=Function('Gaussian', Height=10, Sigma=0.3),
                         background=Function('FlatBackground', A0=1.0))
        cf.constraints('IntensityScaling0 > 0', '0 < IntensityScaling1 < 2', 'B22 < 4')
        cf.background[0].peak.ties(Height=10.1)
        cf.background[0].peak.constraints('Sigma > 0.1')
        cf.background[1].peak.ties(Height=20.2)
        cf.background[1].peak.constraints('Sigma > 0.2')

        cf.peaks[1].ties({'f2.FWHM': '2*f1.FWHM', 'f3.FWHM': '2*f2.FWHM'})
        cf.peaks[0].constraints('f1.FWHM < 2.2')
        cf.peaks[1].constraints('f1.FWHM > 1.1', '1 < f4.FWHM < 2.2')

        s = cf.makeMultiSpectrumFunction()

        self.assertTrue('0<IntensityScaling0' in s)
        self.assertTrue('IntensityScaling1<2' in s)
        self.assertTrue('f0.f0.f0.Height=10.1' in s)
        self.assertTrue('f1.f0.f0.Height=20.2' in s)
        self.assertTrue('0.1<f0.f0.f0.Sigma' in s)
        self.assertTrue('0.2<f1.f0.f0.Sigma' in s)
        self.assertTrue('f0.f1.FWHM<2.2' in s)
        self.assertTrue('1.1<f1.f1.FWHM' in s)
        self.assertTrue('1<f1.f4.FWHM<2.2' in s)
        self.assertTrue('f1.f2.FWHM=2*f1.f1.FWHM' in s)
        self.assertTrue('f1.f3.FWHM=2*f1.f2.FWHM' in s)

        fun = FunctionFactory.createInitialized(s)

    def test_all_peak_ties_multi_spectrum(self):
        from CrystalField import CrystalField, CrystalFieldFit, Background, Function
        from mantid.simpleapi import FunctionFactory

        cf = CrystalField('Ce', 'C2v', B20=0.37737, B22=3.9770, B40=-0.031787, B42=-0.11611, B44=-0.12544,
                          Temperature=[50, 100], FWHM=[0.9, 0.1])
        cf.peaks[0].tieAll('FWHM=f1.FWHM', 2, 4)
        cf.peaks[1].tieAll('FWHM=3.14', 4)

        s = cf.makeMultiSpectrumFunction()
        self.assertTrue('f0.f1.FWHM=f0.f1.FWHM' not in s)
        self.assertTrue('f0.f2.FWHM=f0.f1.FWHM' in s)
        self.assertTrue('f0.f3.FWHM=f0.f1.FWHM' in s)
        self.assertTrue('f0.f4.FWHM=f0.f1.FWHM' in s)
        self.assertTrue('f0.f5.FWHM=f0.f1.FWHM' not in s)

        self.assertTrue('f1.f0.FWHM=3.14' not in s)
        self.assertTrue('f1.f1.FWHM=3.14' in s)
        self.assertTrue('f1.f2.FWHM=3.14' in s)
        self.assertTrue('f1.f3.FWHM=3.14' in s)
        self.assertTrue('f1.f4.FWHM=3.14' in s)
        self.assertTrue('f1.f5.FWHM=3.14' not in s)

        # Test that ties and constraints are correctly defined
        fun = FunctionFactory.createInitialized(s)
        self.assertTrue(fun is not None)

    def test_all_peak_constraints_multi_spectrum_range(self):
        from CrystalField import CrystalField, CrystalFieldFit, Background, Function
        from mantid.simpleapi import FunctionFactory

        cf = CrystalField('Ce', 'C2v', B20=0.37737, B22=3.9770, B40=-0.031787, B42=-0.11611, B44=-0.12544,
                          Temperature=[50, 20], FWHM=[0.9, 10])
        cf.peaks[0].constrainAll('0.1 < FWHM <=2.1', 2)
        cf.peaks[1].constrainAll('FWHM > 12.1', 3, 5)

        s = cf.makeMultiSpectrumFunction()
        self.assertTrue('0.1<f0.f0.FWHM<2.1' not in s)
        self.assertTrue('0.1<f0.f1.FWHM<2.1' in s)
        self.assertTrue('0.1<f0.f2.FWHM<2.1' in s)
        self.assertTrue('0.1<f0.f4.FWHM<2.1' not in s)

        self.assertTrue('12.1<f1.f2.FWHM' not in s)
        self.assertTrue('12.1<f1.f3.FWHM' in s)
        self.assertTrue('12.1<f1.f4.FWHM' in s)
        self.assertTrue('12.1<f1.f5.FWHM' in s)
        self.assertTrue('12.1<f1.f6.FWHM' not in s)

        # Test that ties and constraints are correctly defined
        fun = FunctionFactory.createInitialized(s)
        self.assertTrue(fun is not None)

    def test_bad_input(self):
        from CrystalField import CrystalField

        self.assertRaises(Exception, CrystalField, 'Ce', 'C2v', B20='aaa', B22=3.97, B40=-0.0317, B42=-0.116, B44=-0.12,
                          Temperature=44.0, FWHM=1.0)

        self.assertRaises(Exception, CrystalField, 'Ce', 'C2v', B20=1, B22=3.97, B40=[-0.0317], B42=-0.116, B44=-0.12,
                          Temperature=44.0, FWHM=1.0)

        self.assertRaises(Exception, CrystalField, 'Ce', 'C2v', B20=1, B22=3.97, B40=np.array([-0.0317]), B42=-0.116,
                          B44=-0.12, Temperature=44.0, FWHM=1.0)

        self.assertRaises(Exception, CrystalField, 'Ce', 'C2v', B20=1, B22=3.97, B40=np.array([1.2, 2.3]), B42=-0.116,
                          B44=-0.12, Temperature=44.0, FWHM=1.0)

        cf = CrystalField('Ce', 'C2v', B20=1, B22=3.97, B40=-0.0317, B42=-0.116, B44=-0.12,
                          Temperature=44.0, FWHM=1.0)

        def set_peak_parameter():
            cf.peaks.param[1]["FWHM"] = 'aaa'
        self.assertRaises(Exception, set_peak_parameter)

    def test_resolution_single_spectrum(self):
        from CrystalField import CrystalField
        cf = CrystalField('Ce', 'C2v', B20=0.37, B22=3.97, B40=-0.0317, B42=-0.116, B44=-0.12,
                      Temperature=44.0, FWHM=1.0, ResolutionModel=([0, 50], [1, 2]))
        self.assertAlmostEqual(cf.peaks.param[0]['FWHM'], 1.0, 8)
        self.assertAlmostEqual(cf.peaks.param[1]['FWHM'], 1.581014682, 8)
        self.assertAlmostEqual(cf.peaks.param[2]['FWHM'], 1.884945866, 8)

    def test_resolution_single_spectrum_fit(self):
        from CrystalField import CrystalField, CrystalFieldFit
        from CrystalField.fitting import makeWorkspace
        origin = CrystalField('Ce', 'C2v', B20=0.37, B22=3.97, B40=-0.0317, B42=-0.116, B44=-0.12,
                      Temperature=44.0, FWHM=1.0)
        origin.peaks.param[0]['FWHM'] = 1.01
        origin.peaks.param[1]['FWHM'] = 1.4
        origin.peaks.param[2]['FWHM'] = 1.8

        x, y = origin.getSpectrum()
        ws = makeWorkspace(x, y)

        cf = CrystalField('Ce', 'C2v', B20=0.37, B22=3.97, B40=-0.0317, B42=-0.116, B44=-0.12,
                          Temperature=44.0, FWHM=1.0, ResolutionModel=([0, 50], [1, 2]))

        fit = CrystalFieldFit(Model=cf, InputWorkspace=ws)
        fit.fit()

        self.assertAlmostEqual(cf.peaks.param[0]['FWHM'], 1.0112, 4)
        self.assertAlmostEqual(cf.peaks.param[1]['FWHM'], 1.58106, 4)
        self.assertAlmostEqual(cf.peaks.param[2]['FWHM'], 1.7947, 2)

    def test_resolution_single_spectrum_fit_variation(self):
        from CrystalField import CrystalField, CrystalFieldFit
        from CrystalField.fitting import makeWorkspace
        origin = CrystalField('Ce', 'C2v', B20=0.37, B22=3.97, B40=-0.0317, B42=-0.116, B44=-0.12,
                      Temperature=44.0, FWHM=1.0)
        origin.peaks.param[0]['FWHM'] = 1.01
        origin.peaks.param[1]['FWHM'] = 1.4
        origin.peaks.param[2]['FWHM'] = 1.8

        x, y = origin.getSpectrum()
        ws = makeWorkspace(x, y)

        cf = CrystalField('Ce', 'C2v', B20=0.37, B22=3.97, B40=-0.0317, B42=-0.116, B44=-0.12,
                          Temperature=44.0, FWHM=1.0, ResolutionModel=([0, 50], [1, 2]), FWHMVariation=0.3)

        fit = CrystalFieldFit(Model=cf, InputWorkspace=ws)
        fit.fit()

        self.assertAlmostEqual(cf.peaks.param[0]['FWHM'], 1.01, 4)
        self.assertAlmostEqual(cf.peaks.param[1]['FWHM'], 1.4, 4)
        self.assertAlmostEqual(cf.peaks.param[2]['FWHM'], 1.8, 4)

    def test_ResolutionModel_func_single(self):
        from CrystalField import ResolutionModel

        def func(x):
            return np.sin(x)
        rm = ResolutionModel(func, 0, np.pi)
        self.assertEquals(len(rm.model[0]), 129)
        self.assertEquals(len(rm.model[1]), 129)
        self.assertTrue(np.all(func(rm.model[0]) == rm.model[1]))

    def test_ResolutionModel_func_multi(self):
        from CrystalField import ResolutionModel
        def func0(x):
            return np.sin(np.array(x))

        class CalcWidth:
            def __call__(self, x):
                return np.cos(np.array(x) / 2)

            def model(self, x):
                return np.tan(np.array(x) / 2)
        func1 = CalcWidth()
        func2 = func1.model
        rm = ResolutionModel([func0, func1, func2], -np.pi/2, np.pi/2, accuracy = 0.01)

        self.assertEquals(len(rm.model), 3)
        self.assertEquals(len(rm.model[0][0]), 17)
        self.assertEquals(len(rm.model[0][1]), 17)
        self.assertEquals(len(rm.model[1][0]), 9)
        self.assertEquals(len(rm.model[1][1]), 9)
        self.assertEquals(len(rm.model[2][0]), 17)
        self.assertEquals(len(rm.model[2][1]), 17)
        self.assertTrue(np.all(func0(rm.model[0][0]) == rm.model[0][1]))
        self.assertTrue(np.all(func1(rm.model[1][0]) == rm.model[1][1]))
        self.assertTrue(np.all(func2(rm.model[2][0]) == rm.model[2][1]))

    def test_ResolutionModel_array_single(self):
        from CrystalField import ResolutionModel

        x = [1, 2, 3]
        y = [3, 2, 1]
        rm = ResolutionModel((x, y))
        self.assertEquals(rm.model[0], x)
        self.assertEquals(rm.model[1], y)

    def test_ResolutionModel_array_multi(self):
        from CrystalField import ResolutionModel

        x0 = [1, 2, 3]
        y0 = [3, 2, 1]
        x1 = [4, 5, 6]
        y1 = [6, 5, 4]
        rm = ResolutionModel([(x0, y0), (x1, y1)])

        self.assertEquals(len(rm.model), 2)
        self.assertEquals(rm.model[0][0], x0)
        self.assertEquals(rm.model[0][1], y0)
        self.assertEquals(rm.model[1][0], x1)
        self.assertEquals(rm.model[1][1], y1)

    def test_ResolutionModel_set_single(self):
        from CrystalField import ResolutionModel, CrystalField

        x = [0, 50]
        y = [1, 2]
        rm = ResolutionModel((x, y))

        cf = CrystalField('Ce', 'C2v', B20=0.37, B22=3.97, B40=-0.0317, B42=-0.116, B44=-0.12,
                      Temperature=44.0, FWHM=1.0, ResolutionModel=rm)
        sp = cf.getSpectrum()
        self.assertAlmostEqual(cf.peaks.param[1]['PeakCentre'], 29.0507341109, 8)
        self.assertAlmostEqual(cf.peaks.param[0]['FWHM'], 1.0, 8)
        self.assertAlmostEqual(cf.peaks.param[1]['FWHM'], 1.581014682, 8)
        self.assertAlmostEqual(cf.peaks.param[2]['FWHM'], 1.884945866, 8)

    def test_ResolutionModel_set_single_variation(self):
        from CrystalField import ResolutionModel, CrystalField

        x = [0, 50]
        y = [1, 2]
        rm = ResolutionModel((x, y))

        cf = CrystalField('Ce', 'C2v', B20=0.37, B22=3.97, B40=-0.0317, B42=-0.116, B44=-0.12,
                      Temperature=44.0, FWHM=1.0, ResolutionModel=rm, FWHMVariation=0.3)
        sp = cf.getSpectrum()
        self.assertAlmostEqual(cf.peaks.param[0]['FWHM'], 1.0, 8)
        self.assertAlmostEqual(cf.peaks.param[1]['FWHM'], 1.58101, 1)
        self.assertAlmostEqual(cf.peaks.param[2]['FWHM'], 1.85644, 1)

    def test_ResolutionModel_set_multi(self):
        from CrystalField import ResolutionModel, CrystalField, CrystalFieldFit

        x0 = [0, 50]
        y0 = [1, 2]
        x1 = [0, 51]
        y1 = [3, 4]
        rm = ResolutionModel([(x0, y0), (x1, y1)])

        cf = CrystalField('Ce', 'C2v', B20=0.37, B22=3.97, B40=-0.0317, B42=-0.116, B44=-0.12,
                      Temperature=[44.0, 50], ResolutionModel=rm)

        att = cf.function.getAttributeValue('FWHMX0')
        self.assertEqual(att[0], 0)
        self.assertEqual(att[1], 50)
        att = cf.function.getAttributeValue('FWHMY0')
        self.assertEqual(att[0], 1)
        self.assertEqual(att[1], 2)
        att = cf.function.getAttributeValue('FWHMX1')
        self.assertEqual(att[0], 0)
        self.assertEqual(att[1], 51)
        att = cf.function.getAttributeValue('FWHMY1')
        self.assertEqual(att[0], 3)
        self.assertEqual(att[1], 4)

    def test_ResolutionModel_set_multi_variation(self):
        from CrystalField import ResolutionModel, CrystalField, CrystalFieldFit

        x0 = [0, 50]
        y0 = [1, 2]
        x1 = [1, 51]
        y1 = [3, 4]
        rm = ResolutionModel([(x0, y0), (x1, y1)])

        cf = CrystalField('Ce', 'C2v', B20=0.37, B22=3.97, B40=-0.0317, B42=-0.116, B44=-0.12,
                      Temperature=[44.0, 50], ResolutionModel=rm, FWHMVariation=0.1)

        att = cf.function.getAttributeValue('FWHMX0')
        self.assertEqual(att[0], 0)
        self.assertEqual(att[1], 50)
        att = cf.function.getAttributeValue('FWHMY0')
        self.assertEqual(att[0], 1)
        self.assertEqual(att[1], 2)
        att = cf.function.getAttributeValue('FWHMX1')
        self.assertEqual(att[0], 1)
        self.assertEqual(att[1], 51)
        att = cf.function.getAttributeValue('FWHMY1')
        self.assertEqual(att[0], 3)
        self.assertEqual(att[1], 4)
        self.assertEqual(cf.FWHMVariation, 0.1)

    def test_peak_width_update(self):
        from CrystalField import ResolutionModel, CrystalField

        x = [0, 50]
        y = [0.1, 2]
        rm = ResolutionModel((x, y))

        cf1 = CrystalField('Ce', 'C2v', B20=0.37, B22=3.97, B40=-0.0317, B42=-0.116, B44=-0.12,
                           Temperature=44.0, FWHM=1.0, ResolutionModel=rm, FWHMVariation=0.01)

        cf2 = CrystalField('Ce', 'C2v', B20=0.57, B22=2.97, B40=-0.0317, B42=-0.116, B44=-0.12,
                           Temperature=44.0, FWHM=1.0, ResolutionModel=rm, FWHMVariation=0.01)

        cf1['B20'] = 0.57
        cf1['B22'] = 2.97
        self.assertEqual(cf1.peaks.param[1]['Amplitude'], cf2.peaks.param[1]['Amplitude'],)
        self.assertEqual(cf1.peaks.param[1]['FWHM'], cf2.peaks.param[1]['FWHM'],)

    def test_monte_carlo_single_spectrum(self):
        from CrystalField.fitting import makeWorkspace
        from CrystalField import CrystalField, CrystalFieldFit, Background, Function

        # Create some crystal field data
        origin = CrystalField('Ce', 'C2v', B20=0.37737, B22=3.9770, B40=-0.031787, B42=-0.11611, B44=-0.12544,
                              Temperature=44.0, FWHM=1.1)
        x, y = origin.getSpectrum()
        ws = makeWorkspace(x, y)

        # Define a CrystalField object with parameters slightly shifted.
        cf = CrystalField('Ce', 'C2v', B20=0, B22=0, B40=0, B42=0, B44=0,
                          Temperature=44.0, FWHM=1.0)

        # Set the ties
        cf.ties(B20=0.37737)
        cf.constraints('2<B22<6', '-0.2<B40<0.2', '-0.2<B42<0.2', '-0.2<B44<0.2')
        # Create a fit object
        fit = CrystalFieldFit(cf, InputWorkspace=ws)
        fit.monte_carlo(NSamples=100, Constraints='20<f1.PeakCentre<45,20<f2.PeakCentre<45', Seed=123)
        # Run fit
        fit.fit()
        self.assertTrue(cf.chi2 > 0.0)
        self.assertTrue(cf.chi2 < 100.0)

    def test_monte_carlo_multi_spectrum(self):
        from CrystalField.fitting import makeWorkspace
        from CrystalField import CrystalField, CrystalFieldFit, Background, Function, ResolutionModel

        # Create some crystal field data
        origin = CrystalField('Ce', 'C2v', B20=0.37737, B22=3.9770, B40=-0.031787, B42=-0.11611, B44=-0.12544,
                              Temperature=[44.0, 50.0], FWHM=[1.0, 1.0])
        ws1 = makeWorkspace(*origin.getSpectrum(0))
        ws2 = makeWorkspace(*origin.getSpectrum(1))

        # Define a CrystalField object with parameters slightly shifted.
        cf = CrystalField('Ce', 'C2v', B20=0.37737, B22=3.9770, B40=0, B42=0, B44=0, NPeaks=11,
                          Temperature=[44.0, 50.0], FWHM=[1.0, 1.0])

        # Set the ties
        cf.ties(B20=0.37737)
        cf.constraints('2<B22<6', '-0.2<B40<0.2', '-0.2<B42<0.2', '-0.2<B44<0.2')
        # Create a fit object
        fit = CrystalFieldFit(cf, InputWorkspace=[ws1, ws2])
        fit.monte_carlo(NSamples=100, Constraints='20<f0.f1.PeakCentre<45,20<f0.f2.PeakCentre<45', Seed=123)
        # Run fit
        fit.fit()
        self.assertTrue(cf.chi2 > 0.0)
        self.assertTrue(cf.chi2 < 200.0)

    def test_normalisation(self):
        from CrystalField.normalisation import split2range
        ranges = split2range(Ion='Pr', EnergySplitting=10,
                             Parameters=['B20', 'B22', 'B40', 'B42', 'B44', 'B60', 'B62', 'B64', 'B66'])
        self.assertAlmostEqual(ranges['B44'], 0.013099063718959822, 7)
        self.assertAlmostEqual(ranges['B60'], 0.00010601965319487525, 7)
        self.assertAlmostEqual(ranges['B40'], 0.0022141458870077678, 7)
        self.assertAlmostEqual(ranges['B42'], 0.009901961430901874, 7)
        self.assertAlmostEqual(ranges['B64'], 0.00084150490931686321, 7)
        self.assertAlmostEqual(ranges['B22'], 0.19554806997215959, 7)
        self.assertAlmostEqual(ranges['B20'], 0.11289973083793813, 7)
        self.assertAlmostEqual(ranges['B66'], 0.0011394030334966495, 7)
        self.assertAlmostEqual(ranges['B62'], 0.00076818536847364201, 7)

    def test_estimate_parameters_cross_entropy(self):
        from CrystalField.fitting import makeWorkspace
        from CrystalField import CrystalField, CrystalFieldFit, Background, Function

        # Create some crystal field data
        origin = CrystalField('Ce', 'C2v', B20=0.37737, B22=3.9770, B40=-0.031787, B42=-0.11611, B44=-0.12544,
                              Temperature=44.0, FWHM=1.1)
        x, y = origin.getSpectrum()
        ws = makeWorkspace(x, y)

        # Define a CrystalField object with parameters slightly shifted.
        cf = CrystalField('Ce', 'C2v', B20=0, B22=0, B40=0, B42=0, B44=0,
                          Temperature=44.0, FWHM=1.0)

        # Set the ties
        cf.ties(B20=0.37737)
        # Create a fit object
        fit = CrystalFieldFit(cf, InputWorkspace=ws)
        fit.estimate_parameters(50, ['B22', 'B40', 'B42', 'B44'],
                                constraints='20<f1.PeakCentre<45,20<f2.PeakCentre<45',
                                Type='Cross Entropy', NSamples=10, Seed=123)
        # Run fit
        fit.fit()
        self.assertTrue(cf.chi2 < 100.0)

    def test_estimate_parameters_multiple_results(self):
        from CrystalField.fitting import makeWorkspace
        from CrystalField import CrystalField, CrystalFieldFit, Background, Function

        # Create some crystal field data
        origin = CrystalField('Ce', 'C2v', B20=0.37737, B22=3.9770, B40=-0.031787, B42=-0.11611, B44=-0.12544,
                              Temperature=44.0, FWHM=1.1)
        x, y = origin.getSpectrum()
        ws = makeWorkspace(x, y)

        # Define a CrystalField object with parameters slightly shifted.
        cf = CrystalField('Ce', 'C2v', B20=0, B22=0, B40=0, B42=0, B44=0,
                          Temperature=44.0, FWHM=1.0)

        # Set the ties
        cf.ties(B20=0.37737)
        # Create a fit object
        fit = CrystalFieldFit(cf, InputWorkspace=ws)
        fit.estimate_parameters(50, ['B22', 'B40', 'B42', 'B44'],
                                constraints='20<f1.PeakCentre<45,20<f2.PeakCentre<45', NSamples=100, Seed=123)
        self.assertTrue(fit.get_number_estimates() > 1)
        fit.fit()
        self.assertTrue(cf.chi2 < 100.0)

    def test_intensity_scaling_single_spectrum(self):
        from CrystalField import CrystalField, CrystalFieldFit, Background, Function

        # Define a CrystalField object with parameters slightly shifted.
        cf = CrystalField('Ce', 'C2v', B20=0.37737, B22=3.9770, B40=-0.031787, B42=-0.11611, B44=-0.12544,
                          Temperature=44.0, FWHM=1.0)
        _, y0 = cf.getSpectrum()

        cf.IntensityScaling = 2.5
        _, y1 = cf.getSpectrum()

        self.assertTrue(np.all(y1 / y0 > 2.49999999999))
        self.assertTrue(np.all(y1 / y0 < 2.50000000001))

    def test_intensity_scaling_single_spectrum_1(self):
        from CrystalField import CrystalField, CrystalFieldFit, Background, Function

        # Define a CrystalField object with parameters slightly shifted.
        cf = CrystalField('Ce', 'C2v', IntensityScaling=2.5, B20=0.37737, B22=3.9770, B40=-0.031787,
                          B42=-0.11611, B44=-0.12544, Temperature=44.0, FWHM=1.0)
        _, y0 = cf.getSpectrum()

        cf.IntensityScaling = 1.0
        _, y1 = cf.getSpectrum()

        self.assertTrue(np.all(y0 / y1 > 2.49999999999))
        self.assertTrue(np.all(y0 / y1 < 2.50000000001))

    def test_intensity_scaling_multi_spectrum(self):
        from CrystalField.fitting import makeWorkspace
        from CrystalField import CrystalField, CrystalFieldFit, Background, Function

        # Define a CrystalField object with parameters slightly shifted.
        cf = CrystalField('Ce', 'C2v', B20=0.37737, B22=3.9770, B40=-0.031787, B42=-0.11611, B44=-0.12544,
                          Temperature=[44.0, 50.0], FWHM=[1.0, 1.2])
        x, y0 = cf.getSpectrum(0)
        ws0 = makeWorkspace(x, y0)
        x, y1 = cf.getSpectrum(1)
        ws1 = makeWorkspace(x, y1)

        cf.IntensityScaling = [2.5, 1.5]

        EvaluateFunction(cf.makeMultiSpectrumFunction(), InputWorkspace=ws0, InputWorkspace_1=ws1,
                         OutputWorkspace='out')
        out = mtd['out']
        out0 = out[0].readY(1)
        out1 = out[1].readY(1)

        self.assertTrue(np.all(out0 / y0 > 2.49))
        self.assertTrue(np.all(out0 / y0 < 2.51))

        self.assertTrue(np.all(out1 / y1 > 1.49))
        self.assertTrue(np.all(out1 / y1 < 1.51))

    def test_CrystalField_PointCharge_ligand(self):
        from CrystalField import PointCharge
        # Set up a PointCharge object with ligands explicitly specified (2 -2|e| charges at +/-3A in z)
        pc = PointCharge([[-2, 0, 0, 3], [-2, 0, 0, -3]], 'Ce')
        blm = pc.calculate()
        # For axial ligands, only expect m=0 terms. 
        # For Ce, l=6 term should be zero because only l<2J allowed, and J=2.5 for Ce.
        self.assertEqual(len(blm), 2)
        self.assertTrue('B20' in blm.keys())
        self.assertTrue('B40' in blm.keys())
        pc = PointCharge(Structure=[[-2, 0, 0, 3], [-2, 0, 0, -3]], Ion='Pr')
        blm = pc.calculate()
        # For Pr, J=4, so all three m=0 terms B20, B40, B60 should be nonzero
        self.assertEqual(len(blm), 3)
        self.assertTrue('B20' in blm.keys())
        self.assertTrue('B40' in blm.keys())
        self.assertTrue('B60' in blm.keys())

    def test_CrystalField_PointCharge_workspace(self):
        from CrystalField import PointCharge
        from mantid.geometry import CrystalStructure
        from mantid.simpleapi import CreateWorkspace, DeleteWorkspace
        import uuid
        perovskite = CrystalStructure('4 4 4', 'P m -3 m', 
                                      'Ce 0. 0. 0. 1. 0.; Al 0.5 0.5 0.5 1. 0.; O 0.5 0.5 0. 1. 0.')
        # Check direct input of CrystalStructure works
        pc = PointCharge(perovskite, 'Ce', {'Ce':3, 'Al':3, 'O':-2})
        blm0 = pc.calculate()
        # Set up a PointCharge calculation from a workspace with a structure
        # Cannot use CrystalField.fitting.makeWorkspace because it only gets passed as a string (ws name) in Python.
        ws = CreateWorkspace(1, 1, OutputWorkspace='ws_'+str(uuid.uuid4())[:8])
        ws.sample().setCrystalStructure(perovskite)
        pc = PointCharge(ws, 'Ce', {'Ce':3, 'Al':3, 'O':-2})
        blm = pc.calculate()
        for k, v in blm.items():
            self.assertAlmostEqual(blm0[k], v)
        self.assertEqual(len(blm), 2)
        self.assertAlmostEqual(blm['B44'] / blm['B40'], 5., 3)   # Cubic symmetry implies B44=5B40
        self.assertRaises(ValueError, PointCharge, ws, 'Pr', {'Ce':3, 'Al':3, 'O':-2})
        pc = PointCharge(Structure=ws, IonLabel='Ce', Charges={'Ce':3, 'Al':3, 'O':-2}, Ion='Pr')
        blm = pc.calculate()
        self.assertEqual(len(blm), 4)
        self.assertAlmostEqual(blm['B44'] / blm['B40'], 5., 3)   # Cubic symmetry implies B44=5B40
        self.assertAlmostEqual(blm['B64'] / blm['B60'], -21., 3) # Cubic symmetry implies B64=-21B60
        DeleteWorkspace(ws)

    def test_CrystalField_PointCharge_file(self):
        from CrystalField import PointCharge
        import sys
        import mantid.simpleapi
        if sys.version_info.major == 3:
            from unittest import mock
        else:
            import mock
        # Just check that LoadCIF is called... we'll rely on LoadCIF working properly!
        with mock.patch.object(mantid.simpleapi, 'LoadCIF') as loadcif:
            self.assertRaises(RuntimeError, PointCharge, 'somefile.cif')  # Error because no actual CIF loaded
        loadcif.assert_called_with(mock.ANY, 'somefile.cif')

    def test_CrystalFieldFit_physical_properties(self):
        from CrystalField.fitting import makeWorkspace
        from CrystalField import CrystalField, CrystalFieldFit, PhysicalProperties
        origin = CrystalField('Ce', 'C2v', B20=0.37737, B22=3.9770, B40=-0.031787, B42=-0.11611, B44=-0.12544,
                              Temperature=[10, 100], FWHM=[1.1, 1.2])
        ws0 = makeWorkspace(*origin.getSpectrum(0))
        ws1 = makeWorkspace(*origin.getSpectrum(1))
        wscv = makeWorkspace(*origin.getHeatCapacity())
        wschi = makeWorkspace(*origin.getSusceptibility(Hdir='powder'))
        wsmag = makeWorkspace(*origin.getMagneticMoment(Hdir=[0,1,0], Hmag=np.linspace(0,30,3)))
        wsmom = makeWorkspace(*origin.getMagneticMoment(H=100, T=np.linspace(1,300,300), Unit='cgs'))

        # Fits single physical properties dataset
        cf = CrystalField('Ce', 'C2v', B20=0.37, B22=3.97, B40=-0.0317, B42=-0.116, B44=-0.12)
        cf.PhysicalProperty = PhysicalProperties('susc', Hdir='powder')
        fit = CrystalFieldFit(Model=cf, InputWorkspace=wschi, MaxIterations=100)
        fit.fit()
        self.assertAlmostEqual(cf['B20'], 0.37737, 4)
        self.assertAlmostEqual(cf['B22'], 3.97700087765, 4)
        self.assertAlmostEqual(cf['B40'], -0.0317867635188, 4)
        self.assertAlmostEqual(cf['B42'], -0.116110640723, 4)
        self.assertAlmostEqual(cf['B44'], -0.125439939584, 4)

        # Fits multiple physical properties
        cf = CrystalField('Ce', 'C2v', B20=0.37, B22=3.97, B40=-0.0317, B42=-0.116, B44=-0.12)
        cf.PhysicalProperty = [PhysicalProperties('susc', 'powder'), PhysicalProperties('M(H)', Hdir=[0,1,0])]
        fit = CrystalFieldFit(Model=cf, InputWorkspace=[wschi, wsmag], MaxIterations=100)
        fit.fit()
        self.assertAlmostEqual(cf['B20'], 0.37737, 4)
        self.assertAlmostEqual(cf['B22'], 3.97700087765, 4)
        self.assertAlmostEqual(cf['B40'], -0.0317867635188, 4)
        self.assertAlmostEqual(cf['B42'], -0.116110640723, 4)
        self.assertAlmostEqual(cf['B44'], -0.125439939584, 4)

        # Fits one INS spectrum and one physical property
        cf = CrystalField('Ce', 'C2v', B20=0.37, B22=3.97, B40=-0.0317, B42=-0.116, B44=-0.12, Temperature=10, FWHM=1.1)
        cf.PhysicalProperty = PhysicalProperties('M(H)',Hdir=[0,1,0])
        fit = CrystalFieldFit(Model=cf, InputWorkspace=[ws0, wsmag], MaxIterations=100)
        fit.fit()
        self.assertAlmostEqual(cf['B20'], 0.37737, 3)
        self.assertAlmostEqual(cf['B22'], 3.97700087765, 3)
        self.assertAlmostEqual(cf['B40'], -0.0317867635188, 3)
        self.assertAlmostEqual(cf['B42'], -0.116110640723, 3)
        self.assertAlmostEqual(cf['B44'], -0.125439939584, 3)

        # Fits multiple INS spectra and multiple physical properties
        cf = CrystalField('Ce', 'C2v', B20=0.37, B22=3.97, B40=-0.0317, B42=-0.116, B44=-0.12,
                          Temperature=[10, 100], FWHM=[1.1, 1.2], PhysicalProperty = [PhysicalProperties('susc', 'powder'),
                          PhysicalProperties('M(H)', Hdir=[0,1,0])])

        fit = CrystalFieldFit(Model=cf, InputWorkspace=[ws0, ws1, wschi, wsmag], MaxIterations=1)
        fit.fit()
        self.assertAlmostEqual(cf['B20'], 0.37737, 1)
        self.assertAlmostEqual(cf['B22'], 3.97700087765, 1)
        self.assertAlmostEqual(cf['B40'], -0.0317867635188, 1)
        self.assertAlmostEqual(cf['B42'], -0.116110640723, 1)
        self.assertAlmostEqual(cf['B44'], -0.125439939584, 1)


if __name__ == "__main__":
    unittest.main()
