# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest


class ImportModuleTest(unittest.TestCase):
    def test_import_succeeds(self):
        import mantid

        attrs = dir(mantid)
        self.assertTrue("__version__" in attrs)

    def test_on_import_gui_flag_is_set_to_false_here(self):
        import mantid

        self.assertEqual(False, mantid.__gui__)


if __name__ == "__main__":
    unittest.main()
