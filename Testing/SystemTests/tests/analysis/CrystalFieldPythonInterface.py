# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#pylint: disable=no-init
from systemtesting import MantidSystemTest
from mantid.simpleapi import CreateWorkspace, LinearBackground, FlatBackground, Gaussian, CalculateChiSquared, mtd
from CrystalField.fitting import makeWorkspace
from PyChop import PyChop2
import numpy as np


class CrystalFieldPythonInterface(MantidSystemTest):
    """ Runs all the commands in the crystal field python interface documentation file
        This test only checks that the syntax of all the commands are still valid.
        Unit tests for the individual functions / fitting procedure check for correctness.
    """

    def my_create_ws(self, outwsname, x, y):
        jitter = (np.random.rand(np.shape(y)[0])-0.5)*np.max(y)/100
        CreateWorkspace(x, y + jitter, y*0+np.max(y)/100, Distribution=True, OutputWorkspace=outwsname)
        return mtd[outwsname]

    def my_func(self, en):
        return (25-en)**(1.5) / 200 + 0.1

    def runTest(self):
        from CrystalField import CrystalField, CrystalFieldFit, CrystalFieldMultiSite, Background, Function, ResolutionModel

        cf = CrystalField('Ce', 'C2v')
        cf = CrystalField('Ce', 'C2v', B20=0.37737, B22=3.9770)
        cf['B40'] = -0.031
        b = cf['B40']

        # Calculate and return the Hamiltonian matrix as a 2D numpy array.
        h = cf.getHamiltonian()
        print(h)
        # Calculate and return the eigenvalues of the Hamiltonian as a 1D numpy array.
        e = cf.getEigenvalues()
        print(e)
        # Calculate and return the eigenvectors of the Hamiltonian as a 2D numpy array.
        w = cf.getEigenvectors()
        print(w)
        # Using the keyword argument
        cf = CrystalField('Ce', 'C2v', B20=0.37737, B22=3.9770, Temperature=44)
        # Using the property
        cf.Temperature = 44

        print(cf.getPeakList())
        #[[  0.00000000e+00   2.44006198e+01   4.24977124e+01   1.80970926e+01 -2.44006198e+01]
        # [  2.16711565e+02   8.83098530e+01   5.04430056e+00   1.71153708e-01  1.41609425e-01]]
        cf.ToleranceIntensity = 1
        print(cf.getPeakList())
        #[[   0.           24.40061976   42.49771237]
        # [ 216.71156467   88.30985303    5.04430056]]
        cf.PeakShape = 'Gaussian'
        cf.FWHM = 0.9
        sp = cf.getSpectrum()
        print(cf.function)
        CrystalField_Ce = CreateWorkspace(*sp)
        print(CrystalField_Ce)

        # If the peak shape is Gaussian
        cf.peaks.param[1]['Sigma'] = 2.0
        cf.peaks.param[2]['Sigma'] = 0.01

        # If the peak shape is Lorentzian
        cf.PeakShape = 'Lorentzian'
        cf.peaks.param[1]['FWHM'] = 2.0
        cf.peaks.param[2]['FWHM'] = 0.01

        cf = CrystalField('Ce', 'C2v', B20=0.37737, B22=3.9770, B40=-0.031787, B42=-0.11611, B44=-0.12544,
                          Temperature=44.0, FWHM=1.1)
        cf.background = Background(peak=Function('Gaussian', Height=10, Sigma=1),
                                   background=Function('LinearBackground', A0=1.0, A1=0.01))
        h = cf.background.peak.param['Height']
        a1 = cf.background.background.param['A1']
        print(h)
        print(a1)

        cf.ties(B20=1.0, B40='B20/2')
        cf.constraints('1 < B22 <= 2', 'B22 < 4')

        print(cf.function[1].getTies())
        print(cf.function[1].getConstraints())

        cf.background.peak.ties(Height=10.1)
        cf.background.peak.constraints('Sigma > 0')
        cf.background.background.ties(A0=0.1)
        cf.background.background.constraints('A1 > 0')

        print(cf.function[0][0].getConstraints())
        print(cf.function[0][1].getConstraints())
        print(cf.function.getTies())
        print(cf.function.getConstraints())

        cf.peaks.ties({'f2.FWHM': '2*f1.FWHM', 'f3.FWHM': '2*f2.FWHM'})
        cf.peaks.constraints('f0.FWHM < 2.2', 'f1.FWHM >= 0.1')

        cf.PeakShape = 'Gaussian'
        cf.peaks.tieAll('Sigma=0.1', 3)
        cf.peaks.constrainAll('0 < Sigma < 0.1', 4)
        cf.peaks.tieAll('Sigma=f0.Sigma', 1, 3)
        cf.peaks.ties({'f1.Sigma': 'f0.Sigma', 'f2.Sigma': 'f0.Sigma', 'f3.Sigma': 'f0.Sigma'})

        rm = ResolutionModel(([1, 2, 3, 100], [0.1, 0.3, 0.35, 2.1]))
        cf = CrystalField('Ce', 'C2v', B20=0.37737, B22=3.9770, Temperature=44.0, ResolutionModel=rm)

        rm = ResolutionModel(self.my_func, xstart=0.0, xend=24.0, accuracy=0.01)
        cf = CrystalField('Ce', 'C2v', B20=0.37737, B22=3.9770, Temperature=44.0, ResolutionModel=rm)

        marires = PyChop2('MARI')
        marires.setChopper('S')
        marires.setFrequency(250)
        marires.setEi(30)
        rm = ResolutionModel(marires.getResolution, xstart=0.0, xend=29.0, accuracy=0.01)
        cf = CrystalField('Ce', 'C2v', B20=0.37737, B22=3.9770, Temperature=44.0, ResolutionModel=rm)

        cf = CrystalField('Ce', 'C2v', B20=0.37737, B22=3.9770, Temperature=44.0, ResolutionModel=rm, FWHMVariation=0.1)

        # ---------------------------

        cf = CrystalField('Ce', 'C2v', B20=0.37737, B22=3.9770, B40=-0.031787, B42=-0.11611, B44=-0.12544,
                          Temperature=[44.0, 50], FWHM=[1.1, 0.9])
        cf.PeakShape = 'Lorentzian'
        cf.peaks[0].param[0]['FWHM'] = 1.11
        cf.peaks[1].param[1]['FWHM'] = 1.12
        cf.background = Background(peak=Function('Gaussian', Height=10, Sigma=0.3),
                                   background=Function('FlatBackground', A0=1.0))
        cf.background[1].peak.param['Sigma'] = 0.8
        cf.background[1].background.param['A0'] = 1.1

        # The B parameters are common for all spectra - syntax doesn't change
        cf.ties(B20=1.0, B40='B20/2')
        cf.constraints('1 < B22 <= 2', 'B22 < 4')

        # Backgrounds and peaks are different for different spectra - must be indexed
        cf.background[0].peak.ties(Height=10.1)
        cf.background[0].peak.constraints('Sigma > 0.1')
        cf.background[1].peak.ties(Height=20.2)
        cf.background[1].peak.constraints('Sigma > 0.2')
        cf.peaks[1].tieAll('FWHM=2*f1.FWHM', 2, 5)
        cf.peaks[0].constrainAll('FWHM < 2.2', 1, 4)

        rm = ResolutionModel([self.my_func, marires.getResolution], 0, 100, accuracy = 0.01)
        cf.ResolutionModel = rm

        # Calculate second spectrum, use the generated x-values
        sp = cf.getSpectrum(1)
        # Calculate second spectrum, use the first spectrum of a workspace
        sp = cf.getSpectrum(1, 'CrystalField_Ce')
        # Calculate first spectrum, use the i-th spectrum of a workspace
        i=0
        sp = cf.getSpectrum(0, 'CrystalField_Ce', i)

        print(cf.function)

        cf.Temperature = [5, 50, 150]

        print()
        print(cf.function)

        ws = 'CrystalField_Ce'
        ws1 = 'CrystalField_Ce'
        ws2 = 'CrystalField_Ce'
        cf = CrystalField('Ce', 'C2v', B20=0.37737, B22=3.9770, B40=-0.031787, B42=-0.11611, B44=-0.12544, Temperature=5)

        # In case of a single spectrum (ws is a workspace)
        fit = CrystalFieldFit(Model=cf, InputWorkspace=ws)

        # Or for multiple spectra
        fit = CrystalFieldFit(Model=cf, InputWorkspace=[ws1, ws2])
        cf.Temperature = [5, 50]
        fit.fit()

        params = {'B20': 0.377, 'B22': 3.9, 'B40': -0.03, 'B42': -0.116, 'B44': -0.125,
                  'Temperature': [44.0, 50], 'FWHM': [1.1, 0.9]}
        cf1 = CrystalField('Ce', 'C2v', **params)
        cf2 = CrystalField('Pr', 'C2v', **params)
        cfms = cf1 + cf2
        cf = 2*cf1 + cf2

        cfms = CrystalFieldMultiSite(Ions=['Ce', 'Pr'], Symmetries=['C2v', 'C2v'], Temperatures=[44.0], FWHMs=[1.1])
        cfms['ion0.B40'] = -0.031
        cfms['ion1.B20'] = 0.37737
        b = cfms['ion0.B22']

        print(b)
        print(cfms.function)

        cfms = CrystalFieldMultiSite(Ions=['Ce', 'Pr'], Symmetries=['C2v', 'C2v'], Temperatures=[44.0], FWHMs=[1.1],
                                     parameters={'ion0.B20': 0.37737, 'ion0.B22': 3.9770, 'ion1.B40':-0.031787,
                                                 'ion1.B42':-0.11611, 'ion1.B44':-0.12544})
        cfms = CrystalFieldMultiSite(Ions='Ce', Symmetries='C2v', Temperatures=[20], FWHMs=[1.0],
                                     Background='name=Gaussian,Height=0,PeakCentre=1,Sigma=0;name=LinearBackground,A0=0,A1=0')
        cfms = CrystalFieldMultiSite(Ions=['Ce'], Symmetries=['C2v'], Temperatures=[50], FWHMs=[0.9],
                                     Background=LinearBackground(A0=1.0), BackgroundPeak=Gaussian(Height=10, Sigma=0.3))
        cfms = CrystalFieldMultiSite(Ions='Ce', Symmetries='C2v', Temperatures=[20], FWHMs=[1.0],
                                     Background= Gaussian(PeakCentre=1) + LinearBackground())
        cfms = CrystalFieldMultiSite(Ions=['Ce','Pr'], Symmetries=['C2v', 'C2v'], Temperatures=[44, 50], FWHMs=[1.1, 0.9],
                                     Background=FlatBackground(), BackgroundPeak=Gaussian(Height=10, Sigma=0.3),
                                     parameters={'ion0.B20': 0.37737, 'ion0.B22': 3.9770, 'ion1.B40':-0.031787,
                                                 'ion1.B42':-0.11611, 'ion1.B44':-0.12544})
        cfms.ties({'sp0.bg.f0.Height': 10.1})
        cfms.constraints('sp0.bg.f0.Sigma > 0.1')
        cfms.constraints('ion0.sp0.pk1.FWHM < 2.2')
        cfms.ties({'ion0.sp1.pk2.FWHM': '2*ion0.sp1.pk1.FWHM', 'ion1.sp1.pk3.FWHM': '2*ion1.sp1.pk2.FWHM'})

        # --------------------------

        params = {'ion0.B20': 0.37737, 'ion0.B22': 3.9770, 'ion1.B40':-0.031787, 'ion1.B42':-0.11611, 'ion1.B44':-0.12544}
        cf = CrystalFieldMultiSite(Ions=['Ce', 'Pr'], Symmetries=['C2v', 'C2v'], Temperatures=[44.0, 50.0],
                                   FWHMs=[1.0, 1.0], ToleranceIntensity=6.0, ToleranceEnergy=1.0,  FixAllPeaks=True,
                                   parameters=params)

        cf.fix('ion0.BmolX', 'ion0.BmolY', 'ion0.BmolZ', 'ion0.BextX', 'ion0.BextY', 'ion0.BextZ', 'ion0.B40',
               'ion0.B42', 'ion0.B44', 'ion0.B60', 'ion0.B62', 'ion0.B64', 'ion0.B66', 'ion0.IntensityScaling',
               'ion1.BmolX', 'ion1.BmolY', 'ion1.BmolZ', 'ion1.BextX', 'ion1.BextY', 'ion1.BextZ', 'ion1.B40',
               'ion1.B42', 'ion1.B44', 'ion1.B60', 'ion1.B62', 'ion1.B64', 'ion1.B66', 'ion1.IntensityScaling',
               'sp0.IntensityScaling', 'sp1.IntensityScaling')

        chi2 = CalculateChiSquared(str(cf.function), InputWorkspace=ws1, InputWorkspace_1=ws2)[1]

        fit = CrystalFieldFit(Model=cf, InputWorkspace=[ws1, ws2], MaxIterations=10)
        fit.fit()

        print(chi2)

        cfms = CrystalFieldMultiSite(Ions='Ce', Symmetries='C2', Temperatures=[25], FWHMs=[1.0], PeakShape='Gaussian',
                                     BmolX=1.0, B40=-0.02)
        print(str(cfms.function).split(',')[0])

        # --------------------------

        # Create some crystal field data
        origin = CrystalField('Ce', 'C2v', B20=0.37737, B22=3.9770, B40=-0.031787, B42=-0.11611, B44=-0.12544,
                              Temperature=44.0, FWHM=1.1)
        x, y = origin.getSpectrum()
        ws = makeWorkspace(x, y)

        # Define a CrystalField object with parameters slightly shifted.
        cf = CrystalField('Ce', 'C2v', B20=0, B22=0, B40=0, B42=0, B44=0,
                          Temperature=44.0, FWHM=1.0, ResolutionModel=([0, 100], [1, 1]), FWHMVariation=0)

        # Set any ties on the field parameters.
        cf.ties(B20=0.37737)
        # Create a fit object
        fit = CrystalFieldFit(cf, InputWorkspace=ws)
        # Find initial values for the field parameters.
        # You need to define the energy splitting and names of parameters to estimate.
        # Optionally additional constraints can be set on tied parameters (eg, peak centres).
        fit.estimate_parameters(EnergySplitting=50,
                                Parameters=['B22', 'B40', 'B42', 'B44'],
                                Constraints='20<f1.PeakCentre<45,20<f2.PeakCentre<45',
                                NSamples=1000)
        print('Returned', fit.get_number_estimates(), 'sets of parameters.')
        # The first set (the smallest chi squared) is selected by default.
        # Select a different parameter set if required
        fit.select_estimated_parameters(3)
        print(cf['B22'], cf['B40'], cf['B42'], cf['B44'])
        # Run fit
        fit.fit()

        # --------------------------

        from CrystalField import PointCharge
        axial_pc_model = PointCharge([[-2, 0, 0, -4], [-2, 0, 0, 4]], 'Nd')
        axial_blm = axial_pc_model.calculate()
        print(axial_blm)

        from CrystalField import PointCharge
        from mantid.geometry import CrystalStructure
        perovskite_structure = CrystalStructure('4 4 4 90 90 90', 'P m -3 m', 'Ce 0 0 0 1 0; Al 0.5 0.5 0.5 1 0; O 0.5 0.5 0 1 0')
        cubic_pc_model = PointCharge(perovskite_structure, 'Ce', Charges={'Ce':3, 'Al':3, 'O':-2}, MaxDistance=7.5)

        cubic_pc_model = PointCharge(perovskite_structure, 'Ce', Charges={'Ce':3, 'Al':3, 'O':-2}, Neighbour=2)
        print(cubic_pc_model)

        cif_pc_model = PointCharge('Sm2O3.cif')
        print(cif_pc_model.getIons())

        cif_pc_model.Charges = {'O1':-2, 'O2':-2, 'Sm1':3, 'Sm2':3, 'Sm3':3}
        cif_pc_model.IonLabel = 'Sm2'
        cif_pc_model.Neighbour = 1
        cif_blm = cif_pc_model.calculate()
        print(cif_blm)
        bad_pc_model = PointCharge('Sm2O3.cif', MaxDistance=7.5, Neighbour=2)
        print(bad_pc_model.Neighbour)
        print(bad_pc_model.MaxDistance)

        cif_pc_model.Charges = {'O':-2, 'Sm':3}
        cif_blm = cif_pc_model.calculate()
        print(cif_blm)

        cf = CrystalField('Sm', 'C2', Temperature=5, FWHM=10, **cif_pc_model.calculate())
        fit = CrystalFieldFit(cf, InputWorkspace=ws)

        fit = CrystalFieldFit(cf, InputWorkspace=ws)
        fit.fit()

        # --------------------------

        cf = CrystalField('Ce', 'C2v', B20=0.37737, B22=3.9770, Temperature=44.0)
        Cv = cf.getHeatCapacity()       # Calculates Cv(T) for 1<T<300K in 1K steps  (default)

        T = np.arange(1,900,5)
        Cv = cf.getHeatCapacity(T)      # Calculates Cv(T) for specified values of T (1 to 900K in 5K steps here)

        # Temperatures from a single spectrum workspace
        ws = CreateWorkspace(T, T, T)
        Cv = cf.getHeatCapacity(ws)     # Use the x-values of a workspace as the temperatures
        ws_cp = CreateWorkspace(*Cv)

        # Temperatures from a multi-spectrum workspace
        ws = CreateWorkspace(T, T, T, NSpec=2)
        Cv = cf.getHeatCapacity(ws, 1)  # Uses the second spectrum's x-values for T (e.g. 450<T<900)

        chi_v = cf.getSusceptibility(T, Hdir=[1, 1, 1])
        chi_v_powder = cf.getSusceptibility(T, Hdir='powder')
        chi_v_cgs = cf.getSusceptibility(T, Hdir=[1, 1, 0], Unit='SI')
        chi_v_bohr = cf.getSusceptibility(T, Unit='bohr')
        print(type([chi_v, chi_v_powder, chi_v_cgs, chi_v_bohr]))
        moment_t = cf.getMagneticMoment(Temperature=T, Hdir=[1, 1, 1], Hmag=0.1) # Calcs M(T) with at 0.1T field||[111]
        H = np.linspace(0, 30, 121)
        moment_h = cf.getMagneticMoment(Hmag=H, Hdir='powder', Temperature=10)   # Calcs M(H) at 10K for powder sample
        moment_SI = cf.getMagneticMoment(H, [1, 1, 1], Unit='SI')         # M(H) in Am^2/mol at 1K for H||[111]
        moment_cgs = cf.getMagneticMoment(100, Temperature=T, Unit='cgs') # M(T) in emu/mol in a field of 100G || [001]
        print(type([moment_t, moment_h, moment_SI, moment_cgs]))

        # --------------------------

        from CrystalField import CrystalField, CrystalFieldFit, PhysicalProperties
        # Fits a heat capacity dataset - you must have subtracted the phonon contribution by some method already
        # and the data must be in J/mol/K.
        cf = CrystalField('Ce', 'C2v', B20=0.37737, B22=3.9770, B40=-0.031787, B42=-0.11611, B44=-0.12544,
                          PhysicalProperty=PhysicalProperties('Cv'))
        fitcv = CrystalFieldFit(Model=cf, InputWorkspace=ws)
        fitcv.fit()

        params = {'B20':0.37737, 'B22':3.9770, 'B40':-0.031787, 'B42':-0.11611, 'B44':-0.12544}
        cf = CrystalField('Ce', 'C2v', **params)
        cf.PhysicalProperty = PhysicalProperties('Cv')
        fitcv = CrystalFieldFit(Model=cf, InputWorkspace=ws)
        fitcv.fit()

        # Fits a susceptibility dataset. Data is the volume susceptibility in SI units
        cf = CrystalField('Ce', 'C2v', **params)
        cf.PhysicalProperty = PhysicalProperties('susc', Hdir='powder', Unit='SI')
        fit_chi = CrystalFieldFit(Model=cf, InputWorkspace=ws)
        fit_chi.fit()

        # Fits a magnetisation dataset. Data is in emu/mol, and was measured at 5K with the field || [111].
        cf = CrystalField('Ce', 'C2v', **params)
        cf.PhysicalProperty = PhysicalProperties('M(H)', Temperature=5, Hdir=[1, 1, 1], Unit='cgs')
        fit_mag = CrystalFieldFit(Model=cf, InputWorkspace=ws)
        fit_mag.fit()

        # Fits a magnetisation vs temperature dataset. Data is in Am^2/mol, measured with a 0.1T field || [110]
        cf = CrystalField('Ce', 'C2v', **params)
        cf.PhysicalProperty = PhysicalProperties('M(T)', Hmag=0.1, Hdir=[1, 1, 0], Unit='SI')
        fit_moment = CrystalFieldFit(Model=cf, InputWorkspace=ws)
        fit_moment.fit()

        # --------------------------

        # Pregenerate the required workspaces
        for tt in [10, 44, 50]:
            cf = CrystalField('Ce', 'C2v', B20=0.37737, B22=3.9770, B40=-0.031787, B42=-0.11611, B44=-0.12544, Temperature=tt, FWHM=0.5)
            x, y = cf.getSpectrum()
            self.my_create_ws('ws_ins_'+str(tt)+'K', x, y)
        ws_ins_10K = mtd['ws_ins_10K']
        ws_ins_44K = mtd['ws_ins_44K']
        ws_ins_50K = mtd['ws_ins_50K']
        ws_cp = self.my_create_ws('ws_cp', *cf.getHeatCapacity())
        ws_chi = self.my_create_ws('ws_chi', *cf.getSusceptibility(np.linspace(1,300,100), Hdir='powder', Unit='cgs'))
        ws_mag = self.my_create_ws('ws_mag', *cf.getMagneticMoment(Hmag=np.linspace(0, 30, 100), Hdir=[1,1,1], Unit='bohr', Temperature=5))

        # --------------------------

        # Fits an INS spectrum (at 10K) and the heat capacity simultaneously
        cf = CrystalField('Ce', 'C2v', B20=0.37737, B22=3.9770, B40=-0.031787, B42=-0.11611, B44=-0.12544)
        cf.Temperature = 10
        cf.FWHM = 1.5
        cf.PhysicalProperty = PhysicalProperties('Cv')
        fit = CrystalFieldFit(Model=cf, InputWorkspace=[ws_ins_10K, ws_cp])
        fit.fit()

        # Fits two INS spectra (at 44K and 50K) and the heat capacity, susceptibility and magnetisation simultaneously.
        PPCv = PhysicalProperties('Cv')
        PPchi = PhysicalProperties('susc', 'powder', Unit='cgs')
        PPMag = PhysicalProperties('M(H)', [1, 1, 1], 5, 'bohr')
        cf = CrystalField('Ce', 'C2v', B20=0.37737, B22=3.9770, B40=-0.031787, B42=-0.11611, B44=-0.12544,
                          Temperature=[44.0, 50], FWHM=[1.1, 0.9], PhysicalProperty=[PPCv, PPchi, PPMag] )
        fit = CrystalFieldFit(Model=cf, InputWorkspace=[ws_ins_44K, ws_ins_50K, ws_cp, ws_chi, ws_mag])
        fit.fit()
