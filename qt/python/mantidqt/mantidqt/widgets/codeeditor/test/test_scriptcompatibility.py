# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#    This file is part of the mantid workbench.
#
#

import unittest

from unittest import mock
from mantidqt.widgets.codeeditor.scriptcompatibility import mantid_api_import_needed, mantid_algorithm_used_without_import

PERMISSION_BOX_FUNC = "mantidqt.widgets.codeeditor.scriptcompatibility.permission_box_to_prepend_import"


class ScriptCompatibilityTest(unittest.TestCase):
    def setUp(self):
        self.api_implicit_import = "from mantid.simpleapi import *"
        self.api_explicit_import = "from mantid import simpleapi"
        self.test_cases = [
            self.api_implicit_import + "\ntest string",
            self.api_implicit_import + "\nRebin()",
            "Rebin()",
            "test string",
            self.api_explicit_import + "\ntest string",
            self.api_explicit_import + "\nRebin()",
            self.api_explicit_import + "\nsimpleapi.Rebin()",
            self.api_implicit_import[:-1] + " Rebin" + "\nRebin()",
            "import mantid\nmantid.simpleapi.Rebin()",
        ]

    def gen_fail_msg(self, case, expected_out):
        return "Test case: \n  '{}' \nfailed with expected output:\n  '{}'".format(case, expected_out)

    def test_mantid_api_import_needed(self):
        expected_results = [False, False, True, False, False, True, False, False, False]
        with mock.patch(PERMISSION_BOX_FUNC, lambda: True):
            for case, expected_out in zip(self.test_cases, expected_results):
                self.assertEqual(expected_out, mantid_api_import_needed(case), msg=self.gen_fail_msg(case, expected_out))

    def test_mantid_algorithm_used_without_import(self):
        expected_results = [False, True, True, False, False, True, False, False, False]
        for case, expected_out in zip(self.test_cases, expected_results):
            self.assertEqual(expected_out, mantid_algorithm_used_without_import(case), msg=self.gen_fail_msg(case, expected_out))


if __name__ == "__main__":
    unittest.main()
