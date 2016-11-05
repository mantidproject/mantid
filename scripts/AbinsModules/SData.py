from GeneralData import GeneralData
import AbinsConstants

class SData(GeneralData):
    """
    Class for storing S(Q, omega)
    """

    def __init__(self, temperature=None, sample_form=None):
        super(SData, self).__init__()

        if not isinstance(temperature, (float, int)) and temperature > 0:
            raise ValueError("Invalid value of temperature.")
        self._temperature = float(temperature)

        if  sample_form in AbinsConstants.all_sample_forms:
            self._sample_form = sample_form
        else:
            raise  ValueError("Invalid sample form %s"%sample_form)

        self._data = None # dictionary which stores dynamical structure factor for all atoms


    def set(self, items=None):
        """
        Sets a new value for a collection of the data.
        """
        if not isinstance(items, dict):
            raise ValueError("New value of S  should have a form of a list.")

        if sorted(items.keys()) != sorted(AbinsConstants.all_keywords_s_data):
            raise ValueError("Invalid structure of the dictionary.")

        for item in items["atoms_data"]:

            if not isinstance(items["atoms_data"][item], dict):
                raise ValueError("New value of item from S data should have a form of dictionary.")

            if sorted(items["atoms_data"][item].keys()) != sorted(AbinsConstants.all_keywords_atoms_s_data):
                raise ValueError("Invalid structure of the dictionary.")

            if not items["atoms_data"][item]["symbol"] in AbinsConstants.all_symbols:
                raise ValueError("Invalid symbol of element.")

        self._data = items


    def extract(self):
        """
        Returns the data.
        @return: data
        """
        return self._data


    def __str__(self):
        return "Dynamical structure factors data"