from __future__ import (absolute_import, division, print_function)
import unittest
import numpy as np
from mantid.simpleapi import logger
from AbinsModules import KpointsData, AbinsTestHelpers


def old_python():
    """" Check if Python has proper version."""
    is_python_old = AbinsTestHelpers.old_python()
    if is_python_old:
        logger.warning("Skipping AbinsKpointsDataTest because Python is too old.")
    return is_python_old


def skip_if(skipping_criteria):
    """
    Skip all tests if the supplied function returns true.
    Python unittest.skipIf is not available in 2.6 (RHEL6) so we'll roll our own.
    """
    def decorate(cls):
        if skipping_criteria():
            for attr in cls.__dict__.keys():
                if callable(getattr(cls, attr)) and 'test' in attr:
                    delattr(cls, attr)
        return cls
    return decorate


@skip_if(old_python)
class AbinsKpointsDataTest(unittest.TestCase):

    _good_data = {"k_vectors": np.asarray([[0.2, 0.1, 0.2], [0.1, 0.0, 0.2], [0.2, 0.2, 0.2]]),
                  "weights": np.asarray([0.3, 0.2, 0.5]),
                  "frequencies": np.asarray([[1.0, 2.0, 34.0, 4.9, 1.0, 2.0],
                                             [11.0, 12.0, 134.0, 14.9, 11.0, 12.0],
                                             [1.0, 2.0, 34.0, 4.9, 1.0, 2.0]]),  # 6 frequencies for one k-point
                  "atomic_displacements": np.asarray([[[[1.0, 1.0, 1.0], [1.0, 1.0, 1.0], [1.0, 1.0, 1.0],
                                                        [1.0, 1.0, 1.0], [1.0, 1.0, 1.0], [1.0, 1.0, 1.0]],

                                                       [[1.0, 1.0, 1.0], [1.0, 1.0, 111.0], [1.0, 1.0, 1.0],
                                                       [1.0, 1.0, 1.0], [1.0, 1.0, 1.0], [1.0, 1.0, 1.0]]],

                                                     [[[1.0, 1.0, 1.0], [1.0, 1.0, 1.0], [1.0, 1.0, 1.0],
                                                       [1.0, 1.0, 1.0], [1.0, 1.0, 1.0], [1.0, 1.0, 1.0]],

                                                      [[1.0, 1.0, 1.0], [1.0, 1.0, 221.0], [1.0, 1.0, 1.0],
                                                      [1.0, 1.0, 1.0], [1.0, 1.0, 1.0], [1.0, 1.0, 1.0]]],

                                                     [[[1.0, 1.0, 1.0], [1.0, 1.0, 41.0], [1.0, 1.0, 1.0],
                                                      [1.0, 1.0, 1.0], [1.0, 1.0, 31.0], [1.0, 1.0, 1.0]],

                                                      [[1.0, 1.0, 1.0], [1.0, 1.0, 1.0], [1.0, 1.0, 1.0],
                                                      [1.0, 1.0, 1.0], [1.0, 1.0, 41.0], [1.0, 1.0, 1.0]]]
                                                     # 12 atomic displacements for each k-point
                                                      ]).astype(complex)}

    def setUp(self):
        self.tester = KpointsData(num_k=3, num_atoms=2)

    # tests for append method
    def test_no_dict(self):

        # Case no dict to append
        with self.assertRaises(ValueError):
            wrong_dict = ["k_vectors", 2, "freq"]
            self.tester.set(items=wrong_dict)

    def test_missing_key(self):
        # missing atomic_displacements
        items = {"k_vectors": self._good_data["k_vectors"],
                 "weights": self._good_data["weights"],
                 "frequencies": self._good_data["frequencies"]}
        with self.assertRaises(ValueError):
            self.tester.set(items=items)

    def test_wrong_value(self):
        # value should be a numpy array with real numbers
        items = {"k_vectors": "wrong_value",
                 "weights": self._good_data["weights"],
                 "frequencies": self._good_data["frequencies"],
                 "atomic_displacements":  self._good_data["atomic_displacements"]}
        with self.assertRaises(ValueError):
            self.tester.set(items=items)

    def test_wrong_weight(self):
        # negative weight (weight should be represented as a positive real number)
        items = {"k_vectors": self._good_data["k_vectors"],
                 "weights": np.asarray([-0.1, 0.3, 0.2]),
                 "frequencies": self._good_data["frequencies"],
                 "atomic_displacements": self._good_data["atomic_displacements"]}
        with self.assertRaises(ValueError):
            self.tester.set(items=items)

    def test_wrong_freq(self):
        # frequencies as a string
        wrong_items = {"k_vectors": self._good_data["k_vectors"],
                       "weights": self._good_data["weights"],
                       "frequencies": "Wrong_freq",
                       "atomic_displacements": self._good_data["atomic_displacements"]}
        with self.assertRaises(ValueError):
            self.tester.set(items=wrong_items)

        # complex frequencies
        wrong_items = {"k_vectors": self._good_data["k_vectors"],
                       "weights": self._good_data["weights"],
                       "frequencies": self._good_data["frequencies"].astype(complex),
                       "atomic_displacements": self._good_data["atomic_displacements"]}
        with self.assertRaises(ValueError):
            self.tester.set(items=wrong_items)

        # frequencies as 2D arrays but with a bad shape
        wrong_items = {"k_vectors": self._good_data["k_vectors"],
                       "weights": self._good_data["weights"],
                       "frequencies": np.asarray([[1.0, 2.0, 34.0], [4.9, 1.0, 1.0]]),
                       "atomic_displacements": self._good_data["atomic_displacements"]}
        with self.assertRaises(ValueError):
            self.tester.set(items=wrong_items)

    def test_wrong_displacements(self):
        # displacements as a number
        wrong_items = {"k_vectors": self._good_data["k_vectors"],
                       "weights": self._good_data["weights"],
                       "frequencies": self._good_data["frequencies"],
                       "atomic_displacements": 1}
        with self.assertRaises(ValueError):
            self.tester.set(items=wrong_items)

        # wrong size of the second dimension
        wrong_items = {"k_vectors": self._good_data["k_vectors"],
                       "weights": self._good_data["weights"],
                       "frequencies": self._good_data["frequencies"],
                       "atomic_displacements": np.asarray([[[[1., 1., 11.],  [1.,  1., 1., 1.0], [1.0, 1.0, 1.0],
                                                             [1., 1.0, 1.0], [1., 1., 11.],     [1., 1.,  11.]],

                                                            [[1., 1.0, 1.0], [1., 1., 11.],     [1., 1.,  11.],
                                                            [1., 1.0, 1.0],  [1., 1., 11.],     [1., 1.,  11.]]],
                                                          self._good_data["atomic_displacements"][0, 0],
                                                          self._good_data["atomic_displacements"][0, 1]]
                                                          )}
        with self.assertRaises(ValueError):
            self.tester.set(items=wrong_items)

        # displacements as numpy arrays with integers
        wrong_items = {"k_vectors": self._good_data["k_vectors"],
                       "weights": self._good_data["weights"],
                       "frequencies": self._good_data["frequencies"],
                       "atomic_displacements": self._good_data["atomic_displacements"].astype(int)}
        with self.assertRaises(ValueError):
            self.tester.set(items=wrong_items)

        # displacements as a 1D array
        wrong_items = {"k_vectors": self._good_data["k_vectors"],
                       "weights": self._good_data["weights"],
                       "frequencies": self._good_data["frequencies"],
                       "atomic_displacements": np.ravel(self._good_data["atomic_displacements"])}
        with self.assertRaises(ValueError):
            self.tester.set(items=wrong_items)

    def test_set_good_case(self):
        self.tester.set(items=self._good_data)

        collected_data = self.tester.extract()
        self.assertEqual(True, np.allclose(self._good_data["weights"], collected_data["weights"]))
        self.assertEqual(True, np.allclose(self._good_data["k_vectors"], collected_data["k_vectors"]))
        self.assertEqual(True, np.allclose(self._good_data["frequencies"], collected_data["frequencies"]))
        self.assertEqual(True, np.allclose(self._good_data["atomic_displacements"],
                                           collected_data["atomic_displacements"]))

    # tests for set method
    def test_set_wrong_dict(self):
        with self.assertRaises(ValueError):
            self.tester.set([1, 2234, 8])

    # tests for constructor
    def test_constructor_assertions(self):
        with self.assertRaises(ValueError):
            # noinspection PyUnusedLocal
            poor_tester = KpointsData(num_k=0.1, num_atoms=2)

        with self.assertRaises(ValueError):
            # noinspection PyUnusedLocal
            poor_tester = KpointsData(num_k=1, num_atoms=-2)


if __name__ == "__main__":
    unittest.main()
