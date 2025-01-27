# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

import unittest
from unittest import mock

from mantidqtinterfaces.dns_powder_tof.data_structures.dns_obs_model import DNSObsModel
from mantidqtinterfaces.dns_powder_tof.script_generator.common_script_generator_model import DNSScriptGeneratorModel


class DNSScriptGeneratorModelTest(unittest.TestCase):
    parent = None

    @classmethod
    def setUpClass(cls):
        cls.parent = mock.Mock()
        cls.parent.update_progress = mock.Mock()
        # this is a backcall to the presenter
        cls.model = DNSScriptGeneratorModel(parent=cls.parent)

    def test___init__(self):
        self.assertIsInstance(self.model, DNSScriptGeneratorModel)
        self.assertIsInstance(self.model, DNSObsModel)

    def test_run_script(self):
        testv = self.model.run_script([])
        self.assertEqual(testv, "")
        with self.assertRaises(NameError):
            self.model.run_script(["x=y"])
        testv = self.model.run_script(
            [("from mantidqtinterfaces.dns_powder_tof.data_structures.dns_error import DNSError"), 'raise(DNSError("test"))']
        )
        self.assertEqual(testv, "test")

    def test_script_maker(self):
        testv = self.model.script_maker(None, None, None)
        self.assertEqual(testv, ([""], ""))

    def test_get_filename(self):
        self.assertEqual(self.model.get_filename("", []), "script.py")
        self.assertEqual(self.model.get_filename("txt", []), "txt.py")
        self.assertEqual(self.model.get_filename("a.py", []), "a.py")
        full_data_test = [{"sample_name": "_abc", "file_number": 4}, {"sample_name": "_abc", "file_number": 5}]
        self.assertEqual(self.model.get_filename("a.py", full_data_test, auto=True), "script_abc_from_4_to_5.py")


if __name__ == "__main__":
    unittest.main()
