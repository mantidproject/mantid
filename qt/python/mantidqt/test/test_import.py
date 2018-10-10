# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function,
                        unicode_literals)

import unittest


class ImportTest(unittest.TestCase):

    def test_import(self):
        import mantidqt # noqa

if __name__ == "__main__":
    unittest.main()
