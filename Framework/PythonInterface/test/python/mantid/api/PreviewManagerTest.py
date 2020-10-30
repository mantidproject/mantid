# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from mantid.api import PreviewManager


class PreviewManagerTest(unittest.TestCase):

    def test_instance(self):
        instance = PreviewManager.Instance()
        self.assertTrue(isinstance(instance, PreviewManager))
        self.assertTrue(hasattr(instance, 'getPreview'))
        self.assertTrue(callable(getattr(instance, 'getPreview')))
        self.assertTrue(hasattr(instance, 'getPreviews'))
        self.assertTrue(callable(getattr(instance, 'getPreviews')))

if __name__ == '__main__':
    unittest.main()
