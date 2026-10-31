# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
import unittest
from unittest.mock import MagicMock

from mantidqt.widgets.tutorial.sandbox_base import InterfaceSandbox


class InterfaceSandboxTest(unittest.TestCase):
    def test_teardown_closes_the_view_and_cleans_up_only_once(self):
        sandbox = InterfaceSandbox(MagicMock(), MagicMock(), MagicMock())
        sandbox._cleanup = MagicMock()

        sandbox.teardown()
        sandbox.teardown()

        sandbox.view.close.assert_called_once_with()
        sandbox._cleanup.assert_called_once_with()


if __name__ == "__main__":
    unittest.main()
