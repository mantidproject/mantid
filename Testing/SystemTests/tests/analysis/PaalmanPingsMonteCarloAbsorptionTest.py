# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import systemtesting
from mantid.simpleapi import PaalmanPingsMonteCarloAbsorption, Load, mtd, SaveNexus


class FlatPlateTest(systemtesting.MantidSystemTest):

    def __init__(self):
        super(FlatPlateTest, self).__init__()
        self.setUp()

    def setUp(self):
        Load(Filename='irs26176_graphite002_red.nxs', OutputWorkspace='sample')
        Load(Filename='irs26173_graphite002_red.nxs', OutputWorkspace='container')

    def cleanup(self):
        mtd.clear()

    def validate(self):
        self.tolerance = 1e-3
        self.tolerance_is_rel_err = True
        return ['flat_plate_corr', 'irs_PP_MC_flat_plate.nxs']

    def runTest(self):
        PaalmanPingsMonteCarloAbsorption(
            SampleWorkspace='sample',
            Shape='FlatPlate',
            BeamHeight=2.0,
            BeamWidth=2.0,
            Height=2.0,
            SampleWidth=2.0,
            SampleThickness=0.1,
            SampleChemicalFormula='H2-O',
            SampleDensity=1.0,
            ContainerWorkspace='container',
            ContainerFrontThickness=0.02,
            ContainerBackThickness=0.02,
            ContainerChemicalFormula='V',
            ContainerDensity=6.0,
            CorrectionsWorkspace='flat_plate_corr'
        )
        SaveNexus(InputWorkspace='flat_plate_corr', Filename='C:\\NewTestData\\CLAYTONirs_PP_MC_flat_plate.nxs')


class CylinderTest(systemtesting.MantidSystemTest):

    def __init__(self):
        super(CylinderTest, self).__init__()
        self.setUp()

    def setUp(self):
        Load(Filename='irs26176_graphite002_red.nxs', OutputWorkspace='sample')
        Load(Filename='irs26173_graphite002_red.nxs', OutputWorkspace='container')

    def cleanup(self):
        mtd.clear()

    def validate(self):
        self.tolerance = 1e-3
        self.tolerance_is_rel_err = True
        return ['cylinder_corr', 'irs_PP_MC_cylinder.nxs']

    def runTest(self):
        PaalmanPingsMonteCarloAbsorption(
            SampleWorkspace='sample',
            Shape='Cylinder',
            BeamHeight=2.0,
            BeamWidth=2.0,
            Height=2.0,
            SampleRadius=0.2,
            SampleChemicalFormula='H2-O',
            SampleDensity=1.0,
            ContainerWorkspace='container',
            ContainerRadius=0.22,
            ContainerChemicalFormula='V',
            ContainerDensity=6.0,
            CorrectionsWorkspace='cylinder_corr'
        )
        SaveNexus(InputWorkspace='cylinder_corr', Filename='C:\\NewTestData\\CLAYTONirs_PP_MC_cylinder.nxs')


class AnnulusTest(systemtesting.MantidSystemTest):

    def __init__(self):
        super(AnnulusTest, self).__init__()
        self.setUp()

    def setUp(self):
        Load(Filename='irs26176_graphite002_red.nxs', OutputWorkspace='sample')
        Load(Filename='irs26173_graphite002_red.nxs', OutputWorkspace='container')

    def cleanup(self):
        mtd.clear()

    def validate(self):
        self.tolerance = 1e-3
        self.tolerance_is_rel_err = True
        return ['annulus_corr', 'irs_PP_MC_annulus.nxs']

    def runTest(self):
        PaalmanPingsMonteCarloAbsorption(
            SampleWorkspace='sample',
            Shape='Annulus',
            BeamHeight=2.0,
            BeamWidth=2.0,
            Height=2.0,
            SampleInnerRadius=0.2,
            SampleOuterRadius=0.4,
            SampleChemicalFormula='H2-O',
            SampleDensity=1.0,
            ContainerWorkspace='container',
            ContainerInnerRadius=0.19,
            ContainerOuterRadius=0.41,
            ContainerChemicalFormula='V',
            ContainerDensity=6.0,
            CorrectionsWorkspace='annulus_corr',
            EventsPerPoint=5000
        )
        SaveNexus(InputWorkspace='annulus_corr', Filename='C:\\NewTestData\\CLAYTONirs_PP_MC_annulus.nxs')
