# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.simpleapi import Load, PaalmanPingsMonteCarloAbsorption, mtd, SetSample
from mantid.api import WorkspaceGroup
import unittest


class PaalmanPingsMonteCarloAbsorptionTest(unittest.TestCase):
    _red_ws = None
    _indirect_elastic_ws = None
    _indirect_fws_ws = None
    _geoms_ws = None

    @classmethod
    def setUpClass(cls):
        Load("irs26176_graphite002_red.nxs", OutputWorkspace="red_ws")
        Load("osi104367_elf.nxs", OutputWorkspace="indirect_elastic_ws")
        Load("ILL_IN16B_FWS_Reduced.nxs", OutputWorkspace="indirect_fws_ws")
        Load("HRP38692a.nxs", OutputWorkspace="elastic_ws")
        Load("MAR21335_Ei60meV.nxs", OutputWorkspace="direct_ws")
        Load("irs26176_graphite002_red.nxs", OutputWorkspace="geoms_ws")
        # set this workspace to have defined sample and can geometries to test the preset option
        SetSample(
            "geoms_ws",
            Geometry={"Shape": "Cylinder", "Height": 4.0, "Radius": 2.0, "Center": [0.0, 0.0, 0.0]},
            Material={"ChemicalFormula": "Ni"},
            ContainerGeometry={"Shape": "HollowCylinder", "Height": 4.0, "InnerRadius": 2.0, "OuterRadius": 3.5},
        )

    def setUp(self):
        self._red_ws = mtd["red_ws"]
        self._indirect_elastic_ws = mtd["indirect_elastic_ws"]
        self._indirect_fws_ws = mtd["indirect_fws_ws"]
        self._elastic_ws = mtd["elastic_ws"]
        self._direct_ws = mtd["direct_ws"]
        self._geoms_ws = mtd["geoms_ws"]

        self._expected_unit = self._red_ws.getAxis(0).getUnit().unitID()
        self._expected_hist = 10
        self._expected_blocksize = 1905

        self._arguments = {
            "SampleChemicalFormula": "H2-O",
            "SampleDensityType": "Mass Density",
            "SampleDensity": 1.0,
            "EventsPerPoint": 200,
            "BeamHeight": 3.5,
            "BeamWidth": 4.0,
            "Height": 2.0,
        }

        self._container_args = {"ContainerChemicalFormula": "Al", "ContainerDensityType": "Mass Density", "ContainerDensity": 1.0}
        self._test_arguments = dict()

    @classmethod
    def tearDownClass(cls):
        mtd.clear()

    def _flat_plate_test(self, test_func, sparse=False):
        self._test_arguments["SampleWidth"] = 2.0
        self._test_arguments["SampleThickness"] = 2.0
        self._test_arguments["SampleCenter"] = 1.0
        self._test_arguments["SampleAngle"] = -10.0
        self._test_arguments["SparseInstrument"] = sparse
        test_func("FlatPlate")

    def _cylinder_test(self, test_func, sparse=False):
        self._test_arguments["SampleRadius"] = 0.5
        self._test_arguments["SparseInstrument"] = sparse
        test_func("Cylinder")

    def _annulus_test(self, test_func, sparse=False):
        self._test_arguments["SampleInnerRadius"] = 1.2
        self._test_arguments["SampleOuterRadius"] = 1.8
        self._test_arguments["SparseInstrument"] = sparse
        test_func("Annulus")

    def _preset_with_override_material_test(self, test_func):
        test_func(shape="Preset", sample_ws=self._geoms_ws)

    def _preset_without_override_material_test(self, test_func):
        self._arguments = {"EventsPerPoint": 200, "BeamHeight": 3.5, "BeamWidth": 4.0}
        test_func(shape="Preset", sample_ws=self._geoms_ws)

    def _material_with_cross_section_test(self, test_func):
        self._test_arguments["SampleWidth"] = 2.0
        self._test_arguments["SampleThickness"] = 2.0
        self._test_arguments["SampleChemicalFormula"] = ""
        self._test_arguments["SampleIncoherentXSection"] = 5.7
        self._test_arguments["SampleDensityType"] = "Number Density"
        test_func("FlatPlate")

    def _setup_flat_plate_container(self):
        self._test_arguments["ContainerFrontThickness"] = 1.5
        self._test_arguments["ContainerBackThickness"] = 1.5

    def _setup_cylinder_container(self):
        self._test_arguments["ContainerRadius"] = 1.0

    def _setup_annulus_container(self):
        self._test_arguments["ContainerInnerRadius"] = 1.0
        self._test_arguments["ContainerOuterRadius"] = 2.0

    def _test_corrections_workspace(self, corr_ws, spectrum_axis=None):
        x_unit = corr_ws.getAxis(0).getUnit().unitID()
        self.assertEqual(x_unit, self._expected_unit)

        y_unit = corr_ws.YUnitLabel()
        self.assertEqual(y_unit, "Attenuation factor")

        num_hists = corr_ws.getNumberHistograms()
        self.assertEqual(num_hists, self._expected_hist)

        blocksize = corr_ws.blocksize()
        self.assertEqual(blocksize, self._expected_blocksize)

        if spectrum_axis:
            self.assertEqual(corr_ws.getAxis(1).getUnit().unitID(), spectrum_axis)

    def _test_corrections_workspaces(self, workspaces, spectrum_axis=None, with_container=False):
        self.assertNotEqual(workspaces, None)
        self.assertTrue(isinstance(workspaces, WorkspaceGroup))
        self.assertEqual(workspaces.getNumberOfEntries(), 4 if with_container else 1)

        for workspace in workspaces:
            self._test_corrections_workspace(workspace, spectrum_axis)

    def _run_correction_and_test(self, shape, sample_ws=None, spectrum_axis=None, with_container=False):
        if sample_ws is None:
            sample_ws = self._red_ws

        arguments = self._arguments.copy()
        arguments.update(self._test_arguments)
        corrected = PaalmanPingsMonteCarloAbsorption(InputWorkspace=sample_ws, Shape=shape, **arguments)
        self._test_corrections_workspaces(corrected, spectrum_axis, with_container)

    def _run_correction_with_container_test(self, shape, sample_ws=None):
        self._test_arguments.update(self._container_args)

        if shape == "FlatPlate":
            self._setup_flat_plate_container()
        elif shape == "Cylinder":
            self._setup_cylinder_container()
        elif shape == "Annulus":
            self._setup_annulus_container()

        self._run_correction_and_test(shape=shape, sample_ws=sample_ws, with_container=True)

    def _run_indirect_elastic_test(self, shape):
        self._expected_unit = "MomentumTransfer"
        self._expected_hist = 17
        self._expected_blocksize = 1
        self._run_correction_and_test(shape, self._indirect_elastic_ws, "Label")

    def _run_indirect_fws_test_red(self, shape):
        self._expected_unit = "Wavelength"
        self._expected_hist = 18
        self._expected_blocksize = 1
        self._run_correction_and_test(shape, self._indirect_fws_ws.getItem(2), "Label")

    def _run_indirect_fws_test_q(self, shape):
        self._expected_unit = "Wavelength"
        self._expected_hist = 18
        self._expected_blocksize = 1
        self._run_correction_and_test(shape, self._indirect_fws_ws.getItem(1), "MomentumTransfer")

    def _run_indirect_fws_test_2theta(self, shape):
        self._expected_unit = "Wavelength"
        self._expected_hist = 18
        self._expected_blocksize = 1
        self._run_correction_and_test(shape, self._indirect_fws_ws.getItem(0), "Degrees")

    def _run_elastic_test(self, shape):
        self._expected_unit = "TOF"
        self._expected_hist = 11
        self._expected_blocksize = 23987
        self._run_correction_and_test(shape, self._elastic_ws, "Label")

    def _run_direct_test(self, shape):
        self._expected_unit = "DeltaE"
        self._expected_hist = 285
        self._expected_blocksize = 294
        self._run_correction_and_test(shape, self._direct_ws, "Label")

    def test_material_with_cross_section(self):
        self._material_with_cross_section_test(self._run_correction_and_test)

    def test_flat_plate_no_container(self):
        self._flat_plate_test(self._run_correction_and_test)

    def test_cylinder_no_container(self):
        self._cylinder_test(self._run_correction_and_test)

    def test_annulus_no_container(self):
        self._annulus_test(self._run_correction_and_test)

    def test_preset_with_override_material(self):
        self._preset_with_override_material_test(self._run_correction_with_container_test)

    def test_preset_without_overriding_material(self):
        self._preset_without_override_material_test(self._run_correction_with_container_test)

    def test_flat_plate_with_container(self):
        self._flat_plate_test(self._run_correction_with_container_test)

    def test_flat_plate_with_container_sparse(self):
        self._flat_plate_test(self._run_correction_with_container_test, True)

    def test_cylinder_with_container(self):
        self._cylinder_test(self._run_correction_with_container_test)

    def test_cylinder_with_container_sparse(self):
        self._cylinder_test(self._run_correction_with_container_test, True)

    def test_annulus_with_container(self):
        self._annulus_test(self._run_correction_with_container_test)

    def test_annulus_with_container_sparse(self):
        self._annulus_test(self._run_correction_with_container_test, True)

    def test_flat_plate_indirect_elastic(self):
        self._flat_plate_test(self._run_indirect_elastic_test)

    def test_cylinder_indirect_elastic(self):
        self._cylinder_test(self._run_indirect_elastic_test)

    def test_annulus_indirect_elastic(self):
        self._annulus_test(self._run_indirect_elastic_test)

    def test_flat_plate_indirect_fws(self):
        self._flat_plate_test(self._run_indirect_fws_test_2theta)

    def test_cylinder_indirect_fws(self):
        self._cylinder_test(self._run_indirect_fws_test_q)

    def test_annulus_indirect_fws(self):
        self._annulus_test(self._run_indirect_fws_test_red)

    def test_cylinder_elastic(self):
        self._cylinder_test(self._run_elastic_test)

    def test_cylinder_direct(self):
        self._cylinder_test(self._run_direct_test)


if __name__ == "__main__":
    unittest.main()
