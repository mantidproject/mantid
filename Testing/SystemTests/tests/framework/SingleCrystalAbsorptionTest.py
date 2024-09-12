# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import systemtesting
from mantid.simpleapi import (
    AddAbsorptionWeightedPathLengths,
    SetSample,
    FilterPeaks,
    ConvertPeaksWorkspace,
    CreatePeaksWorkspace,
    LoadNexus,
    AnalysisDataService,
)


class SingleCrystalPeaksAbsorptionTest(systemtesting.MantidSystemTest):
    def tearDown(self):
        AnalysisDataService.clear()

    def runTest(self):
        # peaks workspace
        peaks_orig = LoadNexus(Filename="SCDPeaks.nxs")
        peaks_corr = LoadNexus(Filename="SCDPeaksAbsorptionCorrection.nxs")  # generated using Mantid v6.8

        peaks_orig = FilterPeaks(InputWorkspace=peaks_orig, FilterVariable="RunNumber", FilterValue=80, Operator="=")
        peaks_corr = FilterPeaks(InputWorkspace=peaks_corr, FilterVariable="RunNumber", FilterValue=80, Operator="=")

        # oriented flat plate
        shape_dict = {
            "Shape": "CSG",
            "Value": '<cuboid id="cuboid">'
            + '<width val="0.001"/>'
            + '<height val="0.002"/>'
            + '<depth val="0.004"/>'
            + '<centre x="0.0" y="0.0" z="0.0"/>'
            + '<rotate x="-45.0" y="-90.0" z="-45.0"/>'
            + "</cuboid>"
            + '<algebra val="cuboid"/>',
        }
        mat_dict = {"ChemicalFormula": "V", "ZParameter": 2.0, "UnitCellVolume": 151.55444566227678}

        # rotate sample with goniometer
        R = peaks_orig.getPeak(0).getGoniometerMatrix()
        peaks_orig.run().getGoniometer().setR(R)

        # create lean peaks workspace without instrument
        peaks_lean_no_inst = CreatePeaksWorkspace(NumberOfPeaks=0)
        for peak_orig in peaks_orig:
            peaks_lean_no_inst.addPeak(peak_orig)
        peaks_lean_no_inst.run().getGoniometer().setR(R)

        SetSample(InputWorkspace=peaks_orig, Geometry=shape_dict, Material=mat_dict)
        SetSample(InputWorkspace=peaks_lean_no_inst, Geometry=shape_dict, Material=mat_dict)

        # convert lean peaks workspace
        peaks_lean = ConvertPeaksWorkspace(PeakWorkspace=peaks_orig)

        AddAbsorptionWeightedPathLengths(InputWorkspace=peaks_lean, ApplyCorrection=True)
        AddAbsorptionWeightedPathLengths(InputWorkspace=peaks_orig, ApplyCorrection=True)

        for peak_orig, peak_corr in zip(peaks_orig, peaks_corr):
            self.assertAlmostEqual(peak_orig.getIntensity(), peak_corr.getIntensity(), 1)
            self.assertAlmostEqual(peak_orig.getSigmaIntensity(), peak_corr.getSigmaIntensity(), 3)
            self.assertAlmostEqual(peak_orig.getAbsorptionWeightedPathLength(), peak_corr.getAbsorptionWeightedPathLength(), 3)

        for peak_lean, peak_corr in zip(peaks_lean, peaks_corr):
            self.assertAlmostEqual(peak_lean.getIntensity(), peak_corr.getIntensity(), 1)
            self.assertAlmostEqual(peak_lean.getSigmaIntensity(), peak_corr.getSigmaIntensity(), 3)
            self.assertAlmostEqual(peak_lean.getAbsorptionWeightedPathLength(), peak_corr.getAbsorptionWeightedPathLength(), 3)

        self.assertEqual(peaks_lean.getInstrument().getName(), "TOPAZ")

        # create lean peaks workspace without instrument
        AddAbsorptionWeightedPathLengths(InputWorkspace=peaks_lean_no_inst, ApplyCorrection=True)

        for peak_lean, peak_lean_no_inst in zip(peaks_lean, peaks_lean_no_inst):
            self.assertAlmostEqual(peak_lean.getIntensity(), peak_lean_no_inst.getIntensity(), 1)
            self.assertAlmostEqual(peak_lean.getSigmaIntensity(), peak_lean_no_inst.getSigmaIntensity(), 3)
            self.assertAlmostEqual(peak_lean.getAbsorptionWeightedPathLength(), peak_lean_no_inst.getAbsorptionWeightedPathLength(), 3)

        self.assertEqual(peaks_lean_no_inst.getInstrument().getName(), "")
