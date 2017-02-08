from __future__ import (absolute_import, division, print_function)
import numpy as np
import AbinsModules


class SData(AbinsModules.GeneralData):
    """
    Class for storing S(Q, omega)
    """

    def __init__(self, temperature=None, sample_form=None):
        super(SData, self).__init__()

        if not isinstance(temperature, (float, int)) and temperature > 0:
            raise ValueError("Invalid value of temperature.")
        self._temperature = float(temperature)

        if sample_form in AbinsModules.AbinsConstants.ALL_SAMPLE_FORMS:
            self._sample_form = sample_form
        else:
            raise ValueError("Invalid sample form %s" % sample_form)

        self._data = None  # dictionary which stores dynamical structure factor for all atoms

    def set(self, items=None):
        """
        Sets a new value for a collection of the data.
        """
        if not isinstance(items, dict):
            raise ValueError("New value of S  should have a form of a dict.")

        for item in items:
            if AbinsModules.AbinsConstants.ATOM_LABEL in item:

                if not isinstance(items[item], dict):
                    raise ValueError("New value of item from S data should have a form of dictionary.")

                if sorted(items[item].keys()) != sorted(AbinsModules.AbinsConstants.ALL_KEYWORDS_ATOMS_S_DATA):
                    raise ValueError("Invalid structure of the dictionary.")

                for order in items[item][AbinsModules.AbinsConstants.S_LABEL]:
                    if not isinstance(items[item][AbinsModules.AbinsConstants.S_LABEL][order], np.ndarray):
                        raise ValueError("Numpy array was expected.")

            elif "frequencies" == item:

                bins = np.arange(start=AbinsModules.AbinsParameters.min_wavenumber,
                                 stop=AbinsModules.AbinsParameters.max_wavenumber,
                                 step=AbinsModules.AbinsParameters.bin_width,
                                 dtype=AbinsModules.AbinsConstants.FLOAT_TYPE)

                if not np.array_equal(items[item], bins[1:]):
                    raise ValueError("Invalid frequencies.")

            else:

                raise ValueError("Invalid keyword " + item)

        self._data = items

    def extract(self):
        """
        Returns the data.
        @return: data
        """
        return self._data

    def __str__(self):
        return "Dynamical structure factors data"
