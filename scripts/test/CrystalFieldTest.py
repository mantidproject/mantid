"""Test suite for the crystal field calculations in the Inelastic/CrystalField package
"""
import unittest
import numpy as np

# Import mantid to setup the python paths to the bundled scripts
import mantid
from CrystalField.energies import energies


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
        #   terms but divide by gJ*uB (gJ=0.8 for U4+/Pr3+ and uB=0.6715)
        _, _, hx = energies(2, BextX=1.0)
        _, _, hy = energies(2, BextY=1.0)
        _, _, hz = energies(2, BextZ=1.0)
        ix = np.dot(np.conj(np.transpose(wf)), np.dot(hx, wf))
        iy = np.dot(np.conj(np.transpose(wf)), np.dot(hy, wf))
        iz = np.dot(np.conj(np.transpose(wf)), np.dot(hz, wf))
        gJuB = 0.53716
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
        self.assertEquals(pl.shape, (2, 7))
        self.assertAlmostEqual(pl[0, 0], 0.0, 10)
        self.assertAlmostEqual(pl[1, 0], 1.99118947, 8)
        self.assertAlmostEqual(pl[0, 1], 3.85696607, 8)
        self.assertAlmostEqual(pl[1, 1], 0.86130642, 8)
        self.assertAlmostEqual(pl[0, 2], 2.41303393, 8)
        self.assertAlmostEqual(pl[1, 2], 0.37963778, 8)

    def test_api_CrystalField_peaks_list_2(self):
        from CrystalField import CrystalField
        cf = CrystalField('Ce', 'C2v', B20=0.035, B40=-0.012, B43=-0.027, B60=-0.00012, B63=0.0025, B66=0.0068,
                          Temperature=[44.0, 50.0])
        pl1 = cf.getPeakList()
        self.assertEquals(pl1.shape, (2, 7))
        self.assertAlmostEqual(pl1[0, 0], 0.0, 10)
        self.assertAlmostEqual(pl1[1, 0], 1.99118947, 8)
        self.assertAlmostEqual(pl1[0, 1], 3.85696607, 8)
        self.assertAlmostEqual(pl1[1, 1], 0.86130642, 8)
        self.assertAlmostEqual(pl1[0, 2], 2.41303393, 8)
        self.assertAlmostEqual(pl1[1, 2], 0.37963778, 8)

        pl2 = cf.getPeakList(1)
        self.assertEquals(pl2.shape, (2, 7))
        self.assertAlmostEqual(pl2[0, 0], 0.0, 10)
        self.assertAlmostEqual(pl2[1, 0], 1.97812511, 8)
        self.assertAlmostEqual(pl2[0, 1], 3.85696607, 8)
        self.assertAlmostEqual(pl2[1, 1], 0.82930948, 8)
        self.assertAlmostEqual(pl2[0, 2], 2.41303393, 8)
        self.assertAlmostEqual(pl2[1, 2], 0.38262684, 8)

    def test_PeaksFunction(self):
        from CrystalField import PeaksFunction
        pf = PeaksFunction('Gaussian')
        pf.param[0]['Sigma'] = 1.1
        pf.attr[0]['SomeAttr'] = 'Hello'
        pf.param[1]['Sigma'] = 2.1
        pf.param[1]['Height'] = 100
        self.assertEquals(pf.paramString(), 'f0.SomeAttr=Hello,f0.Sigma=1.1,f1.Sigma=2.1,f1.Height=100')
        self.assertEquals(pf.toString(), 'name=Gaussian,SomeAttr=Hello,Sigma=1.1;name=Gaussian,Sigma=2.1,Height=100')

    def test_api_CrystalField_spectrum(self):
        from CrystalField import CrystalField
        cf = CrystalField('Ce', 'C2v', B20=0.035, B40=-0.012, B43=-0.027, B60=-0.00012, B63=0.0025, B66=0.0068,
                          Temperature=[4.0, 50.0], FWHM=[0.1, 0.2])
        x, y = cf.getSpectrum(0)
        self.assertAlmostEqual(y[60], 5.52333486, 8)
        self.assertAlmostEqual(y[61], 10.11673418, 8)
        self.assertAlmostEqual(y[62], 12.1770908, 8)
        self.assertAlmostEqual(y[63], 7.63981716, 8)
        self.assertAlmostEqual(y[64], 4.08015236, 8)
        x, y = cf.getSpectrum(1)
        self.assertAlmostEqual(y[45], 0.29653544, 8)
        self.assertAlmostEqual(y[46], 0.45996719, 8)
        self.assertAlmostEqual(y[47], 0.65873995, 8)
        self.assertAlmostEqual(y[48], 0.6924739, 8)
        self.assertAlmostEqual(y[49], 0.51119472, 8)

    def test_api_CrystalField_spectrum_0(self):
        from CrystalField import CrystalField
        cf = CrystalField('Ce', 'C2v', B20=0.035, B40=-0.012, B43=-0.027, B60=-0.00012, B63=0.0025, B66=0.0068,
                          Temperature=4.0, FWHM=0.1)
        x, y = cf.getSpectrum()
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
        self.assertAlmostEqual(y[0], 12.474954833565066, 8)
        self.assertAlmostEqual(y[1], 4.3004160689570403, 8)
        self.assertAlmostEqual(y[2], 1.4523089577890338, 8)
        self.assertAlmostEqual(y[3], 0.6922657279528992, 8)
        self.assertAlmostEqual(y[4], 0.40107924259746491, 8)
        self.assertAlmostEqual(y[15], 0.050129858433581413, 8)
        self.assertAlmostEqual(y[16], 0.054427788297191478, 8)
        x, y = cf.getSpectrum(1, workspace)
        self.assertAlmostEqual(y[0], 6.3037011243626706, 8)
        self.assertAlmostEqual(y[1], 4.2744246158088393, 8)
        self.assertAlmostEqual(y[2], 2.1770150365787457, 8)
        self.assertAlmostEqual(y[3], 1.2003766255981201, 8)
        self.assertAlmostEqual(y[4], 0.73968415199300741, 8)
        x, y = cf.getSpectrum(workspace)
        self.assertAlmostEqual(y[0], 12.474954833565066, 8)
        self.assertAlmostEqual(y[1], 4.3004160689570403, 8)
        workspace = CreateWorkspace(x, y, e, 2)
        x, y = cf.getSpectrum(workspace, 1)
        self.assertAlmostEqual(y[0], 0.050129858433581413, 8)
        self.assertAlmostEqual(y[1], 0.054427788297191478, 8)

    def test_api_CrystalField_spectrum_peaks(self):
        from CrystalField import CrystalField, PeaksFunction
        cf = CrystalField('Ce', 'C2v', B20=0.035, B40=-0.012, B43=-0.027, B60=-0.00012, B63=0.0025, B66=0.0068,
                          Temperature=10.0, FWHM=0.1)
        cf.peaks = PeaksFunction('Gaussian')
        cf.peaks.param[1]['Sigma'] = 0.05
        cf.peaks.param[2]['Sigma'] = 0.1
        cf.peaks.param[3]['Sigma'] = 0.2
        cf.peaks.param[4]['Sigma'] = 0.3
        x, y = cf.getSpectrum()
        self.assertAlmostEqual(y[123], 0.067679792127989441, 8)
        self.assertAlmostEqual(y[124], 0.099056455104978708, 8)

    def test_api_CrystalField_spectrum_peaks_multi(self):
        from CrystalField import CrystalField, PeaksFunction
        cf = CrystalField('Ce', 'C2v', B20=0.035, B40=-0.012, B43=-0.027, B60=-0.00012, B63=0.0025, B66=0.0068,
                          Temperature=[10.0, 10.0], FWHM=1.0)
        cf.setPeaks('Gaussian')
        cf.peaks[0].param[1]['Sigma'] = 0.1
        cf.peaks[0].param[2]['Sigma'] = 0.2
        cf.peaks[0].param[3]['Sigma'] = 0.3
        x0, y0 = cf.getSpectrum()
        x1, y1 = cf.getSpectrum(1)
        self.assertAlmostEqual(y0[139], 0.094692329804360792, 8)
        self.assertAlmostEqual(y0[142], 0.07623409141946233, 8)
        self.assertAlmostEqual(y1[139], 0.16332256923203797, 8)
        self.assertAlmostEqual(y1[142], 0.16601423535307261, 8)

    def test_api_CrystalField_spectrum_background(self):
        from CrystalField import CrystalField, PeaksFunction, Background, Function
        cf = CrystalField('Ce', 'C2v', B20=0.035, B40=-0.012, B43=-0.027, B60=-0.00012, B63=0.0025, B66=0.0068,
                          Temperature=10.0, FWHM=0.1)
        cf.peaks = PeaksFunction('Gaussian')
        cf.peaks.param[1]['Sigma'] = 0.1
        cf.peaks.param[2]['Sigma'] = 0.2
        cf.peaks.param[3]['Sigma'] = 0.3
        cf.background = Background(peak=Function('PseudoVoigt', Height=10, FWHM=1, Mixing=0.5),
                                   background=Function('LinearBackground', A0=1.0, A1=0.1))
        x, y = cf.getSpectrum()
        self.assertAlmostEqual(y[80], 2.5853135104737239, 8)
        self.assertAlmostEqual(y[90], 6.6726231052015859, 8)


class CrystalFieldFitTest(unittest.TestCase):

    def test_CrystalFieldFit(self):
        from CrystalField.fitting import MakeWorkspace
        from CrystalField import CrystalField, CrystalFieldFit
        origin = CrystalField('Ce', 'C2v', B20=0.035, B40=-0.012, B43=-0.027, B60=-0.00012, B63=0.0025, B66=0.0068,
                          Temperature=10.0, FWHM=0.1)
        x, y = origin.getSpectrum()
        ws = MakeWorkspace(x, y)

        cf = CrystalField('Ce', 'C2v', B20=0.035, B40=-0.01, B43=-0.03, B60=-0.0001, B63=0.002, B66=0.006,
                              Temperature=10.0, FWHM=0.1)
        cf.setPeaks('Gaussian')
        cf.peaks.ties({'f0.Sigma':1.0, 'f1.Sigma': '2*f0.Sigma'})
        print cf.makeSpectrumFunction()
        #fit = CrystalFieldFit(cf, InputWorkspace=ws)
        #print fit.fit()

if __name__ == "__main__":
    unittest.main()
