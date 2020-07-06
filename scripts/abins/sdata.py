# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from typing import Optional, Union
import numpy as np
from numbers import Real

from mantid.kernel import logger as mantid_logger
import abins
from abins.constants import ALL_KEYWORDS_ATOMS_S_DATA, ALL_SAMPLE_FORMS, ATOM_LABEL, FLOAT_TYPE, S_LABEL


class SData:
    """
    Class for storing S(Q, omega) with relevant metadata
    """

    def __init__(self, *,
                 data: dict, frequencies: np.ndarray,
                 temperature: Optional[float] = None,
                 sample_form: str = '',
                 ) -> None:
        super().__init__()

        if temperature is None:
            self._temperature = None
        elif isinstance(temperature, Real):
            self._temperature = float(temperature)
        else:
            raise TypeError("Temperature must be a real number or None")

        if isinstance(sample_form, str):
            self._sample_form = sample_form
        else:
            raise TypeError("Sample form must be a string. Use '' (default) if unspecified.")

        self._frequencies = np.asarray(frequencies, dtype=FLOAT_TYPE)
        self._check_frequencies()

        self._data = data
        self._check_data()

    def get_frequencies(self) -> np.ndarray:
        return self._frequencies.copy()

    def get_temperature(self) -> Union[float, None]:
        return self._temperature

    def get_sample_form(self) -> str:
        return self._sample_form

    def get_bin_width(self) -> Union[float, None]:
        """Check frequency series and return the bin size

        If the frequency series does not have a consistent step size, return None
        """

        self._check_frequencies()
        step_size = (self._frequencies[-1] - self._frequencies[0]) / (self._frequencies.size - 1)

        if np.allclose(step_size, self._frequencies[1:] - self._frequencies[:-1]):
            return step_size
        else:
            return None

    def check_finite_temperature(self):
        """Raise an error if Temperature is not greater than zero"""
        temperature = self.get_temperature()
        if not (isinstance(temperature, (float, int)) and temperature > 0):
            raise ValueError("Invalid value of temperature.")

    def check_known_sample_form(self):
        """Raise an error if sample form is not known to Abins"""
        sample_form = self.get_sample_form()
        if sample_form not in ALL_SAMPLE_FORMS:
            raise ValueError(
                f"Invalid sample form {sample_form}: known sample forms are {ALL_SAMPLE_FORMS}")

    def _check_frequencies(self):
        # Check frequencies are ordered low to high
        if not np.allclose(np.sort(self._frequencies),
                           self._frequencies):
            raise ValueError("Frequencies not sorted low to high")

    def _check_data(self):
        """Check data set is consistent and has correct types"""
        if not isinstance(self._data, dict):
            raise ValueError("New value of S  should have a form of a dict.")

        for key, item in self._data.items():
            if ATOM_LABEL in key:
                if not isinstance(item, dict):
                    raise ValueError("New value of item from S data should have a form of dictionary.")

                if sorted(item.keys()) != sorted(ALL_KEYWORDS_ATOMS_S_DATA):
                    raise ValueError("Invalid structure of the dictionary.")

                for order in item[S_LABEL]:
                    if not isinstance(item[S_LABEL][order], np.ndarray):
                        raise ValueError("Numpy array was expected.")

            else:
                raise ValueError("Invalid keyword " + item)

    def extract(self):
        """
        Returns the data.
        :returns: data
        """
        full_data = self._data
        full_data.update({'frequencies': self._frequencies})
        return full_data

    def check_thresholds(self, return_cases=False, logger=None):
        """
        Compare the S data values to minimum thresholds and warn if the threshold appears large relative to the data

        Warnings will be raised if [max(S) * s_relative_threshold] is less than s_absolute_threshold. These
        thresholds are defined in the abins.parameters.sampling dictionary.

        :param return_cases: If True, return a list of cases where S was small compared to threshold.
        :type return_cases: bool

        :returns: If return_cases=True, this method returns a list of cases which failed the test, as tuples of
            ``(atom_key, order_number, max(S))``. Otherwise, the method returns ``None``.

        """

        if logger is None:
            logger = mantid_logger

        warning_cases = []
        absolute_threshold = abins.parameters.sampling['s_absolute_threshold']
        relative_threshold = abins.parameters.sampling['s_relative_threshold']
        for key, entry in self._data.items():
            if ATOM_LABEL in key:
                for order, s in entry['s'].items():
                    if max(s.flatten()) * relative_threshold < absolute_threshold:
                        warning_cases.append((key, order, max(s.flatten())))

        if len(warning_cases) > 0:
            logger.warning("Warning: some contributions had small S compared to threshold.")
            logger.warning("The minimum S threshold ({}) is greater than {}% of the "
                           "maximum S for the following:".format(absolute_threshold,
                                                                 relative_threshold * 100))

            # Sort the warnings by atom number, order number
            # Assuming that keys will be of form "atom_1", "atom_2", ...
            # and "order_1", "order_2", ...
            def int_key(case):
                key, order, _ = case
                return (int(key.split('_')[-1]), int(order.split('_')[-1]))

            for case in sorted(warning_cases, key=int_key):
                logger.warning("{0}, {1}: max S {2:10.4E}".format(*case))

        if return_cases:
            return warning_cases
        else:
            return None

    def __str__(self):
        return "Dynamical structure factors data"
