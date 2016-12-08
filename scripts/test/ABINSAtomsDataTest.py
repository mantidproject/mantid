import unittest
from mantid.simpleapi import logger
import numpy as np
from AbinsModules import AtomsDaTa, AbinsConstants


def old_python():
    """" Check if Python has proper version."""
    is_python_old = AbinsConstants.old_python()
    if is_python_old:
        logger.warning("Skipping ABINSAtomsDataTest because Python is too old.")
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
class ABINSAtomsDataTest(unittest.TestCase):
    _good_data = [{'sort': 0, 'symbol': 'Si', 'fract_coord': np.asarray([0.,  0.,  0.]), 'atom': 0, 'mass':28.085500},
                  {'sort': 1, 'symbol': 'Si', 'fract_coord': np.asarray([0.25,  0.25,  0.25]), 'atom': 1,
                   'mass':28.085500}]

    def setUp(self):
        self.tester = AtomsDaTa(num_atoms=2)

    # constructor
    def test_wrong_num_atoms(self):
        with self.assertRaises(ValueError):
            # noinspection PyUnusedLocal
            wrong_tester = AtomsDaTa(num_atoms=-2)

    # append
    def test_wrong_sort(self):

        # too large
        wrong_data = {"sort": 3,
                      "symbol": self._good_data[0]["symbol"],
                      "fract_coord": self._good_data[0]["fract_coord"],
                      "atom": self._good_data[0]["atom"],
                      "mass": self._good_data[0]["mass"]
                      }
        with self.assertRaises(ValueError):
            self.tester._append(item=wrong_data)

        wrong_data["sort"] = 2  # we count from zero so 2 is also to large
        with self.assertRaises(ValueError):
            self.tester._append(item=wrong_data)

        # negative
        wrong_data["sort"] = -1,

        with self.assertRaises(ValueError):
            self.tester._append(item=wrong_data)

        # string instead of number
        wrong_data["sort"] = "-1"

        with self.assertRaises(ValueError):
            self.tester._append(item=wrong_data)

    def test_wrong_symbol(self):
        wrong_data = {"sort": self._good_data[0]["sort"],
                      "symbol": "CA",  # valid is Ca not CA
                      "fract_coord": self._good_data[0]["fract_coord"],
                      "atom": self._good_data[0]["atom"],
                      "mass": self._good_data[0]["mass"]
                      }
        with self.assertRaises(ValueError):
            self.tester._append(item=wrong_data)

    def test_wrong_fract_coord(self):
        wrong_data = {"sort": self._good_data[0]["sort"],
                      "symbol": self._good_data[0]["symbol"],
                      "fract_coord": "wrong",
                      "atom": self._good_data[0]["atom"],
                      "mass": self._good_data[0]["mass"]
                      }
        with self.assertRaises(ValueError):
            self.tester._append(item=wrong_data)

        wrong_data["fract_coord"] = np.asarray([[1, 2], [4]])
        with self.assertRaises(ValueError):
            self.tester._append(item=wrong_data)

        wrong_data["fract_coord"] = np.asarray([1, 2])
        with self.assertRaises(ValueError):
            self.tester._append(item=wrong_data)

    def test_wrong_atom(self):
        wrong_data = {"sort": self._good_data[0]["sort"],
                      "symbol": self._good_data[0]["symbol"],
                      "fract_coord": self._good_data[0]["fract_coord"],
                      "atom": -1,
                      "mass": self._good_data[0]["mass"]}

        with self.assertRaises(ValueError):
            self.tester._append(item=wrong_data)

        wrong_data["atom"] = 2
        with self.assertRaises(ValueError):
            self.tester._append(item=wrong_data)

        wrong_data["atom"] = 3
        with self.assertRaises(ValueError):
            self.tester._append(item=wrong_data)

    def test_wrong_mass(self):

        wrong_data = {"sort": self._good_data[0]["sort"],
                      "symbol": self._good_data[0]["symbol"],
                      "fract_coord": self._good_data[0]["fract_coord"],
                      "atom": self._good_data[0]["atom"],
                      "mass": -1.0,
                      }
        with self.assertRaises(ValueError):
            self.tester._append(item=wrong_data)

        wrong_data["mass"] = 28  # int instead of float
        with self.assertRaises(ValueError):
            self.tester._append(item=wrong_data)

    # set
    def test_wrong_list(self):

        items = self._good_data[0]  # dict instead of list
        with self.assertRaises(ValueError):
            self.tester.set(items=items)

    # extract
    def wrong_size_of_data(self):
        self.tester._append(self._good_data[0])
        with self.assertRaises(ValueError):
            self.tester.extract()

    # valid situations
    def test_good_append(self):

        self.tester._append(self._good_data[0])
        self.tester._append(self._good_data[1])

        data = self.tester.extract()
        elements = len(self._good_data)
        for el in range(elements):
            self.assertEqual(self._good_data[el]["sort"], data[el]["sort"])
            self.assertEqual(self._good_data[el]["symbol"], data[el]["symbol"])
            self.assertEqual(True, np.allclose(self._good_data[el]["fract_coord"], data[el]["fract_coord"]))
            self.assertEqual(self._good_data[el]["atom"], data[el]["atom"])
            self.assertEqual(self._good_data[el]["mass"], data[el]["mass"])

    def test_good_set(self):
        self.tester.set(self._good_data)
        data = self.tester.extract()
        elements = len(self._good_data)

        for el in range(elements):
            self.assertEqual(self._good_data[el]["sort"], data[el]["sort"])
            self.assertEqual(self._good_data[el]["symbol"], data[el]["symbol"])
            self.assertEqual(True, np.allclose(self._good_data[el]["fract_coord"], data[el]["fract_coord"]))
            self.assertEqual(self._good_data[el]["atom"], data[el]["atom"])
            self.assertEqual(self._good_data[el]["mass"], data[el]["mass"])

if __name__ == '__main__':
    unittest.main()
