import unittest
import json
from mantid.simpleapi import *

from AbinsModules import  CalculateQ


class CalculateQTest(unittest.TestCase):


    def test_simple(self):
        """
        Tests various  assertions
        """

        # wrong file name
        with self.assertRaises(ValueError):
            poor_q_calculator = CalculateQ(filename=1, instrument="TOSCA", sample_form="Powder")

        # wrong instrument
        with self.assertRaises(ValueError):
            poor_q_calculator = CalculateQ(filename="one_file", instrument="Different_instrument", sample_form="Powder")

        # wrong sample form
        with self.assertRaises(ValueError):
            poor_q_calculator = CalculateQ(filename="one_file", instrument="Different_instrument", sample_form="Solid")

        # no frequencies required for the case when Q vectors do not depend on frequencies
        with self.assertRaises(ValueError):
            poor_q_calculator = CalculateQ(filename="one_file", instrument="Different_instrument", sample_form="Powder")
            poor_q_calculator.collectFrequencies(frequencies=np.array([1,2,3,4]))


    _core = "../ExternalData/Testing/Data/UnitTest/"

    # Use case: TOSCA
    def test_TOSCA(self):

        raw_data = self._prepare_data(name="TOSCA")
        correct_q_vectors = raw_data * raw_data/16.0

        q_calculator = CalculateQ(filename="TestingFile_TOSCA.phonon",
                                  instrument="TOSCA",
                                  sample_form="Powder")
        q_calculator.collectFrequencies(frequencies=raw_data)
        q_vectors = q_calculator.getQvectors()

        self.assertEqual(True,np.allclose(correct_q_vectors, q_vectors))

        loaded_q = q_calculator.loadData()

        self.assertEqual(True,np.allclose(correct_q_vectors, loaded_q))

        # here we have a list not a numpy array
        with self.assertRaises(ValueError):
            q_calculator.collectFrequencies([1,2,3])


    # Helper functions
    def _prepare_data(self, name=None):
        """Reads a corrects values from ASCII file."""
        correct_data = None
        with open(self._core+"Calculate_Q_"+name+"_data.txt") as data_file:
            correct_data = json.loads(data_file.read().replace("\n"," ").
                                      replace("array","").
                                      replace("([","[").
                                      replace("])","]").
                                      replace(".,",".0,").
                                      replace(".]",".0]").
                                      replace(". ",".0").
                                      replace("'",'"'))



        return np.array(correct_data)




