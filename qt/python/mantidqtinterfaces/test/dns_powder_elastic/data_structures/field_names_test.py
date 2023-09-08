# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

import unittest

from mantidqtinterfaces.dns_powder_elastic.data_structures.field_names import field_dict


class field_dictTest(unittest.TestCase):
    # field_dict is not a class just a dictionary

    def test_field_dict(self):
        self.assertIsInstance(field_dict, dict)
        self.assertEqual(len(field_dict), 24)
        self.assertEqual(field_dict["y7_nsf"], "y_nsf")


if __name__ == "__main__":
    unittest.main()
