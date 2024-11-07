# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
import os
import shutil
import tempfile
from mantid.simpleapi import TotScatCalculateSelfScattering
from mantid.api import mtd
from isis_powder import SampleDetails
from testhelpers import WorkspaceCreationHelper


class TotScatCalculateSelfScatteringTest(unittest.TestCase):
    def setUp(self):
        sample_details = SampleDetails(height=4.0, radius=0.2985, center=[0, 0, 0], shape="cylinder")
        sample_details.set_material(chemical_formula="Si")
        self.geometry = {
            "Shape": "Cylinder",
            "Height": sample_details.height(),
            "Radius": sample_details.radius(),
            "Center": sample_details.center(),
        }

        material = sample_details.material_object
        material_json = {"ChemicalFormula": material.chemical_formula}
        if material.number_density:
            material_json["SampleNumberDensity"] = material.number_density
        if material.absorption_cross_section:
            material_json["AttenuationXSection"] = material.absorption_cross_section
        if material.scattering_cross_section:
            material_json["ScatteringXSection"] = material.scattering_cross_section
        self.material = material_json

        self.dirpath = tempfile.mkdtemp()
        self.cal_file_path = os.path.join(self.dirpath, "tot_scat.cal")
        file = open(self.cal_file_path, "w")
        file.write("%i\t%i\t%f\t%i\t%i\n" % (1, 1, 0.0, 1, 1))
        file.write("%i\t%i\t%f\t%i\t%i\n" % (2, 2, 0.0, 1, 1))
        file.write("%i\t%i\t%f\t%i\t%i\n" % (3, 3, 0.0, 1, 2))
        file.write("%i\t%i\t%f\t%i\t%i\n" % (4, 4, 0.0, 1, 2))
        file.close()

    def tearDown(self):
        try:
            os.remove(self.cal_file_path)
            shutil.rmtree(self.dirpath)
        except shutil.Error:
            pass

    def test_TotScatCalculateSelfScattering_executes(self):
        raw_ws = WorkspaceCreationHelper.create2DWorkspaceWithFullInstrument(6, 100, True)
        mtd.addOrReplace("tot_scat_test", raw_ws)
        correction_ws = TotScatCalculateSelfScattering(
            InputWorkspace="tot_scat_test", CalFileName=self.cal_file_path, SampleGeometry=self.geometry, SampleMaterial=self.material
        )
        self.assertEqual(correction_ws.getNumberHistograms(), 2)

    def test_TotScatCalculateSelfScattering_placzek_2(self):
        raw_ws = WorkspaceCreationHelper.create2DWorkspaceWithFullInstrument(6, 100, True)
        mtd.addOrReplace("tot_scat_test", raw_ws)

        correction_ws = TotScatCalculateSelfScattering(
            InputWorkspace="tot_scat_test",
            CalFileName=self.cal_file_path,
            SampleGeometry=self.geometry,
            SampleMaterial=self.material,
            PlaczekOrder=2,
            SampleTemp="293",
        )
        self.assertEqual(correction_ws.getNumberHistograms(), 2)

    # fails if no temperature provided
    def test_TotScatCalculateSelfScattering_placzek_2_fail(self):
        raw_ws = WorkspaceCreationHelper.create2DWorkspaceWithFullInstrument(6, 100, True)
        mtd.addOrReplace("tot_scat_test", raw_ws)

        with self.assertRaisesRegex(RuntimeError, "SampleTemperature must be provided"):
            TotScatCalculateSelfScattering(
                InputWorkspace="tot_scat_test",
                OutputWorkspace="correction_ws",
                CalFileName=self.cal_file_path,
                SampleGeometry=self.geometry,
                SampleMaterial=self.material,
                PlaczekOrder=2,
            )


if __name__ == "__main__":
    unittest.main()
