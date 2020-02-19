# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)
import numpy as np
from mantid.kernel import logger as mantid_logger
import AbinsModules
from AbinsModules import AbinsConstants, AbinsParameters


class SData(AbinsModules.GeneralData):
    """
    Class for storing S(Q, omega)
    """

    def __init__(self, temperature=None, sample_form=None):
        super(SData, self).__init__()

        if not isinstance(temperature, (float, int)) and temperature > 0:
            raise ValueError("Invalid value of temperature.")
        self._temperature = float(temperature)

        if sample_form in AbinsConstants.ALL_SAMPLE_FORMS:
            self._sample_form = sample_form
        else:
            raise ValueError("Invalid sample form %s" % sample_form)

        self._data = None  # dictionary which stores dynamical structure factor for all atoms
        self._bin_width = None

    def set_bin_width(self, width=None):
        self._bin_width = width

    def set(self, items=None):
        """
        Sets a new value for a collection of the data.
        """
        if not isinstance(items, dict):
            raise ValueError("New value of S  should have a form of a dict.")

        for item in items:
            if AbinsConstants.ATOM_LABEL in item:

                if not isinstance(items[item], dict):
                    raise ValueError("New value of item from S data should have a form of dictionary.")

                if sorted(items[item].keys()) != sorted(AbinsConstants.ALL_KEYWORDS_ATOMS_S_DATA):
                    raise ValueError("Invalid structure of the dictionary.")

                for order in items[item][AbinsConstants.S_LABEL]:
                    if not isinstance(items[item][AbinsConstants.S_LABEL][order], np.ndarray):
                        raise ValueError("Numpy array was expected.")

            elif "frequencies" == item:
                step = self._bin_width
                bins = np.arange(start=AbinsParameters.sampling['min_wavenumber'],
                                 stop=AbinsParameters.sampling['max_wavenumber'] + step,
                                 step=step,
                                 dtype=AbinsConstants.FLOAT_TYPE)

                freq_points = bins[:-1] + (step / 2)
                if not np.array_equal(items[item], freq_points):
                    raise ValueError("Invalid frequencies, these should correspond to the mid points of the resampled bins.")

            else:
                raise ValueError("Invalid keyword " + item)

        self._data = items

    def extract(self):
        """
        Returns the data.
        :returns: data
        """
        return self._data

    def check_thresholds(self, return_cases=False, logger=None):
        """
        Compare the S data values to minimum thresholds and warn if the threshold appears large relative to the data

        Warnings will be raised if [max(S) * s_relative_threshold] is less than s_absolute_threshold. These
        thresholds are defined in the AbinsParameters.sampling dictionary.

        :param return_cases: If True, return a list of cases where S was small compared to threshold.
        :type return_cases: bool

        :returns: If return_cases=True, this method returns a list of cases which failed the test, as tuples of
            ``(atom_key, order_number, max(S))``. Otherwise, the method returns ``None``.

        """

        if logger is None:
            logger = mantid_logger

        warning_cases = []
        absolute_threshold = AbinsParameters.sampling['s_absolute_threshold']
        relative_threshold = AbinsParameters.sampling['s_relative_threshold']
        for key, entry in self._data.items():
            if AbinsConstants.ATOM_LABEL in key:
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
