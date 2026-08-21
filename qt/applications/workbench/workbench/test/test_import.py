# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#
#


import subprocess
import sys
import unittest


class ImportTest(unittest.TestCase):
    def test_import_workbench(self):
        import workbench  # noqa

    def test_importing_launcher_does_not_construct_workbench_config(self):
        result = subprocess.run(
            [
                sys.executable,
                "-c",
                "import sys; import workbench.app.start; "
                "assert 'workbench.config' not in sys.modules; "
                "assert 'workbench.widgets.about.presenter' not in sys.modules",
            ],
            capture_output=True,
            check=False,
            text=True,
        )

        self.assertEqual(0, result.returncode, msg=result.stderr)


if __name__ == "__main__":
    unittest.main()
