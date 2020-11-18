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
    expected_parameter_values = [-1.004456, -0.488169, 0.579923, 0.000888, -0.001776, 0.000195, 0.001026, 0.000512,
                                 -5.418847e-7, -4.066401e-7, -1.459750e-7, 2.347822e-7, -3.138693e-7, -1.384160e-6]

    def __init__(self):
        super(LoadCIFDataWithTwoSectionsTest, self).__init__()

    def requiredFiles(self):
        return ['DyAgGe20K.cif']

    def runTest(self):

        cif_pc_model = PointCharge('DyAgGe20K.cif')

        cif_pc_model.Charges = {'Ge2': -3, 'Ge1': -3, 'Dy': 2.3, 'Ag': 0.7}
        cif_pc_model.IonLabel = 'Dy'
        cif_pc_model.Neighbour = 4
        cif_blm = cif_pc_model.calculate()

        np.testing.assert_allclose(list(cif_blm.values()), self.expected_parameter_values, atol=0.00001)
