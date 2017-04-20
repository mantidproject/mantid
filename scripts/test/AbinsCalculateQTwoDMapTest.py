from __future__ import (absolute_import, division, print_function)
import unittest
import numpy as np
from mantid.simpleapi import *

from os import path

try:
    import json
except ImportError:
    logger.warning("Failure of CalculateQTest because simplejson is unavailable.")
    exit(1)

try:
    import scipy
except ImportError:
    logger.warning("Failure of CalculateQTest because scipy is unavailable.")
    exit(1)

try:
    import h5py
except ImportError:
    logger.warning("Failure of CalculateQTest because h5py is unavailable.")
    exit(1)

from AbinsModules import CalculateQ
from AbinsModules import KpointsData
from AbinsModules.InstrumentProducer import InstrumentProducer
from AbinsModules import AbinsParameters


class AbinsCalculateQTwoDMapTest(unittest.TestCase):

    def setUp(self):
        _core = os.path.normpath("../ExternalData/Testing/Data/UnitTest/")
        producer = InstrumentProducer()
        self._none_instrument = producer.produce_instrument("None")
        self._filename = path.abspath(os.path.join(_core, "_si2-sc_Q_test.phonon"))
        self._sample_form = "Powder"
        self._raw_data = KpointsData(num_k=1, num_atoms=2)
        self._raw_data.set({"k_vectors": np.asarray([[0.2, 0.1, 0.2]]),
                            "weights": np.asarray([0.3]),
                            "frequencies": np.asarray([[1.0, 2.0, 3.0, 4.0,  5.0, 6.0]]),  # 6 frequencies
                            "atomic_displacements": np.asarray([[[[1.0, 1.0, 1.0], [1.0, 1.0, 1.0],   [1.0, 1.0, 1.0],
                                                                 [1.0,  1.0, 1.0], [1.0, 1.0, 1.0],   [1.0, 1.0, 1.0]],
                                                                [[1.0,  1.0, 1.0], [1.0, 1.0, 111.0], [1.0, 1.0, 1.0],
                                                                 [1.0,  1.0, 1.0], [1.0, 1.0, 1.0], [1.0, 1.0, 1.0]]]]).
                           astype(complex)})  # 12 atomic displacements

    # Use case: TOSCA instrument
    def test_two_d_map(self):

        extracted_raw_data = self._raw_data.extract()

        correct_q_data = ((extracted_raw_data["frequencies"][0] / AbinsParameters.cm1_2_hartree) *
                          (extracted_raw_data["frequencies"][0] / AbinsParameters.cm1_2_hartree) /
                          16.0)

        producer = InstrumentProducer()
        tosca_instrument = producer.produce_instrument("TwoDMap")
        q_calculator = CalculateQ(filename=self._filename, instrument=self._none_instrument,
                                  sample_form=self._sample_form, k_points_data=self._raw_data)
        q_vectors = q_calculator.calculate_data()

        # noinspection PyTypeChecker
        self.assertEqual(True, np.allclose(correct_q_data, q_vectors.extract()))

        # check loading data
        loaded_q = q_calculator.load_formatted_data()

        # noinspection PyTypeChecker
        self.assertEqual(True, np.allclose(correct_q_data, loaded_q.extract()))

if __name__ == '__main__':
    unittest.main()
