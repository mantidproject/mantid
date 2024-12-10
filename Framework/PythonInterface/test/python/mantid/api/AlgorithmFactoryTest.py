# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
import testhelpers

from mantid.api import AlgorithmFactory, FrameworkManagerImpl, PythonAlgorithm


class IsAnAlgorithm(PythonAlgorithm):
    def PyInit(self):
        pass


class NotAnAlgorithm(object):
    pass


class AlgorithmFactoryTest(unittest.TestCase):
    def setUp(self):
        FrameworkManagerImpl.Instance()

    def test_get_algorithm_factory_does_not_return_None(self):
        self.assertNotEqual(AlgorithmFactory, None)

    def test_getDescriptors(self):
        descriptors = AlgorithmFactory.getDescriptors(True)
        self.assertGreater(len(descriptors), 0)
        d = descriptors[0]
        self.assertTrue(hasattr(d, "name"))
        self.assertTrue(hasattr(d, "alias"))
        self.assertTrue(hasattr(d, "category"))
        self.assertTrue(hasattr(d, "version"))

    def test_getDescriptorsWithAlias(self):
        descriptors = AlgorithmFactory.getDescriptors(True, True)
        result = [d for d in descriptors if (d.name == "Subtract")]
        self.assertEqual(1, len(result))

    def test_exists_returns_correct_value_for_given_args(self):
        self.assertTrue(AlgorithmFactory.exists("ConvertUnits"))  # any version
        self.assertTrue(AlgorithmFactory.exists("ConvertUnits", 1))  # any version
        self.assertTrue(not AlgorithmFactory.exists("ConvertUnits", 100))  # any version

    def test_get_registered_algs_returns_dictionary_of_known_algorithms(self):
        all_algs = AlgorithmFactory.getRegisteredAlgorithms(True)
        self.assertTrue(len(all_algs) > 0)
        self.assertTrue("ConvertUnits" in all_algs)
        self.assertTrue("LoadRaw" in all_algs)
        # one versions of LoadRaw
        self.assertEqual(len(all_algs["LoadRaw"]), 1)
        self.assertEqual(all_algs["LoadRaw"], [3])

    def test_algorithm_subscription_with_valid_object_succeeds(self):
        testhelpers.assertRaisesNothing(self, AlgorithmFactory.subscribe, IsAnAlgorithm)

    def test_algorithm_registration_with_invalid_object_throws(self):
        self.assertRaises(ValueError, AlgorithmFactory.subscribe, NotAnAlgorithm)

    def test_can_enable_and_disable_notifications(self):
        try:
            AlgorithmFactory.enableNotifications()
        except Exception:
            self.fail("Algorithm factory class is expected to have a method 'enableNotifications'")

        try:
            AlgorithmFactory.disableNotifications()
        except Exception:
            self.fail("Algorithm factory class is expected to have a method 'disableNotifications'")


if __name__ == "__main__":
    unittest.main()
