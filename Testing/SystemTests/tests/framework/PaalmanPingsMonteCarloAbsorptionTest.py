# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import systemtesting
from mantid.simpleapi import PaalmanPingsMonteCarloAbsorption, Load, mtd, SetSample


class FlatPlateTest(systemtesting.MantidSystemTest):
    def __init__(self):
        super(FlatPlateTest, self).__init__()
        self.setUp()

    def setUp(self):
        Load(Filename="irs26176_graphite002_red.nxs", OutputWorkspace="sample")

    def cleanup(self):
        mtd.clear()

    def validate(self):
        self.tolerance = 1e-3
        self.tolerance_is_rel_err = True
        return ["flat_plate_corr", "irs_PP_MC_flat_plate.nxs"]

    def runTest(self):
        PaalmanPingsMonteCarloAbsorption(
            InputWorkspace="sample",
            Shape="FlatPlate",
            BeamHeight=2.0,
            BeamWidth=2.0,
            Height=2.0,
            SampleWidth=2.0,
            SampleThickness=0.1,
            SampleChemicalFormula="H2-O",
            SampleDensity=1.0,
            ContainerFrontThickness=0.02,
            ContainerBackThickness=0.02,
            ContainerChemicalFormula="V",
            ContainerDensity=6.0,
            CorrectionsWorkspace="flat_plate_corr",
            EventsPerPoint=5000,
        )


class CylinderTest(systemtesting.MantidSystemTest):
    def __init__(self):
        super(CylinderTest, self).__init__()
        self.setUp()

    def setUp(self):
        Load(Filename="irs26176_graphite002_red.nxs", OutputWorkspace="sample")

    def cleanup(self):
        mtd.clear()

    def validate(self):
        self.tolerance = 1e-3
        self.tolerance_is_rel_err = True
        return ["cylinder_corr", "irs_PP_MC_cylinder.nxs"]

    def runTest(self):
        PaalmanPingsMonteCarloAbsorption(
            InputWorkspace="sample",
            Shape="Cylinder",
            BeamHeight=2.0,
            BeamWidth=2.0,
            Height=2.0,
            SampleRadius=0.2,
            SampleChemicalFormula="H2-O",
            SampleDensity=1.0,
            ContainerRadius=0.22,
            ContainerChemicalFormula="V",
            ContainerDensity=6.0,
            CorrectionsWorkspace="cylinder_corr",
            EventsPerPoint=5000,
        )


class AnnulusTest(systemtesting.MantidSystemTest):
    def __init__(self):
        super(AnnulusTest, self).__init__()
        self.setUp()

    def setUp(self):
        Load(Filename="irs26176_graphite002_red.nxs", OutputWorkspace="sample")

    def cleanup(self):
        mtd.clear()

    def validate(self):
        self.tolerance = 1e-3
        self.tolerance_is_rel_err = False
        return ["annulus_corr", "irs_PP_MC_annulus.nxs"]

    def runTest(self):
        PaalmanPingsMonteCarloAbsorption(
            InputWorkspace="sample",
            Shape="Annulus",
            BeamHeight=2.0,
            BeamWidth=2.0,
            Height=2.0,
            SampleInnerRadius=0.2,
            SampleOuterRadius=0.4,
            SampleChemicalFormula="H2-O",
            SampleDensity=1.0,
            ContainerInnerRadius=0.19,
            ContainerOuterRadius=0.41,
            ContainerChemicalFormula="V",
            ContainerDensity=6.0,
            CorrectionsWorkspace="annulus_corr",
            EventsPerPoint=10000,
        )


class PresetTest(systemtesting.MantidSystemTest):
    def __init__(self):
        super(PresetTest, self).__init__()
        self.setUp()

    def setUp(self):
        Load(Filename="irs26176_graphite002_red.nxs", OutputWorkspace="sample")
        SetSample(
            InputWorkspace="sample",
            Geometry={"Shape": "FlatPlate", "Height": 2.0, "Width": 2.0, "Thick": 0.1, "Center": [0.0, 0.0, 0.0]},
            Material={"ChemicalFormula": "H2-O", "MassDensity": 1.0},
            ContainerGeometry={
                "Shape": "FlatPlateHolder",
                "Height": 2.0,
                "Width": 2.0,
                "Thick": 0.1,
                "FrontThick": 0.02,
                "BackThick": 0.02,
                "Center": [0.0, 0.0, 0.0],
            },
            ContainerMaterial={"ChemicalFormula": "V", "MassDensity": 6.0},
        )

    def cleanup(self):
        mtd.clear()

    def validate(self):
        self.tolerance = 1e-3
        self.tolerance_is_rel_err = True
        return ["preset_corr", "irs_PP_MC_flat_plate.nxs"]

    def runTest(self):
        PaalmanPingsMonteCarloAbsorption(
            InputWorkspace="sample", Shape="Preset", BeamHeight=2.0, BeamWidth=2.0, CorrectionsWorkspace="preset_corr", EventsPerPoint=5000
        )
