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

from AbinsModules import CalculateQ
from AbinsModules import KpointsData
from AbinsModules.InstrumentProducer import InstrumentProducer
from AbinsModules import AbinsConstants, AbinsParameters


class ABINSCalculateQToscaTest(unittest.TestCase):

    def setUp(self):
        _core = "../ExternalData/Testing/Data/UnitTest/"
        producer = InstrumentProducer()
        self._tosca_instrument = producer.produceInstrument("TOSCA")
        self._filename = path.relpath(_core + "Si2-sc_Q_test.phonon")
        self._sample_form = "Powder"
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

    def test_simple(self):
        """
        Tests various  assertions
        """

        # wrong file name
        with self.assertRaises(ValueError):
            poor_q_calculator = CalculateQ(filename=1,
                                           instrument=self._tosca_instrument,
                                           sample_form=self._sample_form,
                                           k_points_data=self._raw_data,
                                           overtones=False,
                                           combinations=False)

        # wrong instrument
        with self.assertRaises(ValueError):
            poor_q_calculator = CalculateQ(filename=self._filename,
                                           instrument="Different_instrument",
                                           sample_form=self._sample_form,
                                           k_points_data=self._raw_data,
                                           overtones=False,
                                           combinations=False)

        # wrong sample form
        with self.assertRaises(ValueError):
            poor_q_calculator = CalculateQ(filename=self._filename,
                                           instrument=self._tosca_instrument,
                                           sample_form="Solid",
                                           k_points_data=self._raw_data,
                                           overtones=False,
                                           combinations=False)

        # no k_points_data
        with self.assertRaises(ValueError):
            poor_q_calculator = CalculateQ(filename=self._filename,
                                           sample_form=self._sample_form,
                                           instrument=self._tosca_instrument,
                                           overtones=False,
                                           combinations=False)

        # wrong value of overtones
        with self.assertRaises(ValueError):
            poor_q_calculator = CalculateQ(filename=self._filename,
                                           instrument=self._tosca_instrument,
                                           sample_form=self._sample_form,
                                           k_points_data=self._raw_data,
                                           overtones="bad_overtones",
                                           combinations=False)

        # wrong value of combinations
        with self.assertRaises(ValueError):
            poor_q_calculator = CalculateQ(filename=self._filename,
                                           instrument=self._tosca_instrument,
                                           sample_form=self._sample_form,
                                           k_points_data=self._raw_data,
                                           overtones=False,
                                           combinations="bad_combinations")

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

        q_calculator = CalculateQ(filename=self._filename,
                                  instrument=self._tosca_instrument,
                                  sample_form=self._sample_form,
                                  k_points_data=self._raw_data,
                                  overtones=False,
                                  combinations=False)
        q_vectors = q_calculator.calculateData()

        # noinspection PyTypeChecker
        self.assertEqual(True, np.allclose(correct_q_data,
                                           q_vectors.extract()["order_%s" % AbinsConstants.fundamentals]))

        # check loading data
        loaded_q = q_calculator.loadData()

        # noinspection PyTypeChecker
        self.assertEqual(True, np.allclose(correct_q_data,
                                           loaded_q.extract()["order_%s" % AbinsConstants.fundamentals]))


if __name__ == '__main__':
    unittest.main()
