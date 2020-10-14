# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
import numpy as np

# from mantid.simpleapi import *
import abins
from abins.constants import CM1_2_HARTREE, FIRST_OPTICAL_PHONON, WAVENUMBER_TO_INVERSE_A
import abins.instruments
from abins import KpointsData


class CalculateQToscaTest(unittest.TestCase):
    def setUp(self):
        self._tosca_instrument = abins.instruments.get_instrument("TOSCA")

        atomic_displacements = np.asarray([[[[1.0, 1.0, 1.0], [1.0, 1.0, 1.0],  [1.0, 1.0, 1.0],
                                             [1.0, 1.0, 1.0], [1.0, 1.0, 1.0],  [1.0, 1.0, 1.0]],
                                            [[1.0, 1.0, 1.0], [1.0, 1.0, 111.0], [1.0, 1.0, 1.0],
                                             [1.0, 1.0, 1.0], [1.0, 1.0, 1.0],  [1.0, 1.0, 1.0]]]]).astype(complex)

        self._raw_data = KpointsData(k_vectors=np.asarray([[0.0, 0.0, 0.0]]),
                                     weights=np.asarray([0.3]),
                                     # 6 frequencies, globally frequencies are in hartree units if necessary
                                     # they are converted to cm^-1
                                     frequencies=(np.asarray([[100.0, 200.0, 300.0, 400.0, 500.0, 600.0]])
                                                  * CM1_2_HARTREE),
                                     # 12 atomic displacements
                                     atomic_displacements=atomic_displacements,
                                     unit_cell=np.asarray([[7.44, 0., 0.],
                                                           [0., 9.55, 0.],
                                                           [0., 0., 6.92]]))

    # Use case: TOSCA instrument
    def test_TOSCA(self):
        tosca_params = abins.parameters.instruments['TOSCA']

        # calculate Q for TOSCA
        extracted_raw_data = self._raw_data.extract()

        # convert frequencies to cm^-1 in order to compare
        freq = extracted_raw_data["frequencies"]["0"][FIRST_OPTICAL_PHONON:] / CM1_2_HARTREE
        k2_i = (freq + tosca_params['final_neutron_energy']) * WAVENUMBER_TO_INVERSE_A
        k2_f = tosca_params['final_neutron_energy'] * WAVENUMBER_TO_INVERSE_A
        # noinspection PyTypeChecker
        correct_q_data = k2_i + k2_f - 2 * np.power(k2_i * k2_f, 0.5) * tosca_params['cos_scattering_angle']

        q2 = self._tosca_instrument.calculate_q_powder(freq)

        # noinspection PyTypeChecker
        self.assertEqual(True, np.allclose(correct_q_data, q2))


if __name__ == '__main__':
    unittest.main()
