# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

import unittest
from mantid.simpleapi import TotScatCalculateSelfScattering, Load
from isis_powder import SampleDetails


class TotScatCalculateSelfScatteringTest(unittest.TestCase):

    def setUp(self):
        sample_details = SampleDetails(height=4.0, radius=0.2985, center=[0, 0, 0], shape='cylinder')
        sample_details.set_material(chemical_formula='Si')
        self.geometry = {'Shape': 'Cylinder',
                         'Height': sample_details.height(),
                         'Radius': sample_details.radius(),
                         'Center': sample_details.center()}

        material = sample_details.material_object
        material_json = {'ChemicalFormula': material.chemical_formula}
        if material.number_density:
            material_json["SampleNumberDensity"] = material.number_density
        if material.absorption_cross_section:
            material_json["AttenuationXSection"] = material.absorption_cross_section
        if material.scattering_cross_section:
            material_json["ScatteringXSection"] = material.scattering_cross_section
        self.material = material_json

        self.cal_file_path = "polaris_grouping_file.cal"

    def test_TotScatCalculateSelfScattering_executes(self):
        raw_ws = Load(Filename='POLARIS98533.nxs')
        correction_ws = TotScatCalculateSelfScattering(RawWorkspace=raw_ws,
                                                       CalFileName=self.cal_file_path,
                                                       SampleGeometry=self.geometry,
                                                       SampleMaterial=self.material)
        self.assertEqual(correction_ws.getNumberHistograms(), 5)


if __name__ == "__main__":
    unittest.main()
