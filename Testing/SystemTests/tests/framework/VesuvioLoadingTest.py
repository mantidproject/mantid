# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init
"""These are more integration tests as they will require that the test data is available
and that mantid can be imported
"""

import systemtesting

from mantid.api import MatrixWorkspace  # mantid must be imported first to set the script paths correctly
from vesuvio.loading import VesuvioLoadHelper, VesuvioTOFFitInput


class LoadSingleRunSingleSpectrumTest(systemtesting.MantidSystemTest):
    _tof_input = None

    def runTest(self):
        loader = VesuvioLoadHelper("single", "spectrum", "Vesuvio_IP_file_test.par", load_log_files=False)
        self._tof_input = VesuvioTOFFitInput("15039", None, "143", loader)

    def validate(self):
        self.assertFalse(
            self._tof_input.using_back_scattering_spectra,
            "Forward-scattering spectra passed to VesuvioTOFFitInput but 'using_back_scattering_spectra' property is True",
        )
        self.assertTrue(
            isinstance(self._tof_input.sample_data, MatrixWorkspace),
            "Loaded time-of-flight data is not in the form of a MatrixWorkspace, "
            "found type '" + str(type(self._tof_input.sample_data)) + "' instead.",
        )
        self.assertEqual(
            self._tof_input.spectra,
            "143",
            "Passed '143' as spectra to VesuvioTOFFitInput but found different value for spectra field: '" + self._tof_input.spectra + "'.",
        )


class LoadMultipleRunSingleSpectrumTest(systemtesting.MantidSystemTest):
    _tof_input = None

    def runTest(self):
        loader = VesuvioLoadHelper("single", "spectrum", "Vesuvio_IP_file_test.par", load_log_files=False)
        self._tof_input = VesuvioTOFFitInput("15039-15045", None, "143", loader)

    def validate(self):
        self.assertFalse(
            self._tof_input.using_back_scattering_spectra,
            "Forward-scattering spectra passed to VesuvioTOFFitInput but 'using_back_scattering_spectra' property is True",
        )
        self.assertTrue(
            isinstance(self._tof_input.sample_data, MatrixWorkspace),
            "Loaded time-of-flight data is not in the form of a MatrixWorkspace, "
            "found type '" + str(type(self._tof_input.sample_data)) + "' instead.",
        )
        self.assertEqual(
            self._tof_input.spectra,
            "143",
            "Passed '143' as spectra to VesuvioTOFFitInput but found different value for spectra field: '" + self._tof_input.spectra + "'.",
        )


class LoadMultipleRunMultipleSpectrumTest(systemtesting.MantidSystemTest):
    _tof_input = None

    def runTest(self):
        loader = VesuvioLoadHelper("single", "spectrum", "Vesuvio_IP_file_test.par", load_log_files=False)
        self._tof_input = VesuvioTOFFitInput("15039-15045", None, "forward", loader)

    def validate(self):
        self.assertFalse(
            self._tof_input.using_back_scattering_spectra,
            "Forward-scattering spectra passed to VesuvioTOFFitInput but 'using_back_scattering_spectra' property is True",
        )
        self.assertTrue(
            isinstance(self._tof_input.sample_data, MatrixWorkspace),
            "Loaded time-of-flight data is not in the form of a MatrixWorkspace, "
            "found type '" + str(type(self._tof_input.sample_data)) + "' instead.",
        )
        self.assertEqual(
            self._tof_input.spectra,
            "135-198",
            "Passed 'forward' as spectra to VesuvioTOFFitInput, expected '135-198' "
            "but found different value for spectra field: '" + self._tof_input.spectra + "'.",
        )


class LoadBackScatteringSpectrumTest(systemtesting.MantidSystemTest):
    _tof_input = None

    def runTest(self):
        loader = VesuvioLoadHelper("single", "spectrum", "Vesuvio_IP_file_test.par", load_log_files=False)
        self._tof_input = VesuvioTOFFitInput("15039-15045", None, "backward", loader)

    def validate(self):
        self.assertTrue(
            self._tof_input.using_back_scattering_spectra,
            "Back-scattering spectra passed to VesuvioTOFFitInput but 'using_back_scattering_spectra' property is False",
        )
        self.assertTrue(
            isinstance(self._tof_input.sample_data, MatrixWorkspace),
            "Loaded time-of-flight data is not in the form of a MatrixWorkspace, "
            "found type '" + str(type(self._tof_input.sample_data)) + "' instead.",
        )
        self.assertEqual(
            self._tof_input.spectra,
            "3-134",
            "Passed 'backward' as spectra to VesuvioTOFFitInput, expected '3-134' "
            "but found different value for spectra field: '" + self._tof_input.spectra + "'.",
        )
