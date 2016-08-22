from GeneralData import GeneralData
import AbinsParameters

class SData(GeneralData):
    """
    Class for storing S(Q, omega)
    """

    def __init__(self, temperature=None, sample_form=None):
        super(SData, self).__init__()

        if not ((isinstance(temperature, float) or isinstance(temperature, int)) and temperature > 0):
            raise ValueError("Invalid value of temperature.")
        self._temperature = temperature

        if  sample_form in AbinsParameters.all_sample_forms:
            self._sample_form = sample_form
        else:
            raise  ValueError("Invalid sample form %s"%sample_form)

        self._data = None # dictionary which stores dynamical structure factor for each atom


    def set(self, items=None):
        """
        Sets a new value for a collection of the data.
        """
        if not isinstance(items, dict):
            raise ValueError("New value of S  should have a form of a dictionary.")

        if sorted(items.keys()) != sorted(AbinsParameters.all_keywords_s_data):
            raise ValueError("Invalid structure of the dictionary.")

        for item in items["atoms_data"]:

            if not isinstance(item, dict):
                raise ValueError("New value of item from S data should have a form of dictionary.")

            if sorted(item.keys()) != sorted(AbinsParameters.all_keywords_s_sub_data):
                raise ValueError("Invalid structure of the dictionary.")


            if not item["symbol"] in AbinsParameters.all_symbols:
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