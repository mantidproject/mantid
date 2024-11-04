# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init
import systemtesting
from mantid.simpleapi import CreateFloodWorkspace, ApplyFloodWorkspace, CreateWorkspace, SaveNexus, Load, Rebin, ConvertUnits, mtd
import os
import tempfile


class ReflectometryCreateFloodWorkspaceNoExclude(systemtesting.MantidSystemTest):
    flood_ws_name = "flood"

    def runTest(self):
        CreateFloodWorkspace("OFFSPEC00035946.nxs", StartSpectrum=265, EndSpectrum=500, OutputWorkspace=self.flood_ws_name)

    def validate(self):
        self.disableChecking.append("Instrument")
        return self.flood_ws_name, "ReflectometryCreateFloodWorkspaceNoExclude.nxs"


class ReflectometryCreateFloodWorkspaceExclude(systemtesting.MantidSystemTest):
    flood_ws_name = "flood"

    def runTest(self):
        CreateFloodWorkspace(
            "OFFSPEC00035946.nxs",
            StartSpectrum=250,
            EndSpectrum=600,
            ExcludeSpectra=[260, 261, 262, 516, 517, 518],
            OutputWorkspace=self.flood_ws_name,
        )

    def validate(self):
        self.disableChecking.append("Instrument")
        return self.flood_ws_name, "ReflectometryCreateFloodWorkspaceExclude.nxs"


class ReflectometryCreateFloodWorkspaceQuadratic(systemtesting.MantidSystemTest):
    flood_ws_name = "flood"

    def runTest(self):
        CreateFloodWorkspace(
            "OFFSPEC00035946.nxs",
            StartSpectrum=10,
            Background="Quadratic",
            ExcludeSpectra=[260, 261, 262, 516, 517, 518],
            OutputWorkspace=self.flood_ws_name,
        )

    def validate(self):
        self.disableChecking.append("Instrument")
        return self.flood_ws_name, "ReflectometryCreateFloodWorkspaceQuadratic.nxs"


class ReflectometryCreateFloodWorkspaceNegativeBackground(systemtesting.MantidSystemTest):
    flood_ws_name = "flood"

    def runTest(self):
        try:
            input_file = tempfile.gettempdir() + "/__refl_flood_cor_temp.nxs"
            x = [0, 100000, 0, 100000, 0, 100000, 0, 100000, 0, 100000, 0, 100000]
            y = [1, 9, 8, 3, 1, 1]
            ws = CreateWorkspace(x, y, NSpec=6)
            SaveNexus(ws, input_file)
            self.assertRaises(RuntimeError, CreateFloodWorkspace, input_file, Background="Quadratic", OutputWorkspace=self.flood_ws_name)
        finally:
            os.unlink(input_file)


class ReflectometryApplyFloodWorkspace(systemtesting.MantidSystemTest):
    out_ws_name = "out"

    def runTest(self):
        flood = CreateFloodWorkspace(
            "OFFSPEC00035946.nxs",
            StartSpectrum=250,
            EndSpectrum=600,
            ExcludeSpectra=[260, 261, 262, 516, 517, 518],
            OutputWorkspace="flood",
        )
        data = Load("OFFSPEC00044998.nxs", NumberOfBins=1)
        ApplyFloodWorkspace(InputWorkspace=data, FloodWorkspace=flood, OutputWorkspace=self.out_ws_name)

    def validate(self):
        self.disableChecking.append("Instrument")
        return self.out_ws_name, "ReflectometryApplyFloodWorkspace.nxs"


class ReflectometryApplyFloodWorkspaceRebinned(systemtesting.MantidSystemTest):
    out_ws_name = "out"

    def runTest(self):
        flood = CreateFloodWorkspace(
            "OFFSPEC00035946.nxs",
            StartSpectrum=250,
            EndSpectrum=600,
            ExcludeSpectra=[260, 261, 262, 516, 517, 518],
            OutputWorkspace="flood",
        )
        data = Load("OFFSPEC00044998.nxs")
        data = Rebin(data, [0, 1000, 100000], PreserveEvents=False)
        ApplyFloodWorkspace(InputWorkspace=data, FloodWorkspace=flood, OutputWorkspace=self.out_ws_name)

    def validate(self):
        self.disableChecking.append("Instrument")
        return self.out_ws_name, "ReflectometryApplyFloodWorkspaceRebinned.nxs"


class ReflectometryApplyFloodWorkspaceUnits(systemtesting.MantidSystemTest):
    out_ws_name = "out"

    def runTest(self):
        flood = CreateFloodWorkspace(
            "OFFSPEC00035946.nxs",
            StartSpectrum=250,
            EndSpectrum=600,
            ExcludeSpectra=[260, 261, 262, 516, 517, 518],
            OutputWorkspace="flood",
        )
        data = Load("OFFSPEC00044998.nxs")
        data = Rebin(data, [0, 1000, 100000], PreserveEvents=False)
        data = ConvertUnits(data, "Wavelength")
        ApplyFloodWorkspace(InputWorkspace=data, FloodWorkspace=flood, OutputWorkspace=self.out_ws_name)

    def validate(self):
        self.disableChecking.append("Instrument")
        return self.out_ws_name, "ReflectometryApplyFloodWorkspaceUnits.nxs"


class ReflectometryCreateFloodWorkspaceCentralPixel(systemtesting.MantidSystemTest):
    flood_ws_name = "flood"

    def runTest(self):
        try:
            input_file = tempfile.gettempdir() + "/__refl_flood_cor_temp.nxs"
            x = [0, 100000, 0, 100000, 0, 100000, 0, 100000, 0, 100000, 0, 100000]
            y = [1, 9, 8, 3, 14, 15]
            ws = CreateWorkspace(x, y, NSpec=6)
            SaveNexus(ws, input_file)
            CreateFloodWorkspace(input_file, CentralPixelSpectrum=3, OutputWorkspace=self.flood_ws_name)
            out = mtd[self.flood_ws_name]
            self.assertAlmostEqual(out.readY(0)[0], 1.0 / 8)
            self.assertAlmostEqual(out.readY(1)[0], 9.0 / 8)
            self.assertAlmostEqual(out.readY(2)[0], 1.0)
            self.assertAlmostEqual(out.readY(3)[0], 3.0 / 8)
            self.assertAlmostEqual(out.readY(4)[0], 14.0 / 8)
            self.assertAlmostEqual(out.readY(5)[0], 15.0 / 8)
        finally:
            os.unlink(input_file)


class ReflectometryCreateFloodWorkspaceIntegrationRange(systemtesting.MantidSystemTest):
    flood_ws_name = "flood"

    def runTest(self):
        try:
            input_file = tempfile.gettempdir() + "/__refl_flood_cor_temp.nxs"
            x = [0, 5, 6, 10] * 6
            y = [1, 2, 1] + [9, 3, 9] + [8, 4, 8] + [3, 5, 3] + [14, 6, 14] + [15, 7, 15]
            ws = CreateWorkspace(x, y, NSpec=6)
            SaveNexus(ws, input_file)
            CreateFloodWorkspace(input_file, CentralPixelSpectrum=3, RangeLower=3, RangeUpper=7, OutputWorkspace=self.flood_ws_name)
            out = mtd[self.flood_ws_name]
            self.assertAlmostEqual(out.readY(0)[0], 2.0 / 4)
            self.assertAlmostEqual(out.readY(1)[0], 3.0 / 4)
            self.assertAlmostEqual(out.readY(2)[0], 1.0)
            self.assertAlmostEqual(out.readY(3)[0], 5.0 / 4)
            self.assertAlmostEqual(out.readY(4)[0], 6.0 / 4)
            self.assertAlmostEqual(out.readY(5)[0], 7.0 / 4)
        finally:
            os.unlink(input_file)


class ReflectometryCreateFloodWorkspaceDivisionByZero(systemtesting.MantidSystemTest):
    flood_ws_name = "flood"

    def runTest(self):
        try:
            input_file = tempfile.gettempdir() + "/__refl_flood_cor_temp.nxs"
            x = [0, 5, 6, 10] * 6
            y = [1, 2, 1] + [9, 3, 9] + [8, 0, 8] + [3, 5, 3] + [14, 6, 14] + [15, 7, 15]
            ws = CreateWorkspace(x, y, NSpec=6)
            SaveNexus(ws, input_file)
            self.assertRaises(
                RuntimeError,
                CreateFloodWorkspace,
                input_file,
                CentralPixelSpectrum=3,
                RangeLower=3,
                RangeUpper=7,
                OutputWorkspace=self.flood_ws_name,
            )
        finally:
            os.unlink(input_file)


class ReflectometryCreateFloodWorkspaceCentralPixelExclude(systemtesting.MantidSystemTest):
    flood_ws_name = "flood"

    def runTest(self):
        try:
            input_file = tempfile.gettempdir() + "/__refl_flood_cor_temp.nxs"
            x = [0, 100000, 0, 100000, 0, 100000, 0, 100000, 0, 100000, 0, 100000]
            y = [1, 9, 8, 3, 14, 15]
            ws = CreateWorkspace(x, y, NSpec=6)
            SaveNexus(ws, input_file)
            CreateFloodWorkspace(input_file, CentralPixelSpectrum=3, ExcludeSpectra=[1, 5], OutputWorkspace=self.flood_ws_name)
            out = mtd[self.flood_ws_name]
            self.assertGreaterThan(out.readY(0)[0], 1.0e100)
            self.assertAlmostEqual(out.readY(1)[0], 9.0 / 8)
            self.assertAlmostEqual(out.readY(2)[0], 1.0)
            self.assertAlmostEqual(out.readY(3)[0], 3.0 / 8)
            self.assertGreaterThan(out.readY(4)[0], 1.0e100)
            self.assertAlmostEqual(out.readY(5)[0], 15.0 / 8)
        finally:
            os.unlink(input_file)


class ReflectometryCreateFloodWorkspaceCentralPixelRange(systemtesting.MantidSystemTest):
    flood_ws_name = "flood"

    def runTest(self):
        try:
            input_file = tempfile.gettempdir() + "/__refl_flood_cor_temp.nxs"
            x = [0, 100000, 0, 100000, 0, 100000, 0, 100000, 0, 100000, 0, 100000]
            y = [1, 9, 8, 3, 14, 15]
            ws = CreateWorkspace(x, y, NSpec=6)
            SaveNexus(ws, input_file)
            CreateFloodWorkspace(input_file, CentralPixelSpectrum=3, StartSpectrum=2, EndSpectrum=4, OutputWorkspace=self.flood_ws_name)
            out = mtd[self.flood_ws_name]
            self.assertAlmostEqual(out.readY(0)[0], 1.0)
            self.assertAlmostEqual(out.readY(1)[0], 9.0 / 8)
            self.assertAlmostEqual(out.readY(2)[0], 1.0)
            self.assertAlmostEqual(out.readY(3)[0], 3.0 / 8)
            self.assertAlmostEqual(out.readY(4)[0], 1.0)
            self.assertAlmostEqual(out.readY(5)[0], 1.0)
        finally:
            os.unlink(input_file)
