# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import systemtesting
import numpy as np
from CrystalField import PointCharge


class LoadCIFDataWithTwoSectionsTest(systemtesting.MantidSystemTest):
    """
    Tests that the LoadCIF algorithm embedded in the PointCharge class will load cif data which contains two sections,
    and calculates the correct parameters from this data.
    """

    expected_parameter_values = [
        -1.324107e00,
        -4.179778e-01,
        7.644744e-01,
        8.222560e-04,
        -1.914738e-03,
        1.713388e-04,
        1.105470e-03,
        4.747298e-04,
        -3.211547e-07,
        -4.522218e-07,
        -1.456358e-07,
        2.610988e-07,
        -1.854308e-07,
        -1.315970e-06,
    ]

    def __init__(self):
        super(LoadCIFDataWithTwoSectionsTest, self).__init__()

    def requiredFiles(self):
        return ["DyAgGe20K.cif"]

    def runTest(self):
        cif_pc_model = PointCharge("DyAgGe20K.cif")

        cif_pc_model.Charges = {"Ge2": -3, "Ge1": -3, "Dy": 2.3, "Ag": 0.7}
        cif_pc_model.IonLabel = "Dy"
        cif_pc_model.Neighbour = 4
        cif_blm = cif_pc_model.calculate()

        np.testing.assert_allclose(list(cif_blm.values()), self.expected_parameter_values, atol=0.00001)
