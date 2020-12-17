# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from mantid.kernel import PropertyManagerDataService
from mantid.simpleapi import Load, PDLoadCharacterizations, PDDetermineCharacterizations, DeleteWorkspaces
from mantid.utils import AbsorptionCorrUtils


class AbsorptionCorrUtilsTest(unittest.TestCase):

    def test_correction_props(self):
        self.assertRaises(RuntimeError, AbsorptionCorrUtils.create_absorption_input, '', None)

        charfile = "PG3_char_2020_01_04_PAC_limit_1.4MW.txt"
        charTable = PDLoadCharacterizations(Filename=charfile)
        char = charTable[0]

        data = Load(Filename="PG3_46577.nxs.h5", MetaDataOnly=True)

        PDDetermineCharacterizations(InputWorkspace=data,
                                     Characterizations=char,
                                     ReductionProperties="props")
        props = PropertyManagerDataService.retrieve("props")

        # Sample only absorption correction
        abs_sample = AbsorptionCorrUtils.calculate_absorption_correction("PG3_46577.nxs.h5", "SampleOnly", props, "Si",
                                                                         1.165, element_size=2)
        self.assertIsNotNone(abs_sample[0])

        DeleteWorkspaces([char, data, abs_sample[0]])
        PropertyManagerDataService.remove('props')

    def test_correction_methods(self):
        sample_ws, container_ws = AbsorptionCorrUtils.calculate_absorption_correction('', "None", None, "V", 1.0)

        self.assertIsNone(sample_ws)
        self.assertIsNone(container_ws)


if __name__ == '__main__':
    unittest.main()
