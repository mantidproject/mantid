import unittest
import numpy as np
from mantid.simpleapi import *


from os import path

try:
    import simplejson as json
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

from AbinsModules import KpointsData
from AbinsModules.InstrumentProducer import InstrumentProducer
from AbinsModules import AbinsConstants, AbinsParameters


class ABINSCalculateQToscaTest(unittest.TestCase):

    def setUp(self):
        producer = InstrumentProducer()
        self._tosca_instrument = producer.produceInstrument("TOSCA")
        self._raw_data = KpointsData(num_k=1, num_atoms=2)
        self._raw_data.set({"k_vectors": np.asarray([[0.0, 0.0, 0.0]]),
                            "weights": np.asarray([0.3]),
                            # 6 frequencies, globally frequencies are in hartree units if necessary
                            # they are converted to cm^-1
                            "frequencies":
                            np.asarray([[100.0, 200.0, 300.0, 400.0,  500.0, 600.0]]) * AbinsConstants.cm1_2_hartree,

                            "atomic_displacements":

                            # 12 atomic displacements
                            np.asarray([[[[1.0, 1.0, 1.0], [1.0, 1.0, 1.0],  [1.0, 1.0, 1.0],
                                          [1.0, 1.0, 1.0], [1.0, 1.0, 1.0],  [1.0, 1.0, 1.0]],
                                         [[1.0, 1.0, 1.0], [1.0, 1.0, 111.0], [1.0, 1.0, 1.0],
                                          [1.0, 1.0, 1.0], [1.0, 1.0, 1.0],  [1.0, 1.0, 1.0]]]]).astype(complex)})

    # Use case: TOSCA instrument
    def test_TOSCA(self):

        # calculate Q for TOSCA
        extracted_raw_data = self._raw_data.extract()

        # convert frequencies to cm^-1 in order to compare
        freq = extracted_raw_data["frequencies"][0][AbinsConstants.first_optical_phonon:] / AbinsConstants.cm1_2_hartree
        k2_i = (freq + AbinsParameters.TOSCA_final_neutron_energy) * AbinsConstants.TOSCA_constant
        k2_f = AbinsParameters.TOSCA_final_neutron_energy * AbinsConstants.TOSCA_constant
        # noinspection PyTypeChecker
        correct_q_data = k2_i + k2_f - 2 * np.power(k2_i * k2_f, 0.5) * AbinsParameters.TOSCA_cos_scattering_angle

        q2 = self._tosca_instrument._calculate_q_powder(freq)

        # noinspection PyTypeChecker
        self.assertEqual(True, np.allclose(correct_q_data, q2))

if __name__ == '__main__':
    unittest.main()
