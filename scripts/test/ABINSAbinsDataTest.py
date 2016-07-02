import unittest
import numpy as np
from mantid.simpleapi import *
from AbinsModules import AbinsData

class ABINSAbinsDataTest(unittest.TestCase):

    _good_data= [{"value":np.asarray([0.2, 0.1, 0.2]),
                   "weight":0.3,
                   "frequencies":np.asarray([1.0, 2.0, 34.0, 4.9,  1.0, 2.0]),  # 6 frequencies
                   "atomic_displacements":np.asarray([[1.0,1.0,1.0],[1.0,1.0,1.0],  [1.0,1.0,1.0],
                                                      [1.0,1.0,1.0],[1.0,1.0,1.0],  [1.0,1.0,1.0],
                                                      [1.0,1.0,1.0],[1.0,1.0,111.0],[1.0,1.0,1.0],
                                                      [1.0,1.0,1.0],[1.0,1.0,1.0],  [1.0,1.0,1.0]]) }, # 12 atomic displacements
                  {"value":np.asarray([0.1, 0.0, 0.2]),
                   "weight":0.2,
                   "frequencies":np.asarray([11.0, 12.0, 134.0, 14.9,  11.0, 12.0]),  # 6 frequencies
                   "atomic_displacements":np.asarray([[1.0,1.0,1.0],[1.0,1.0,1.0],  [1.0,1.0,1.0],
                                                      [1.0,1.0,1.0],[1.0,1.0,1.0],  [1.0,1.0,1.0],
                                                      [1.0,1.0,1.0],[1.0,1.0,221.0],[1.0,1.0,1.0],
                                                      [1.0,1.0,1.0],[1.0,1.0,1.0],  [1.0,1.0,1.0]]) },
                  {"value":np.asarray([0.2, 0.2, 0.2]),
                   "weight":0.5,
                   "frequencies":np.asarray([1.0, 2.0, 34.0, 4.9,  1.0, 2.0]),  # 6 frequencies
                   "atomic_displacements":np.asarray([[1.0,1.0,1.0],[1.0,1.0,41.0],  [1.0,1.0,1.0],
                                                      [1.0,1.0,1.0],[1.0,1.0,31.0],  [1.0,1.0,1.0],
                                                      [1.0,1.0,1.0],[1.0,1.0,1.0],   [1.0,1.0,1.0],
                                                      [1.0,1.0,1.0],[1.0,1.0,41.0],  [1.0,1.0,1.0]]) } # 12 atomic displacements
                  ]

    def setUp(self):
        self.tester = AbinsData(num_k=3, num_atoms=2)


    # tests for append method
    def test_no_dict(self):

        # Case no dict to append
        with self.assertRaises(ValueError):
            wrong_dict = ["value", 2, "freq"]
            self.tester.append(item=wrong_dict)

    def test_missing_key(self):
        # missing atomic_displacements
        item = {"value":self._good_data[0]["value"],
                "weight":self._good_data[0]["weight"],
                "frequencies": self._good_data[0]["frequencies"]}
        with self.assertRaises(ValueError):
            self.tester.append(item=item)


    def test_wrong_value(self):
        # value should be a real number
        item = {"value":"wrong_value",
                "weight":self._good_data[0]["weight"],
                "frequencies": self._good_data[0]["frequencies"],
                "atomic_displacements":  self._good_data[0]["atomic_displacements"]}
        with self.assertRaises(ValueError):
            self.tester.append(item=item)


    def test_wrong_weight(self):
        # complex weight (weight should be represented as a positive real number)
        item = { "value": self._good_data[0]["value"],
                 "weight": -0.1,
                 "frequencies": self._good_data[0]["frequencies"],
                 "atomic_displacements":  self._good_data[0]["atomic_displacements"]}
        with self.assertRaises(ValueError):
            self.tester.append(item=item)


    def test_wrong_freq(self):
        # frequencies as a string
        item = {"value":self._good_data[0]["value"],
                "weight":self._good_data[0]["weight"],
                "frequencies":"Wrong_freq",
                "atomic_displacements":self._good_data[0]["atomic_displacements"]}
        with self.assertRaises(ValueError):
            self.tester.append(item=item)



        # complex frequencies
        item = {"value":self._good_data[0]["value"],
                "weight":self._good_data[0]["weight"],
                "frequencies":self._good_data[0]["frequencies"].astype(complex),
                "atomic_displacements":self._good_data[0]["atomic_displacements"]
                }
        with self.assertRaises(ValueError):
            self.tester.append(item=item)

        # frequencies as 2D arrays
        item    = {"value":self._good_data[0]["value"],
                   "weight":self._good_data[0]["weight"],
                   "frequencies":np.asarray([[1.0,2.0,34.0],[4.9,1.0,1.0]]),
                   "atomic_displacements":self._good_data[0]["atomic_displacements"]}
        with self.assertRaises(ValueError):
            self.tester.append(item=item)


    def test_wrong_displacements(self):
        # displacements as a number
        item = {"value":self._good_data[0]["value"],
                "weight":self._good_data[0]["weight"],
                "frequencies":self._good_data[0]["frequencies"],
                "atomic_displacements":1}
        with self.assertRaises(ValueError):
            self.tester.append(item=item)

        # wrong size of the second dimension
        item = {"value":self._good_data[0]["value"],
                "weight":self._good_data[0]["weight"],
                "frequencies":self._good_data[0]["frequencies"],
                "atomic_displacements":np.asarray([[1.,1.,11.],[1.,1.,1.,1.0],[1.0,1.0, 1.0],
                                                   [1,1.0,1.0],[1.,1.,11.],   [1.,1.,11.],
                                                   [1,1.0,1.0],[1.,1.,11.],   [1.,1.,11.],
                                                   [1,1.0,1.0],[1.,1.,11.],   [1.,1.,11.]])}
        with self.assertRaises(ValueError):
            self.tester.append(item=item)

        # displacements as numpy arrays with integers
        item = {"value":self._good_data[0]["value"],
                "weight":self._good_data[0]["weight"],
                "frequencies":self._good_data[0]["frequencies"],
                "atomic_displacements": self._good_data[0]["atomic_displacements"].astype(int)}
        with self.assertRaises(ValueError):
            self.tester.append(item=item)

        # displacements as a 1D array
        item    = {"value":self._good_data[0]["value"],
                   "weight":self._good_data[0]["weight"],
                   "frequencies":self._good_data[0]["frequencies"],
                   "atomic_displacements":np.ravel(self._good_data[0]["atomic_displacements"])}
        with self.assertRaises(ValueError):
            self.tester.append(item=item)


    def test_append_good_case(self):
        for good_item in self._good_data:
            self.tester.append(item=good_item)

        collected_data = self.tester.extract()
        for i in range(len(self._good_data)):
            self.assertEqual(self._good_data[i]["weight"], collected_data[i]["weight"])
            self.assertEqual(True, np.allclose(self._good_data[i]["value"], collected_data[i]["value"]))
            self.assertEqual(True, np.allclose(self._good_data[i]["frequencies"], collected_data[i]["frequencies"]))
            self.assertEqual(True, np.allclose(self._good_data[i]["atomic_displacements"], collected_data[i]["atomic_displacements"]))


    # tests for set method
    def test_set_wrong_dict(self):
        with self.assertRaises(ValueError):
            self.tester.set([1,2234,8])


    # tests for constructor
    def test_constructor_assertions(self):
        with self.assertRaises(ValueError):
            poor_tester = AbinsData(num_k=0.1, num_atoms=2)

        with self.assertRaises(ValueError):
            poor_tester = AbinsData(num_k=1, num_atoms=-2)


    #tests for extract method
    def test_wrong_k(self):
        poor_tester= AbinsData(num_k=1, num_atoms=2)
        poor_tester.set(items=self._good_data)
        with self.assertRaises(ValueError):
            poor_data = poor_tester.extract()


    def test_wrong_atoms(self):
        poor_tester= AbinsData(num_k=3, num_atoms=4)
        with self.assertRaises(ValueError):
            poor_tester.set(items=self._good_data)


    def test_extract_good_case(self):
        self.tester.set(self._good_data)
        collected_data = self.tester.extract()

        for i in range(len(self._good_data)):
            self.assertEqual(self._good_data[i]["weight"], collected_data[i]["weight"])
            self.assertEqual(True, np.allclose(self._good_data[i]["value"], collected_data[i]["value"]))
            self.assertEqual(True, np.allclose(self._good_data[i]["frequencies"], collected_data[i]["frequencies"]))
            self.assertEqual(True, np.allclose(self._good_data[i]["atomic_displacements"], collected_data[i]["atomic_displacements"]))

if __name__ == '__main__':
    unittest.main()