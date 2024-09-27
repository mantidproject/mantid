# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from mantid.api import AlgorithmManager, FrameworkManager
from sans_core.common.log_tagger import has_tag, set_tag, get_tag, has_hash, set_hash, get_hash_value


class SANSLogTaggerTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        FrameworkManager.Instance()

    @staticmethod
    def _provide_sample_workspace():
        alg = AlgorithmManager.createUnmanaged("CreateSampleWorkspace")
        alg.setChild(True)
        alg.initialize()
        alg.setProperty("OutputWorkspace", "dummy")
        alg.execute()
        return alg.getProperty("OutputWorkspace").value

    def test_that_can_read_and_write_tag_in_sample_logs(self):
        # Arrange
        ws1 = SANSLogTaggerTest._provide_sample_workspace()
        tag1 = "test"
        value1 = 123

        # Act + Assert
        self.assertFalse(has_tag(tag1, ws1))
        set_tag(tag1, value1, ws1)
        self.assertTrue(has_tag(tag1, ws1))
        self.assertEqual(get_tag(tag1, ws1), value1)

    def test_that_can_read_and_write_hash_in_sample_log(self):
        # Arrange
        ws1 = self._provide_sample_workspace()
        tag1 = "test"
        value1 = "tested"
        value2 = "tested2"

        # Act + Assert
        hashed_value_1 = get_hash_value(value1)
        hashed_value_2 = get_hash_value(value2)
        self.assertFalse(has_hash(tag1, hashed_value_1, ws1))
        set_hash(tag1, hashed_value_1, ws1)
        self.assertTrue(has_hash(tag1, hashed_value_1, ws1))
        self.assertFalse(has_hash(tag1, hashed_value_2, ws1))


if __name__ == "__main__":
    unittest.main()
