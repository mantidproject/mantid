# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from mantid.simpleapi import (
    AddSampleLog,
    ComputeIncoherentDOS,
    CreateSampleWorkspace,
    CreateWorkspace,
    LoadInstrument,
    ScaleX,
    Scale,
    SetInstrumentParameter,
    SetSampleMaterial,
    SofQW3,
    Transpose,
)
import numpy as np
from numpy import testing
from scipy import constants
import testhelpers


class ComputeIncoherentDOSTest(unittest.TestCase):
    def createPhononWS(self, T, en, e_units):
        fn = "name=Gaussian, PeakCentre=" + str(en) + ", Height=1, Sigma=0.5;"
        fn += "name=Gaussian, PeakCentre=-" + str(en) + ", Height=" + str(np.exp(-en * 11.6 / T)) + ", Sigma=0.5;"
        ws = CreateSampleWorkspace(binWidth=0.1, XMin=-25, XMax=25, XUnit=e_units, Function="User Defined", UserDefinedFunction=fn)
        LoadInstrument(ws, InstrumentName="MARI", RewriteSpectraMap=True)
        with self.assertRaisesRegex(
            RuntimeError,
            r"Input workspace must be in \(Q,E\) \[momentum and energy transfer\] or \(2theta, E\) \[scattering angle and energy transfer.\]",
        ):
            ws_DOS = ComputeIncoherentDOS(ws)
        ws = SofQW3(ws, [0, 0.05, 8], "Direct", 25)
        qq = np.arange(0, 8, 0.05) + 0.025
        for i in range(ws.getNumberHistograms()):
            ws.setY(i, ws.readY(i) * qq[i] ** 2)
            ws.setE(i, ws.readE(i) * qq[i] ** 2)
        return ws

    def compute(self, qs, energyBins, msd=0.0, temperature=300.0):
        ws = (energyBins[1:] + energyBins[:-1]) / 2.0
        if len(qs) > 1:
            qs = (qs[:-1] + qs[1:]) / 2.0
        g = qs**-2 * np.exp(msd * qs**2) * (1.0 - np.exp(-ws * constants.e * 1e-3 / constants.k / temperature)) * ws
        return g

    def computeFromTwoTheta(self, twoThetas, energyBins, msd=0.0, temperature=300.0):
        ws = (energyBins[1:] + energyBins[:-1]) / 2.0
        if len(twoThetas) > 1:
            twoThetas = (twoThetas[:-1] + twoThetas[1:]) / 2.0
        twoTheta = np.deg2rad(twoThetas)
        EFixed = 8.0
        Ei = EFixed * constants.e * 1e-3
        Ef = (EFixed - ws) * constants.e * 1e-3
        qs = np.sqrt(2.0 * constants.m_n / constants.hbar**2 * ((Ei + Ef) - 2.0 * np.sqrt(Ei * Ef) * np.cos(twoTheta))) * 1e-10
        g = qs**-2 * np.exp(msd * qs**2) * (1.0 - np.exp(-ws * constants.e * 1e-3 / constants.k / temperature)) * ws
        return g

    def convertToWavenumber(self, ws):
        mev2cm = (constants.elementary_charge / 1000) / (constants.h * constants.c * 100)
        u0 = ws.getAxis(0).getUnit().unitID()
        u1 = ws.getAxis(1).getUnit().unitID()
        if u0 == "DeltaE" or u1 == "DeltaE":
            if u0 == "MomentumTransfer":
                ws = Transpose(ws)
            ws.getAxis(0).setUnit("DeltaE_inWavenumber")
            ws = ScaleX(ws, mev2cm)
            ws = Scale(ws, 1 / mev2cm)
            if u0 == "MomentumTransfer":
                ws = Transpose(ws)
        return ws

    def unitySQWSingleHistogram(self, energyBins, qs):
        EFixed = 8.0
        Ys = np.ones(len(energyBins) - 1)
        Es = Ys
        verticalAxis = [str(q) for q in qs]
        ws = CreateWorkspace(
            energyBins, Ys, Es, UnitX="DeltaE", VerticalAxisUnit="MomentumTransfer", VerticalAxisValues=verticalAxis, StoreInADS=False
        )
        LoadInstrument(ws, InstrumentName="IN4", RewriteSpectraMap=False, StoreInADS=False)
        AddSampleLog(ws, LogName="Ei", LogText=str(EFixed), LogType="Number", LogUnit="meV", StoreInADS=False)
        return ws

    def unitySTwoThetaWSingleHistogram(self, energyBins, qs):
        EFixed = 8.0
        Ys = np.ones(len(energyBins) - 1)
        Es = Ys
        verticalAxis = [str(q) for q in qs]
        ws = CreateWorkspace(
            energyBins, Ys, Es, UnitX="DeltaE", VerticalAxisUnit="Degrees", VerticalAxisValues=verticalAxis, StoreInADS=False
        )
        LoadInstrument(ws, InstrumentName="IN4", RewriteSpectraMap=False, StoreInADS=False)
        AddSampleLog(ws, LogName="Ei", LogText=str(EFixed), LogType="Number", LogUnit="meV", StoreInADS=False)
        return ws

    def test_computeincoherentdos(self):
        ws = CreateSampleWorkspace()
        # Will fail unless the input workspace has Q and DeltaE axes.
        with self.assertRaisesRegex(
            RuntimeError,
            r"Input workspace must be in \(Q,E\) \[momentum and energy transfer\] or \(2theta, E\) \[scattering angle and energy transfer.\]",
        ):
            ws_DOS = ComputeIncoherentDOS(ws)
        ws = CreateSampleWorkspace(XUnit="DeltaE")
        with self.assertRaisesRegex(
            RuntimeError,
            r"Input workspace must be in \(Q,E\) \[momentum and energy transfer\] or \(2theta, E\) \[scattering angle and energy transfer.\]",
        ):
            ws_DOS = ComputeIncoherentDOS(ws)
        # Creates a workspace with two optic phonon modes at +E and -E with Q^2 dependence and population correct for T=300K
        ws = self.createPhononWS(300, 5, "DeltaE")
        # This should work!
        ws_DOS = ComputeIncoherentDOS(ws)
        self.assertEqual(ws_DOS.getAxis(0).getUnit().unitID(), "DeltaE")
        self.assertEqual(ws_DOS.getNumberHistograms(), 1)
        # Checks that it works if the workspace has |Q| along x instead of y, and converts energy to wavenumber
        ws = Transpose(ws)
        ws_DOS = ComputeIncoherentDOS(ws, Temperature=300, EnergyBinning="-20, 0.2, 20", Wavenumbers=True)
        self.assertEqual(ws_DOS.getAxis(0).getUnit().unitID(), "DeltaE_inWavenumber")
        self.assertEqual(ws_DOS.blocksize(), 200)
        # Checks that the Bose factor correction is ok.
        dos_eplus = np.max(ws_DOS.readY(0)[100:200])
        dos_eminus = np.max(ws_DOS.readY(0)[:100])
        self.assertAlmostEqual(dos_eplus / dos_eminus, 1.0, places=1)
        # Check that unit conversion from cm^-1 to meV works and also that conversion to states/meV is done
        ws = self.convertToWavenumber(ws)
        SetSampleMaterial(ws, "Al")
        ws_DOSn = ComputeIncoherentDOS(ws, EnergyBinning="-160, 1.6, 160", StatesPerEnergy=True)
        self.assertTrue("states" in ws_DOSn.YUnitLabel())
        self.assertEqual(ws_DOSn.getAxis(0).getUnit().unitID(), "DeltaE")
        material = ws.sample().getMaterial()
        factor = material.relativeMolecularMass() / (material.totalScatterXSection() * 1000) * 4 * np.pi
        self.assertAlmostEqual(np.max(ws_DOSn.readY(0)) / (np.max(ws_DOS.readY(0)) * factor), 1.0, places=1)

    def test_computation_nontransposed_QW(self):
        energyBins = np.arange(-7.0, 7.0, 0.13)
        qs = np.array([2.3])
        ws = self.unitySQWSingleHistogram(energyBins, qs)
        dos = ComputeIncoherentDOS(ws, EnergyBinning="Emin, Emax", StoreInADS=False)
        self.assertEqual(dos.getNumberHistograms(), 1)
        self.assertEqual(dos.getAxis(0).getUnit().unitID(), "DeltaE")
        dos_Xs = dos.readX(0)
        self.assertEqual(len(dos_Xs), len(energyBins))
        dos_Ys = dos.readY(0)
        dos_Es = dos.readE(0)
        g = self.compute(qs, energyBins)
        np.testing.assert_equal(dos_Xs, energyBins)
        for i in range(len(dos_Ys)):
            self.assertAlmostEqual(dos_Ys[i], g[i])
            self.assertAlmostEqual(dos_Es[i], g[i])

    def test_computation_nontransposed_TwoThetaW(self):
        energyBins = np.arange(-7.0, 7.0, 0.13)
        twoThetas = np.array([17.0])
        ws = self.unitySTwoThetaWSingleHistogram(energyBins, twoThetas)
        dos = ComputeIncoherentDOS(ws, EnergyBinning="Emin, Emax", StoreInADS=False)
        self.assertEqual(dos.getNumberHistograms(), 1)
        self.assertEqual(dos.getAxis(0).getUnit().unitID(), "DeltaE")
        dos_Xs = dos.readX(0)
        self.assertEqual(len(dos_Xs), len(energyBins))
        dos_Ys = dos.readY(0)
        dos_Es = dos.readE(0)
        g = self.computeFromTwoTheta(twoThetas, energyBins)
        np.testing.assert_equal(dos_Xs, energyBins)
        for i in range(len(dos_Ys)):
            self.assertAlmostEqual(dos_Ys[i], g[i])
            self.assertAlmostEqual(dos_Es[i], g[i])

    def test_computation_transposed_TwoThetaW(self):
        energyBins = np.arange(-7.0, 7.0, 0.13)
        twoThetas = np.array([16.5, 17.5])
        ws = self.unitySTwoThetaWSingleHistogram(energyBins, twoThetas)
        ws = Transpose(ws, StoreInADS=False)
        dos = ComputeIncoherentDOS(ws, EnergyBinning="Emin, Emax", StoreInADS=False)
        self.assertEqual(dos.getNumberHistograms(), 1)
        self.assertEqual(dos.getAxis(0).getUnit().unitID(), "DeltaE")
        dos_Xs = dos.readX(0)
        self.assertEqual(len(dos_Xs), len(energyBins))
        dos_Ys = dos.readY(0)
        dos_Es = dos.readE(0)
        g = self.computeFromTwoTheta(twoThetas, energyBins)
        np.testing.assert_equal(dos_Xs, energyBins)
        for i in range(len(dos_Ys)):
            self.assertAlmostEqual(dos_Ys[i], g[i])
            self.assertAlmostEqual(dos_Es[i], g[i])

    def test_computation_transposed_QW(self):
        energyBins = np.arange(-7.0, 7.0, 0.13)
        qs = np.array([2.15, 2.25])
        ws = self.unitySQWSingleHistogram(energyBins, qs)
        ws = Transpose(ws, StoreInADS=False)
        dos = ComputeIncoherentDOS(ws, EnergyBinning="Emin, Emax", StoreInADS=False)
        self.assertEqual(dos.getNumberHistograms(), 1)
        self.assertEqual(dos.getAxis(0).getUnit().unitID(), "DeltaE")
        dos_Xs = dos.readX(0)
        self.assertEqual(len(dos_Xs), len(energyBins))
        dos_Ys = dos.readY(0)
        dos_Es = dos.readE(0)
        g = self.compute(qs, energyBins)
        np.testing.assert_equal(dos_Xs, energyBins)
        for i in range(len(dos_Ys)):
            self.assertAlmostEqual(dos_Ys[i], g[i])
            self.assertAlmostEqual(dos_Es[i], g[i])

    def test_nonzero_MDS(self):
        energyBins = np.arange(-7.0, 7.0, 0.13)
        qs = np.array([2.3])
        msd = 5.5
        ws = self.unitySQWSingleHistogram(energyBins, qs)
        dos = ComputeIncoherentDOS(ws, MeanSquareDisplacement=msd, EnergyBinning="Emin, Emax", StoreInADS=False)
        self.assertEqual(dos.getNumberHistograms(), 1)
        self.assertEqual(dos.getAxis(0).getUnit().unitID(), "DeltaE")
        dos_Xs = dos.readX(0)
        self.assertEqual(len(dos_Xs), len(energyBins))
        dos_Ys = dos.readY(0)
        dos_Es = dos.readE(0)
        g = self.compute(qs, energyBins, msd=msd)
        np.testing.assert_equal(dos_Xs, energyBins)
        for i in range(len(dos_Ys)):
            self.assertAlmostEqual(dos_Ys[i], g[i], delta=g[i] * 1e-12)
            self.assertAlmostEqual(dos_Es[i], g[i], delta=g[i] * 1e-12)

    def test_nondefault_temperature(self):
        energyBins = np.arange(-7.0, 7.0, 0.13)
        qs = np.array([2.3])
        temperature = 666.7
        ws = self.unitySQWSingleHistogram(energyBins, qs)
        dos = ComputeIncoherentDOS(ws, Temperature=temperature, EnergyBinning="Emin, Emax", StoreInADS=False)
        self.assertEqual(dos.getNumberHistograms(), 1)
        self.assertEqual(dos.getAxis(0).getUnit().unitID(), "DeltaE")
        dos_Xs = dos.readX(0)
        self.assertEqual(len(dos_Xs), len(energyBins))
        dos_Ys = dos.readY(0)
        dos_Es = dos.readE(0)
        g = self.compute(qs, energyBins, temperature=temperature)
        np.testing.assert_equal(dos_Xs, energyBins)
        for i in range(len(dos_Ys)):
            self.assertAlmostEqual(dos_Ys[i], g[i])
            self.assertAlmostEqual(dos_Es[i], g[i])

    def test_multiple_histograms(self):
        energyBins = np.arange(-7.0, 7.0, 0.13)
        qs = np.array([1.1, 1.3, 1.5, 1.7])
        EFixed = 8.0
        Ys = np.ones(3 * (len(energyBins) - 1))
        Es = Ys
        verticalAxis = [str(q) for q in qs]
        ws = CreateWorkspace(
            energyBins,
            Ys,
            Es,
            NSpec=3,
            UnitX="DeltaE",
            VerticalAxisUnit="MomentumTransfer",
            VerticalAxisValues=verticalAxis,
            StoreInADS=False,
        )
        LoadInstrument(ws, InstrumentName="IN4", RewriteSpectraMap=False, StoreInADS=False)
        AddSampleLog(ws, LogName="Ei", LogText=str(EFixed), LogType="Number", LogUnit="meV", StoreInADS=False)
        dos = ComputeIncoherentDOS(ws, EnergyBinning="Emin, Emax", StoreInADS=False)
        self.assertEqual(dos.getNumberHistograms(), 1)
        self.assertEqual(dos.getAxis(0).getUnit().unitID(), "DeltaE")
        dos_Xs = dos.readX(0)
        self.assertEqual(len(dos_Xs), len(energyBins))
        dos_Ys = dos.readY(0)
        dos_Es = dos.readE(0)
        g1 = self.compute(qs[0:2], energyBins)
        g2 = self.compute(qs[1:3], energyBins)
        g3 = self.compute(qs[2:4], energyBins)
        g = (g1 + g2 + g3) / 3
        gE = np.sqrt(g1**2 + g2**2 + g3**2) / 3
        np.testing.assert_equal(dos_Xs, energyBins)
        for i in range(len(dos_Ys)):
            self.assertAlmostEqual(dos_Ys[i], g[i])
            self.assertAlmostEqual(dos_Es[i], gE[i])


if __name__ == "__main__":
    unittest.main()
