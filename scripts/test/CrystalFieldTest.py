"""Test suite for the crystal field calculations in the Inelastic/CrystalField package
"""
import unittest
import numpy as np

# Import mantid to setup the python paths to the bundled scripts
import mantid
from CrystalField.energies import energies
from mantid.simpleapi import CalculateChiSquared
from mantid.kernel import ConfigService

c_mbsr = 79.5774715459  # Conversion from barn to mb/sr

class BackgroundTest(unittest.TestCase):

    def setUp(self):
        self.peakRadius = ConfigService.getString('curvefitting.peakRadius')

    def tearDown(self):
        ConfigService.setString('curvefitting.peakRadius', self.peakRadius)

    def test_mul(self):
        from CrystalField import Background, Function
        b = Background(peak=Function('PseudoVoigt', Height=10, FWHM=1, Mixing=0.5),
                       background=Function('LinearBackground', A0=1.0, A1=0.1)) * 3
        self.assertEquals(len(b), 3)
        self.assertTrue(isinstance(b[0], Background))
        self.assertTrue(isinstance(b[1], Background))
        self.assertTrue(isinstance(b[2], Background))
        b[0].peak.param['Height'] = 31
        b[1].peak.param['Height'] = 41
        b[2].peak.param['Height'] = 51
        self.assertEquals(b[0].peak.param['Height'], 31)
        self.assertEquals(b[1].peak.param['Height'], 41)
        self.assertEquals(b[2].peak.param['Height'], 51)
        b[0].background.param['A1'] = 3
        b[1].background.param['A1'] = 4
        b[2].background.param['A1'] = 5
        self.assertEquals(b[0].background.param['A1'], 3)
        self.assertEquals(b[1].background.param['A1'], 4)
        self.assertEquals(b[2].background.param['A1'], 5)


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
        self.assertEquals(pl1.shape, (2, 7))
        self.assertAlmostEqual(pl1[0, 0], 0.0, 10)
        self.assertAlmostEqual(pl1[1, 0], 1.99118947*c_mbsr, 6)
        self.assertAlmostEqual(pl1[0, 1], 3.85696607, 8)
        self.assertAlmostEqual(pl1[1, 1], 0.86130642*c_mbsr, 6)
        self.assertAlmostEqual(pl1[0, 2], 2.41303393, 8)
        self.assertAlmostEqual(pl1[1, 2], 0.37963778*c_mbsr, 6)

        pl2 = cf.getPeakList(1)
        self.assertEquals(pl2.shape, (2, 7))
        self.assertAlmostEqual(pl2[0, 0], 0.0, 10)
        self.assertAlmostEqual(pl2[1, 0], 1.97812511*c_mbsr, 6)
        self.assertAlmostEqual(pl2[0, 1], 3.85696607, 8)
        self.assertAlmostEqual(pl2[1, 1], 0.82930948*c_mbsr, 6)
        self.assertAlmostEqual(pl2[0, 2], 2.41303393, 8)
        self.assertAlmostEqual(pl2[1, 2], 0.38262684*c_mbsr, 6)

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
                          Temperature=[4.0, 50.0], FWHM=[0.1, 0.2], ToleranceIntensity=0.001*c_mbsr)
        x, y = cf.getSpectrum(0)
        y = y / c_mbsr
        self.assertAlmostEqual(y[60], 5.52333486, 8)
        self.assertAlmostEqual(y[61], 10.11673418, 8)
        self.assertAlmostEqual(y[62], 12.1770908, 8)
        self.assertAlmostEqual(y[63], 7.63981716, 8)
        self.assertAlmostEqual(y[64], 4.08015236, 8)
        x, y = cf.getSpectrum(1)
        y = y / c_mbsr
        self.assertAlmostEqual(y[45], 0.29822612216224065, 8)
        self.assertAlmostEqual(y[46], 0.46181038787922241, 8)
        self.assertAlmostEqual(y[47], 0.66075719314988057, 8)
        self.assertAlmostEqual(y[48], 0.69469096259927476, 8)
        self.assertAlmostEqual(y[49], 0.51364268980567007, 8)

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

        self.assertAlmostEqual(y[0], 12.474954833565066, 6)
        self.assertAlmostEqual(y[1], 1.1901690051585272, 6)
        self.assertAlmostEqual(y[2], 0.12278091428521705, 6)
        self.assertAlmostEqual(y[3], 0.042940202606241519, 6)
        self.assertAlmostEqual(y[4], 10.837438382097396, 6)

        x, y = cf.getSpectrum(1, r)
        y = y / c_mbsr
        self.assertEqual(x[0], 0.0)
        self.assertEqual(x[1], 1.45)
        self.assertEqual(x[2], 2.4)
        self.assertEqual(x[3], 3.0)
        self.assertEqual(x[4], 3.85)

        self.assertAlmostEqual(y[0], 6.3046701386938624, 8)
        self.assertAlmostEqual(y[1], 0.33121919026244667, 8)
        self.assertAlmostEqual(y[2], 1.2246681560002572, 8)
        self.assertAlmostEqual(y[3], 0.078541076629159004, 8)
        self.assertAlmostEqual(y[4], 2.6380618652343704, 8)

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
        self.assertAlmostEqual(y[0], 12.474954833565066, 6)
        self.assertAlmostEqual(y[1], 4.3004160689570403, 6)
        self.assertAlmostEqual(y[2], 1.4523089577890338, 6)
        self.assertAlmostEqual(y[3], 0.6922657279528992, 6)
        self.assertAlmostEqual(y[4], 0.40107924259746491, 6)
        self.assertAlmostEqual(y[15], 0.050129858433581413, 6)
        self.assertAlmostEqual(y[16], 0.054427788297191478, 6)
        x, y = cf.getSpectrum(1, workspace)
        y = y / c_mbsr
        self.assertAlmostEqual(y[0], 6.3046701386938624, 6)
        self.assertAlmostEqual(y[1], 4.2753076741531455, 6)
        self.assertAlmostEqual(y[2], 2.1778230746690772, 6)
        self.assertAlmostEqual(y[3], 1.2011188019120242, 6)
        self.assertAlmostEqual(y[4], 0.74036819427919942, 6)
        x, y = cf.getSpectrum(workspace)
        y = y / c_mbsr
        self.assertAlmostEqual(y[0], 12.474954833565066, 6)
        self.assertAlmostEqual(y[1], 4.3004160689570403, 6)
        workspace = CreateWorkspace(x, y, e, 2)
        x, y = cf.getSpectrum(workspace, 1)
        y = y / c_mbsr
        self.assertAlmostEqual(y[0], 0.050129858433581413, 6)
        self.assertAlmostEqual(y[1], 0.054427788297191478, 6)

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
        y = y / c_mbsr
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
        y0 = y0 / c_mbsr
        y1 = y1 / c_mbsr
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
        cf.background = Background(peak=Function('PseudoVoigt', Height=10*c_mbsr, FWHM=1, Mixing=0.5),
                                   background=Function('LinearBackground', A0=1.0*c_mbsr, A1=0.1*c_mbsr))
        x, y = cf.getSpectrum()
        y = y / c_mbsr
        self.assertAlmostEqual(y[80], 2.5853135104737239, 8)
        self.assertAlmostEqual(y[90], 6.6726231052015859, 8)

    def test_api_CrystalField_multi_spectrum_background(self):
        from CrystalField import CrystalField, PeaksFunction, Background, Function
        cf = CrystalField('Ce', 'C2v', B20=0.035, B40=-0.012, B43=-0.027, B60=-0.00012, B63=0.0025, B66=0.0068,
                          Temperature=[10.0, 10.0], FWHM=1.0)
        cf.setPeaks('Gaussian')
        cf.peaks[0].param[1]['Sigma'] = 0.1
        cf.peaks[0].param[2]['Sigma'] = 0.2
        cf.peaks[0].param[3]['Sigma'] = 0.3
        cf.background = Background(peak=Function('Gaussian', Height=10*c_mbsr, Sigma=1),
                                   background=Function('FlatBackground', A0=1.0*c_mbsr)) * 2
        cf.background[0].peak.param['Sigma'] = 0.3
        cf.background[1].peak.param['Sigma'] = 0.4
        cf.background[1].background.param['A0'] = 2*c_mbsr

        x0, y0 = cf.getSpectrum()
        x1, y1 = cf.getSpectrum(1)
        # Original test was for FOCUS convention - intensity in barn. 
        # Now use ISIS convention with intensity in milibarn/steradian
        y0 = y0 / c_mbsr
        y1 = y1 / c_mbsr
        self.assertAlmostEqual(y0[100], 12.882103856689408, 8)
        self.assertAlmostEqual(y0[120], 1.2731198929218952, 8)
        self.assertAlmostEqual(y0[139], 1.0946924013479913, 8)
        self.assertAlmostEqual(y0[150], 1.3385035814782906, 8)
        self.assertAlmostEqual(y1[100], 13.895769108969075, 8)
        self.assertAlmostEqual(y1[120], 2.8138653727130198, 8)
        self.assertAlmostEqual(y1[139], 2.1635845058245273, 8)
        self.assertAlmostEqual(y1[150], 2.1826462206185795, 8)


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
        cf.ties(B20=0.37737, B60=0, B62=0, B64=0, B66=0, IntensityScaling=1)
        cf.ToleranceIntensity = 0.001
        fit = CrystalFieldFit(cf, InputWorkspace=ws)
        fit.fit()
        self.assertAlmostEqual(cf.background.peak.param['PeakCentre'], 7.62501442212e-10, 8)
        self.assertAlmostEqual(cf.background.peak.param['Sigma'], 1.00000000277, 8)
        self.assertAlmostEqual(cf.background.peak.param['Height'], 9.99999983559*c_mbsr, 4)
        self.assertAlmostEqual(cf.background.background.param['A1'], 0.0100000014282, 4)
        self.assertAlmostEqual(cf.background.background.param['A0'], 0.999999976941, 4)
        self.assertEqual(cf.param['IB63'], 0.0)
        self.assertEqual(cf.param['IB62'], 0.0)
        self.assertEqual(cf.param['IB61'], 0.0)
        self.assertEqual(cf.param['IB66'], 0.0)
        self.assertEqual(cf.param['IB65'], 0.0)
        self.assertEqual(cf.param['IB64'], 0.0)
        self.assertEqual(cf.param['IB41'], 0.0)
        self.assertEqual(cf.param['IB43'], 0.0)
        self.assertEqual(cf.param['IB42'], 0.0)
        self.assertEqual(cf.param['IB44'], 0.0)
        self.assertEqual(cf.param['B21'], 0.0)
        self.assertEqual(cf.param['IB22'], 0.0)
        self.assertEqual(cf.param['IB21'], 0.0)
        self.assertEqual(cf.param['BextX'], 0.0)
        self.assertEqual(cf.param['BextY'], 0.0)
        self.assertEqual(cf.param['BextZ'], 0.0)
        self.assertEqual(cf.param['BmolY'], 0.0)
        self.assertEqual(cf.param['BmolX'], 0.0)
        self.assertEqual(cf.param['BmolZ'], 0.0)
        self.assertEqual(cf.param['B66'], 0.0)
        self.assertEqual(cf.param['B63'], 0.0)
        self.assertEqual(cf.param['B65'], 0.0)
        self.assertEqual(cf.param['B62'], 0.0)
        self.assertEqual(cf.param['B61'], 0.0)
        self.assertEqual(cf.param['B60'], 0.0)
        self.assertEqual(cf.param['B41'], 0.0)
        self.assertEqual(cf.param['B43'], 0.0)
        self.assertEqual(cf.param['B64'], 0.0)
        self.assertAlmostEqual(cf.param['B20'], 0.37737, 4)
        self.assertAlmostEqual(cf.param['B22'], 3.97700087765, 4)
        self.assertAlmostEqual(cf.param['B40'], -0.0317867635188, 4)
        self.assertAlmostEqual(cf.param['B42'], -0.116110640723, 4)
        self.assertAlmostEqual(cf.param['B44'], -0.125439939584, 4)
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
        from mantid.simpleapi import FunctionFactory
        origin = CrystalField('Ce', 'C2v', B20=0.37737, B22=3.9770, B40=-0.031787, B42=-0.11611, B44=-0.12544,
                              Temperature=[44.0, 50], FWHM=[1.1, 0.9])
        origin.setPeaks('Lorentzian')
        origin.peaks[0].param[0]['FWHM'] = 1.11
        origin.peaks[1].param[1]['FWHM'] = 1.12
        origin.setBackground(peak=Function('Gaussian', Height=10, Sigma=0.3),
                             background=Function('FlatBackground', A0=1.0))
        origin.background[1].peak.param['Sigma'] = 0.8
        origin.background[1].background.param['A0'] = 1.1
        s = origin.makeMultiSpectrumFunction()
        fun = FunctionFactory.createInitialized(s)

        self.assertEquals(fun.getParameterValue('f0.f0.f0.Sigma'), 0.3)
        self.assertEquals(fun.getParameterValue('f0.f0.f1.A0'), 1.0)
        self.assertEquals(fun.getParameterValue('f1.f0.f0.Sigma'), 0.8)
        self.assertEquals(fun.getParameterValue('f1.f0.f1.A0'), 1.1)

        self.assertEquals(fun.getParameterValue('f0.f1.FWHM'), 1.11)
        self.assertEquals(fun.getParameterValue('f0.f2.FWHM'), 1.1)
        self.assertEquals(fun.getParameterValue('f0.f3.FWHM'), 1.1)

        self.assertEquals(fun.getParameterValue('f1.f1.FWHM'), 0.9)
        self.assertEquals(fun.getParameterValue('f1.f2.FWHM'), 1.12)
        self.assertEquals(fun.getParameterValue('f1.f3.FWHM'), 0.9)

        cf = CrystalField('Ce', 'C2v', B20=0.37737, B22=3.9770, B40=-0.031787, B42=-0.11611, B44=-0.12544,
                          Temperature=[44.0, 50], FWHM=[1.1, 0.9])
        cf.setPeaks('Lorentzian')
        cf.peaks[0].param[0]['FWHM'] = 1.11
        cf.peaks[1].param[1]['FWHM'] = 1.12
        cf.setBackground(peak=Function('Gaussian', Height=10, Sigma=0.3),
                         background=Function('FlatBackground', A0=1.0))
        cf.ties(IntensityScaling0 = 1.0, IntensityScaling1 = 1.0)
        cf.ToleranceIntensity = 0.001

        ws0 = makeWorkspace(*origin.getSpectrum(0))
        ws1 = makeWorkspace(*origin.getSpectrum(1))

        chi2 = CalculateChiSquared(cf.makeMultiSpectrumFunction(), InputWorkspace=ws0,  InputWorkspace_1=ws1)[1]

        fit = CrystalFieldFit(cf, InputWorkspace=[ws0, ws1])
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

        self.assertNotEqual(cf.peaks[0].param[3]['PeakCentre'], 0.0)
        self.assertNotEqual(cf.peaks[0].param[3]['FWHM'], 0.0)
        self.assertNotEqual(cf.peaks[0].param[3]['Amplitude'], 0.0)

        self.assertNotEqual(cf.peaks[1].param[1]['PeakCentre'], 0.0)
        self.assertNotEqual(cf.peaks[1].param[1]['FWHM'], 0.0)
        self.assertNotEqual(cf.peaks[1].param[1]['Amplitude'], 0.0)

        self.assertNotEqual(cf.peaks[1].param[2]['PeakCentre'], 0.0)
        self.assertNotEqual(cf.peaks[1].param[2]['FWHM'], 0.0)
        self.assertNotEqual(cf.peaks[1].param[2]['Amplitude'], 0.0)

        self.assertNotEqual(cf.peaks[1].param[3]['PeakCentre'], 0.0)
        self.assertNotEqual(cf.peaks[1].param[3]['FWHM'], 0.0)
        self.assertNotEqual(cf.peaks[1].param[3]['Amplitude'], 0.0)


    def test_CrystalFieldFit_multi_spectrum_simple_background(self):
        from CrystalField.fitting import makeWorkspace
        from CrystalField import CrystalField, CrystalFieldFit, Background, Function
        from mantid.simpleapi import FunctionFactory
        origin = CrystalField('Ce', 'C2v', B20=0.37737, B22=3.9770, B40=-0.031787, B42=-0.11611, B44=-0.12544,
                              Temperature=[44.0, 50], FWHM=[1.1, 0.9])
        origin.setPeaks('Lorentzian')
        origin.peaks[0].param[0]['FWHM'] = 1.11
        origin.peaks[1].param[1]['FWHM'] = 1.12
        origin.setBackground(background=Function('FlatBackground', A0=1.0))
        origin.background[1].background.param['A0'] = 1.2
        s = origin.makeMultiSpectrumFunction()
        fun = FunctionFactory.createInitialized(s)

        self.assertEquals(fun.getParameterValue('f1.f0.A0'), 1.2)

        self.assertEquals(fun.getParameterValue('f0.f1.FWHM'), 1.11)
        self.assertEquals(fun.getParameterValue('f0.f2.FWHM'), 1.1)
        self.assertEquals(fun.getParameterValue('f0.f3.FWHM'), 1.1)

        self.assertEquals(fun.getParameterValue('f1.f1.FWHM'), 0.9)
        self.assertEquals(fun.getParameterValue('f1.f2.FWHM'), 1.12)
        self.assertEquals(fun.getParameterValue('f1.f3.FWHM'), 0.9)

        cf = CrystalField('Ce', 'C2v', B20=0.37737, B22=3.9770, B40=-0.031787, B42=-0.11611, B44=-0.12544,
                          Temperature=[44.0, 50], FWHM=[1.1, 0.9])
        cf.setPeaks('Lorentzian')
        cf.peaks[0].param[0]['FWHM'] = 1.11
        cf.peaks[1].param[1]['FWHM'] = 1.12
        cf.setBackground(background=Function('FlatBackground', A0=0.9))
        cf.ties(IntensityScaling0=1.0, IntensityScaling1=1.0)

        ws0 = makeWorkspace(*origin.getSpectrum(0))
        ws1 = makeWorkspace(*origin.getSpectrum(1))

        fit = CrystalFieldFit(cf, InputWorkspace=[ws0, ws1])
        fit.fit()

        self.assertAlmostEqual(cf.background[0].background.param['A0'], 1.0, 8)
        self.assertAlmostEqual(cf.background[1].background.param['A0'], 1.2, 8)

    def test_CrystalFieldFit_multi_spectrum_peak_background(self):
        from CrystalField.fitting import makeWorkspace
        from CrystalField import CrystalField, CrystalFieldFit, Background, Function
        from mantid.simpleapi import FunctionFactory
        origin = CrystalField('Ce', 'C2v', B20=0.37737, B22=3.9770, B40=-0.031787, B42=-0.11611, B44=-0.12544,
                              Temperature=[44.0, 50], FWHM=[1.1, 0.9])
        origin.setPeaks('Lorentzian')
        origin.peaks[0].param[0]['FWHM'] = 1.11
        origin.peaks[1].param[1]['FWHM'] = 1.12
        origin.setBackground(peak=Function('Gaussian', Height=10, Sigma=0.3))
        origin.background[1].peak.param['Sigma'] = 0.8
        s = origin.makeMultiSpectrumFunction()
        fun = FunctionFactory.createInitialized(s)

        self.assertEquals(fun.getParameterValue('f0.f0.Sigma'), 0.3)
        self.assertEquals(fun.getParameterValue('f1.f0.Sigma'), 0.8)

        self.assertEquals(fun.getParameterValue('f0.f1.FWHM'), 1.11)
        self.assertEquals(fun.getParameterValue('f0.f2.FWHM'), 1.1)
        self.assertEquals(fun.getParameterValue('f0.f3.FWHM'), 1.1)

        self.assertEquals(fun.getParameterValue('f1.f1.FWHM'), 0.9)
        self.assertEquals(fun.getParameterValue('f1.f2.FWHM'), 1.12)
        self.assertEquals(fun.getParameterValue('f1.f3.FWHM'), 0.9)

        cf = CrystalField('Ce', 'C2v', B20=0.37737, B22=3.9770, B40=-0.031787, B42=-0.11611, B44=-0.12544,
                          Temperature=[44.0, 50], FWHM=[1.1, 0.9])
        cf.setPeaks('Lorentzian')
        cf.peaks[0].param[0]['FWHM'] = 1.11
        cf.peaks[1].param[1]['FWHM'] = 1.12
        cf.setBackground(peak=Function('Gaussian', Height=10, Sigma=0.3))
        cf.ties(IntensityScaling0=1.0, IntensityScaling1=1.0)

        ws0 = makeWorkspace(*origin.getSpectrum(0))
        ws1 = makeWorkspace(*origin.getSpectrum(1))

        chi2 = CalculateChiSquared(cf.makeMultiSpectrumFunction(), InputWorkspace=ws0,  InputWorkspace_1=ws1)[1]

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

    def test_multi_ion_single_spectrum(self):
        from CrystalField.fitting import makeWorkspace
        from CrystalField import CrystalField, CrystalFieldFit
        from mantid.simpleapi import FunctionFactory
        params = {'B20':0.37737, 'B22':3.9770, 'B40':-0.031787, 'B42':-0.11611, 'B44':-0.12544,
                              'Temperature':44.0, 'FWHM':1.1}
        cf1 = CrystalField('Ce', 'C2v', **params)
        cf2 = CrystalField('Pr', 'C2v', **params)
        cf = cf1 + cf2
        x, y = cf.getSpectrum()
        ws = makeWorkspace(x, y)

        params = {'B20': 0.377, 'B22': 3.9, 'B40': -0.03, 'B42': -0.116, 'B44': -0.125,
                  'Temperature': 44.0, 'FWHM': 1.1}
        cf1 = CrystalField('Ce', 'C2v', **params)
        cf2 = CrystalField('Pr', 'C2v', **params)
        cf = cf1 + cf2

        chi2 = CalculateChiSquared(cf.makeSpectrumFunction(), InputWorkspace=ws)[1]

        fit = CrystalFieldFit(Model=cf, InputWorkspace=ws)
        fit.fit()

        self.assertTrue(cf.chi2 < chi2)

        # Fit outputs are different on different platforms.
        # The following assertions are not for testing but to illustrate
        # how to get the parameters.
        self.assertNotEqual(cf[0].param['B20'], 0.0)
        self.assertNotEqual(cf[0].param['B22'], 0.0)
        self.assertNotEqual(cf[0].param['B40'], 0.0)
        self.assertNotEqual(cf[0].param['B42'], 0.0)
        self.assertNotEqual(cf[0].param['B44'], 0.0)

        self.assertNotEqual(cf[1].param['B20'], 0.0)
        self.assertNotEqual(cf[1].param['B22'], 0.0)
        self.assertNotEqual(cf[1].param['B40'], 0.0)
        self.assertNotEqual(cf[1].param['B42'], 0.0)
        self.assertNotEqual(cf[1].param['B44'], 0.0)

    def test_multi_ion_multi_spectrum(self):
        from CrystalField.fitting import makeWorkspace
        from CrystalField import CrystalField, CrystalFieldFit
        from mantid.simpleapi import FunctionFactory
        params = {'B20': 0.37737, 'B22': 3.9770, 'B40': -0.031787, 'B42': -0.11611, 'B44': -0.12544,
                  'Temperature': [44.0, 50], 'FWHM': [1.1, 0.9]}
        cf1 = CrystalField('Ce', 'C2v', **params)
        cf2 = CrystalField('Pr', 'C2v', **params)
        cf = cf1 + cf2
        ws1 = makeWorkspace(*cf.getSpectrum(0))
        ws2 = makeWorkspace(*cf.getSpectrum(1))

        params = {'B20': 0.377, 'B22': 3.9, 'B40': -0.03, 'B42': -0.116, 'B44': -0.125,
                  'Temperature': [44.0, 50], 'FWHM': [1.1, 0.9]}
        cf1 = CrystalField('Ce', 'C2v', **params)
        cf1.ties(IntensityScaling0=1.0, IntensityScaling1=1.0)
        cf2 = CrystalField('Pr', 'C2v', **params)
        cf2.ties(IntensityScaling0=1.0, IntensityScaling1=1.0)
        cf = cf1 + cf2

        chi2 = CalculateChiSquared(cf.makeMultiSpectrumFunction(), InputWorkspace=ws1,  InputWorkspace_1=ws2)[1]

        fit = CrystalFieldFit(Model=cf, InputWorkspace=[ws1, ws2])
        fit.fit()

        self.assertTrue(cf.chi2 < chi2)

        # Fit outputs are different on different platforms.
        # The following assertions are not for testing but to illustrate
        # how to get the parameters.
        self.assertNotEqual(cf[0].param['B20'], 0.0)
        self.assertNotEqual(cf[0].param['B22'], 0.0)
        self.assertNotEqual(cf[0].param['B40'], 0.0)
        self.assertNotEqual(cf[0].param['B42'], 0.0)
        self.assertNotEqual(cf[0].param['B44'], 0.0)

        self.assertNotEqual(cf[1].param['B20'], 0.0)
        self.assertNotEqual(cf[1].param['B22'], 0.0)
        self.assertNotEqual(cf[1].param['B40'], 0.0)
        self.assertNotEqual(cf[1].param['B42'], 0.0)
        self.assertNotEqual(cf[1].param['B44'], 0.0)

    def test_constraints_single_spectrum(self):
        from CrystalField import CrystalField, CrystalFieldFit, Background, Function
        from mantid.simpleapi import FunctionFactory

        cf = CrystalField('Ce', 'C2v', B20=0.37737, B22=3.9770, B40=-0.031787, B42=-0.11611, B44=-0.12544,
                          Temperature=50, FWHM=0.9)
        cf.setPeaks('Lorentzian')
        cf.setBackground(peak=Function('Gaussian', Height=10, Sigma=0.3),
                         background=Function('LinearBackground', A0=1.0))

        cf.ties(B40='B20/2')
        cf.constraints('IntensityScaling > 0', 'B22 < 4')
        cf.peaks.constraints('f0.FWHM < 2.2', 'f1.FWHM >= 0.1')
        cf.peaks.ties('f2.FWHM=2*f1.FWHM', 'f3.FWHM=2*f2.FWHM')
        cf.background.peak.ties(Height=10.1)
        cf.background.peak.constraints('Sigma > 0')
        cf.background.background.ties(A0=0.1)
        cf.background.background.constraints('A1 > 0')

        s = cf.makeSpectrumFunction()
        self.assertTrue('IntensityScaling > 0' in s)
        self.assertTrue('B22 < 4' in s)
        self.assertTrue('f0.FWHM < 2.2' in s)
        self.assertTrue('f1.FWHM >= 0.1' in s)
        self.assertTrue('Sigma > 0' in s)
        self.assertTrue('A1 > 0' in s)
        self.assertTrue('f2.FWHM=2*f1.FWHM' in s)
        self.assertTrue('f3.FWHM=2*f2.FWHM' in s)
        self.assertTrue('Height=10.1' in s)
        self.assertTrue('A0=0.1' in s)

        # Test that ties and constraints are correctly defined
        fun = FunctionFactory.createInitialized(s)
        self.assertTrue(fun is not None)

    def test_all_peak_ties_single_spectrum(self):
        from CrystalField import CrystalField, CrystalFieldFit, Background, Function
        from mantid.simpleapi import FunctionFactory

        cf = CrystalField('Ce', 'C2v', B20=0.37737, B22=3.9770, B40=-0.031787, B42=-0.11611, B44=-0.12544,
                          Temperature=50, FWHM=0.9)
        cf.peaks.tieAll('FWHM=2.1', 3)

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
        self.assertTrue('0.1 < f0.FWHM <=2.1' in s)
        self.assertTrue('0.1 < f1.FWHM <=2.1' in s)
        self.assertTrue('0.1 < f2.FWHM <=2.1' in s)
        self.assertTrue('0.1 < f3.FWHM <=2.1' not in s)

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
        self.assertTrue('0.1 < f0.FWHM <=2.1' not in s)
        self.assertTrue('0.1 < f1.FWHM <=2.1' in s)
        self.assertTrue('0.1 < f2.FWHM <=2.1' in s)
        self.assertTrue('0.1 < f3.FWHM <=2.1' not in s)

        # Test that ties and constraints are correctly defined
        fun = FunctionFactory.createInitialized(s)
        self.assertTrue(fun is not None)

    def test_constraints_multi_spectrum(self):
        from CrystalField import CrystalField, CrystalFieldFit, Background, Function
        from mantid.simpleapi import FunctionFactory

        cf = CrystalField('Ce', 'C2v', B20=0.37737, B22=3.9770, B40=-0.031787, B42=-0.11611, B44=-0.12544,
                          Temperature=[44.0, 50], FWHM=[1.1, 0.9])
        cf.setPeaks('Lorentzian')
        cf.setBackground(peak=Function('Gaussian', Height=10, Sigma=0.3),
                         background=Function('FlatBackground', A0=1.0))
        cf.constraints('IntensityScaling0 > 0', '0 < IntensityScaling1 < 2', 'B22 < 4')
        cf.background[0].peak.ties(Height=10.1)
        cf.background[0].peak.constraints('Sigma > 0.1')
        cf.background[1].peak.ties(Height=20.2)
        cf.background[1].peak.constraints('Sigma > 0.2')

        cf.peaks[1].ties('f2.FWHM=2*f1.FWHM', 'f3.FWHM=2*f2.FWHM')
        cf.peaks[0].constraints('f1.FWHM < 2.2')
        cf.peaks[1].constraints('f1.FWHM > 1.1', '1 < f4.FWHM < 2.2')

        s = cf.makeMultiSpectrumFunction()

        self.assertTrue('IntensityScaling0 > 0' in s)
        self.assertTrue('IntensityScaling1 < 2' in s)
        self.assertTrue('f0.f0.f0.Height=10.1' in s)
        self.assertTrue('f1.f0.f0.Height=20.2' in s)
        self.assertTrue('f0.f0.f0.Sigma > 0.1' in s)
        self.assertTrue('f1.f0.f0.Sigma > 0.2' in s)
        self.assertTrue('f0.f1.FWHM < 2.2' in s)
        self.assertTrue('f1.f1.FWHM > 1.1' in s)
        self.assertTrue('1 < f1.f4.FWHM < 2.2' in s)
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
        self.assertTrue('0.1 < f0.f0.FWHM <=2.1' not in s)
        self.assertTrue('0.1 < f0.f1.FWHM <=2.1' in s)
        self.assertTrue('0.1 < f0.f2.FWHM <=2.1' in s)
        self.assertTrue('0.1 < f0.f4.FWHM <=2.1' not in s)

        self.assertTrue('f1.f2.FWHM > 12.1' not in s)
        self.assertTrue('f1.f3.FWHM > 12.1' in s)
        self.assertTrue('f1.f4.FWHM > 12.1' in s)
        self.assertTrue('f1.f5.FWHM > 12.1' in s)
        self.assertTrue('f1.f6.FWHM > 12.1' not in s)

        # Test that ties and constraints are correctly defined
        fun = FunctionFactory.createInitialized(s)
        self.assertTrue(fun is not None)

    def test_constraints_multi_ion_multi_spectrum(self):
        from CrystalField import CrystalField, CrystalFieldFit, Background, Function
        from CrystalField.fitting import makeWorkspace
        from mantid.simpleapi import FunctionFactory

        params = {'B20': 0.37737, 'B22': 3.9770, 'B40': -0.031787, 'B42': -0.11611, 'B44': -0.12544,
                  'Temperature': [44.0, 50], 'FWHM': [1.1, 0.9]}
        cf1 = CrystalField('Ce', 'C2v', **params)
        cf2 = CrystalField('Pr', 'C2v', **params)
        cf = cf1 + cf2
        ws1 = makeWorkspace(*cf.getSpectrum(0))
        ws2 = makeWorkspace(*cf.getSpectrum(1))

        params = {'B20': 0.377, 'B22': 3.9, 'B40': -0.03, 'B42': -0.116, 'B44': -0.125,
                  'Temperature': [44.0, 50], 'FWHM': [1.1, 0.9]}
        cf1 = CrystalField('Ce', 'C2v', **params)
        cf2 = CrystalField('Pr', 'C2v', **params)
        cf = cf1 + cf2

        cf1.setPeaks('Lorentzian')
        cf1.setBackground(peak=Function('Gaussian', Height=10, Sigma=0.3),
                         background=Function('FlatBackground', A0=1.0))
        cf1.constraints('IntensityScaling0 > 0', '0 < IntensityScaling1 < 2', 'B22 < 4')
        cf1.background[0].peak.ties(Height=10.1)
        cf1.background[0].peak.constraints('Sigma > 0.1')
        cf1.background[1].peak.ties(Height=20.2)
        cf1.background[1].peak.constraints('Sigma > 0.2')

        cf1.peaks[1].ties('f2.FWHM=2*f1.FWHM', 'f3.FWHM=2*f2.FWHM')
        cf1.peaks[0].constraints('f1.FWHM < 2.2')
        cf1.peaks[1].constraints('f1.FWHM > 1.1', '1 < f4.FWHM < 2.2')

        cf2.setPeaks('Gaussian')
        cf2.setBackground(peak=Function('Lorentzian', Amplitude=8, FWHM=0.33),
                         background=Function('FlatBackground', A0=1.0))
        cf2.background[0].peak.ties(Amplitude=8.1)
        cf2.background[0].peak.constraints('FWHM > 0.1')
        cf2.background[1].peak.ties(Amplitude=16.2)
        cf2.background[1].peak.constraints('FWHM > 0.2')
        cf2.peaks[1].ties('f2.Sigma=2*f1.Sigma', 'f3.Sigma=2*f2.Sigma')
        cf2.peaks[0].constraints('f1.Sigma < 2.2')
        cf2.peaks[1].constraints('f1.Sigma > 1.1', '1 < f4.Sigma < 2.2')

        s = cf.makeMultiSpectrumFunction()

        self.assertTrue('IntensityScaling0 > 0' in s)
        self.assertTrue('IntensityScaling1 < 2' in s)
        self.assertTrue('f0.f0.f0.Height=10.1' in s)
        self.assertTrue('f1.f0.f0.Height=20.2' in s)
        self.assertTrue('f0.f0.f0.Sigma > 0.1' in s)
        self.assertTrue('f1.f0.f0.Sigma > 0.2' in s)
        self.assertTrue('f0.f1.FWHM < 2.2' in s)
        self.assertTrue('f1.f1.FWHM > 1.1' in s)
        self.assertTrue('1 < f1.f4.FWHM < 2.2' in s)
        self.assertTrue('f1.f2.FWHM=2*f1.f1.FWHM' in s)
        self.assertTrue('f1.f3.FWHM=2*f1.f2.FWHM' in s)

        self.assertTrue('f0.f0.f0.Amplitude=8.1' in s)
        self.assertTrue('f1.f0.f0.Amplitude=16.2' in s)
        self.assertTrue('f0.f0.f0.FWHM > 0.1' in s)
        self.assertTrue('f1.f0.f0.FWHM > 0.2' in s)
        self.assertTrue('f1.f2.Sigma=2*f1.f1.Sigma' in s)
        self.assertTrue('f1.f3.Sigma=2*f1.f2.Sigma' in s)
        self.assertTrue('f0.f1.Sigma < 2.2' in s)
        self.assertTrue('f1.f1.Sigma > 1.1' in s)
        self.assertTrue('1 < f1.f4.Sigma < 2.2' in s)

        fun = FunctionFactory.createInitialized(s)

    def test_bad_input(self):
        from CrystalField import CrystalField
        from mantid.simpleapi import FunctionFactory

        cf = CrystalField('Ce', 'C2v', B20='aaa', B22=3.97, B40=-0.0317, B42=-0.116, B44=-0.12,
                      Temperature=44.0, FWHM=1.0)
        s = cf.makeSpectrumFunction()
        self.assertRaises(RuntimeError, FunctionFactory.createInitialized, s)

        cf = CrystalField('Ce', 'C2v', B20=1, B22=3.97, B40=[-0.0317], B42=-0.116, B44=-0.12,
                          Temperature=44.0, FWHM=1.0)
        s = cf.makeSpectrumFunction()
        self.assertRaises(RuntimeError, FunctionFactory.createInitialized, s)

        cf = CrystalField('Ce', 'C2v', B20=1, B22=3.97, B40=np.array([-0.0317]), B42=-0.116, B44=-0.12,
                          Temperature=44.0, FWHM=1.0)
        s = cf.makeSpectrumFunction()
        self.assertRaises(RuntimeError, FunctionFactory.createInitialized, s)

        cf = CrystalField('Ce', 'C2v', B20=1, B22=3.97, B40=np.array([1.2, 2.3]), B42=-0.116, B44=-0.12,
                          Temperature=44.0, FWHM=1.0)
        s = cf.makeSpectrumFunction()
        self.assertRaises(RuntimeError, FunctionFactory.createInitialized, s)

        cf = CrystalField('Ce', 'C2v', B20=1, B22=3.97, B40=-0.0317, B42=-0.116, B44=-0.12,
                          Temperature=44.0, FWHM=1.0)
        cf.peaks.param[1]["FWHM"] = 'aaa'
        s = cf.makeSpectrumFunction()
        self.assertRaises(RuntimeError, FunctionFactory.createInitialized, s)

if __name__ == "__main__":
    unittest.main()
