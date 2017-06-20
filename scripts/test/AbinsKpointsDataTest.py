from __future__ import (absolute_import, division, print_function)
import unittest
import numpy as np
from mantid.simpleapi import logger
import AbinsModules


def old_python():
    """" Check if Python has proper version."""
    is_python_old = AbinsModules.AbinsTestHelpers.old_python()
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

    _good_data_1 = {"k_vectors": np.asarray([[0.2, 0.1, 0.2], [0.1, 0.0, 0.2], [0.2, 0.2, 0.2]]),
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
                                                      ]).astype(complex),
                    "unit_cell": np.asarray([[ 7.44,  0.  ,  0.  ],
                                             [ 0.  ,  9.55,  0.  ],
                                             [ 0.  ,  0.  ,  6.92]])
                    }


    # data with soft phonons
    _good_data_2 = {"k_vectors": np.asarray([[0.2, 0.1, 0.2], [0.1, 0.0, 0.2], [0.2, 0.2, 0.2]]),
                    "weights": np.asarray([0.3, 0.2, 0.5]),
                    "frequencies": np.asarray([[-10.0, -2.0, -3.0, 4.9, 1.0, 2.0],
                                               [11.0, 12.0, 134.0, 14.9, 11.0, 12.0],
                                               [1.0, 2.0, 34.0, 4.9, 1.0, 2.0]]),  # 6 frequencies for one k-point
                    "atomic_displacements": np.asarray([[[[1.0, 1.0, 1.0], [1.0, 1.0, 1.0], [1.0, 1.0, 1.0],
                                                          [1.0, 1.0, 1.0], [1.0, 121.0, 1.0], [1.0, 1.0, 1.0]],

                                                         [[1.0, 1.0, 1.0], [1.0, 1.0, 1.0], [1.0, 1.0, 1.0],
                                                          [1.0, 1.0, 1.0], [1.0, 1.0, 131.0], [1.0, 1.0, 1.0]]],


                                                        [[[1.0, 1.0, 1.0], [1.0, 1.0, 1.0], [1.0, 1.0, 1.0],
                                                          [1.0, 1.0, 1.0], [1.0, 1.0, 1.0], [1.0, 1.0, 1.0]],

                                                         [[1.0, 1.0, 1.0], [1.0, 1.0, 221.0], [1.0, 1.0, 1.0],
                                                          [1.0, 1.0, 1.0], [1.0, 1.0, 1.0], [1.0, 1.0, 1.0]]],


                                                        [[[1.0, 1.0, 1.0], [1.0, 1.0, 41.0], [1.0, 1.0, 1.0],
                                                          [1.0, 1.0, 1.0], [1.0, 1.0, 31.0], [1.0, 1.0, 1.0]],

                                                         [[1.0, 1.0, 1.0], [1.0, 1.0, 1.0], [1.0, 1.0, 1.0],
                                                          [1.0, 1.0, 1.0], [1.0, 1.0, 41.0], [1.0, 1.0, 1.0]]]
                                                        # 12 atomic displacements for each k-point
                                                        ]).astype(complex),
                    "unit_cell": np.asarray([[7.44, 0., 0.],
                                             [0., 9.55, 0.],
                                             [0., 0., 6.92]])
                    }

    def setUp(self):
        self.tester = AbinsModules.KpointsData(num_k=3, num_atoms=2)

    # tests for append method
    def test_no_dict(self):

        # Case no dict to append
        with self.assertRaises(ValueError):
            wrong_dict = ["k_vectors", 2, "freq"]
            self.tester.set(items=wrong_dict)

    def test_missing_key(self):
        # missing atomic_displacements
        items = {"k_vectors": self._good_data_1["k_vectors"],
                 "weights": self._good_data_1["weights"],
                 "frequencies": self._good_data_1["frequencies"]}
        with self.assertRaises(ValueError):
            self.tester.set(items=items)

    def test_wrong_value(self):
        # value should be a numpy array with real numbers
        items = {"k_vectors": "wrong_value",
                 "weights": self._good_data_1["weights"],
                 "frequencies": self._good_data_1["frequencies"],
                 "atomic_displacements":  self._good_data_1["atomic_displacements"]}
        with self.assertRaises(ValueError):
            self.tester.set(items=items)

    def test_wrong_weight(self):
        # negative weight (weight should be represented as a positive real number)
        items = {"k_vectors": self._good_data_1["k_vectors"],
                 "weights": np.asarray([-0.1, 0.3, 0.2]),
                 "frequencies": self._good_data_1["frequencies"],
                 "atomic_displacements": self._good_data_1["atomic_displacements"]}
        with self.assertRaises(ValueError):
            self.tester.set(items=items)

    def test_wrong_freq(self):
        # frequencies as a string
        wrong_items = {"k_vectors": self._good_data_1["k_vectors"],
                       "weights": self._good_data_1["weights"],
                       "frequencies": "Wrong_freq",
                       "atomic_displacements": self._good_data_1["atomic_displacements"]}
        with self.assertRaises(ValueError):
            self.tester.set(items=wrong_items)

        # complex frequencies
        wrong_items = {"k_vectors": self._good_data_1["k_vectors"],
                       "weights": self._good_data_1["weights"],
                       "frequencies": self._good_data_1["frequencies"].astype(complex),
                       "atomic_displacements": self._good_data_1["atomic_displacements"]}
        with self.assertRaises(ValueError):
            self.tester.set(items=wrong_items)

        # frequencies as 2D arrays but with a bad shape
        wrong_items = {"k_vectors": self._good_data_1["k_vectors"],
                       "weights": self._good_data_1["weights"],
                       "frequencies": np.asarray([[1.0, 2.0, 34.0], [4.9, 1.0, 1.0]]),
                       "atomic_displacements": self._good_data_1["atomic_displacements"]}
        with self.assertRaises(ValueError):
            self.tester.set(items=wrong_items)

    def test_wrong_displacements(self):
        # displacements as a number
        wrong_items = {"k_vectors": self._good_data_1["k_vectors"],
                       "weights": self._good_data_1["weights"],
                       "frequencies": self._good_data_1["frequencies"],
                       "atomic_displacements": 1}
        with self.assertRaises(ValueError):
            self.tester.set(items=wrong_items)

        # wrong size of the second dimension
        wrong_items = {"k_vectors": self._good_data_1["k_vectors"],
                       "weights": self._good_data_1["weights"],
                       "frequencies": self._good_data_1["frequencies"],
                       "atomic_displacements": np.asarray([[[[1., 1., 11.],  [1.,  1., 1., 1.0], [1.0, 1.0, 1.0],
                                                             [1., 1.0, 1.0], [1., 1., 11.],     [1., 1.,  11.]],

                                                            [[1., 1.0, 1.0], [1., 1., 11.],     [1., 1.,  11.],
                                                            [1., 1.0, 1.0],  [1., 1., 11.],     [1., 1.,  11.]]],
                                                           self._good_data_1["atomic_displacements"][0, 0],
                                                           self._good_data_1["atomic_displacements"][0, 1]]
                                                          )}
        with self.assertRaises(ValueError):
            self.tester.set(items=wrong_items)

        # displacements as numpy arrays with integers
        wrong_items = {"k_vectors": self._good_data_1["k_vectors"],
                       "weights": self._good_data_1["weights"],
                       "frequencies": self._good_data_1["frequencies"],
                       "atomic_displacements": self._good_data_1["atomic_displacements"].astype(int)}
        with self.assertRaises(ValueError):
            self.tester.set(items=wrong_items)

        # displacements as a 1D array
        wrong_items = {"k_vectors": self._good_data_1["k_vectors"],
                       "weights": self._good_data_1["weights"],
                       "frequencies": self._good_data_1["frequencies"],
                       "atomic_displacements": np.ravel(self._good_data_1["atomic_displacements"])}
        with self.assertRaises(ValueError):
            self.tester.set(items=wrong_items)

    def test_set_good_case(self):

        self._set_good_case_core(data=self._good_data_1)
        self._set_good_case_core(data=self._good_data_2)

    def _set_good_case_core(self, data):
        self.tester.set(items=data)
        collected_data = self.tester.extract()

        for k in range(data["frequencies"].shape[0]):
            indices = data["frequencies"][k] > AbinsModules.AbinsConstants.ACOUSTIC_PHONON_THRESHOLD
            temp_f = data["frequencies"][k]
            self.assertEqual(True, np.allclose(temp_f[indices],
                                               collected_data["frequencies"][str(k)]))
            temp_a = data["atomic_displacements"][k]
            self.assertEqual(True, np.allclose(temp_a[:, indices],
                                               collected_data["atomic_displacements"][str(k)]))
            self.assertEqual(True, np.allclose(data["k_vectors"][k], collected_data["k_vectors"][str(k)]))
            self.assertEqual(data["weights"][k], collected_data["weights"][str(k)])

    # tests for set method
    def test_set_wrong_dict(self):
        with self.assertRaises(ValueError):
            self.tester.set([1, 2234, 8])

    # tests for constructor
    def test_constructor_assertions(self):
        with self.assertRaises(ValueError):
            # noinspection PyUnusedLocal
            poor_tester = AbinsModules.KpointsData(num_k=0.1, num_atoms=2)

        with self.assertRaises(ValueError):
            # noinspection PyUnusedLocal
            poor_tester = AbinsModules.KpointsData(num_k=1, num_atoms=-2)


if __name__ == "__main__":
    unittest.main()
