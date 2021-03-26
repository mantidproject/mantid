# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

import unittest


class TestAll(unittest.TestCase):
    @staticmethod
    def find_missing(package):
        missing = set(n for n in package.__all__ if getattr(package, n, None) is None)
        assert not missing, '__all__ contains unresolved names: {}'.format(', '.join(missing))

    def test_all(self):
        r"""
        Verify that the wild imports (e.g. __all__) don't define things that don't exist
        See http://xion.io/post/code/python-all-wild-imports.html for more information
        """
        import corelli.calibration
        self.find_missing(corelli.calibration)


if __name__ == '__main__':
    unittest.main()
