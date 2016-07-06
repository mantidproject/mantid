import numpy

# ABINS modules
import Constants
from IOmodule import IOmodule
from QData import  QData
from KpointsData import  KpointsData


class CalculateQ(IOmodule):
    """
    Class for calculating Q vectors for instrument of choice.
    """

    def __init__(self, filename=None, instrument=None, sample_form=None):
        """
        @param filename: name of input filename (CASTEP: foo.phonon)
        @param instrument: name of instrument
        @param sample_form: form in which sample is (Powder or SingleCrystal)
        """
        super(CalculateQ, self).__init__(input_filename=filename, group_name=Constants.Q_data_group)

        if not instrument in Constants.all_instruments:
            raise ValueError("Unsupported instrument")
        self._instrument = instrument

        if not sample_form in Constants.all_sample_forms:
            raise ValueError("Invalid value of the sample form. Please specify one of the two options: 'SingleCrystal', 'Powder'.")
        self._sample_form = sample_form

        self._k_points_data = None

         # _functions  defined in the form of dictionary with keys as names of instruments. If a name of
        #  instrument is set to 'None' then Q vectors do not depend on frequency.
        self._functions = {"TOSCA": self._calculate_qvectors_tosca}
        self._Qvectors = None # data with Q vectors

    def  collectFrequencies(self, k_points_data=None):
        """
        Collects frequencies.
        @param k_points_data: frequencies in the form of numpy array
        """

        if self._instrument == "None":
            raise ValueError("Q vectors do not depend on frequency so collecting  frequencies is not needed.")
        
        if not isinstance(k_points_data, KpointsData):
            raise ValueError("Invalid value of k-points data.")
        k_points_data._num_k = len(k_points_data._data)

        self._k_points_data = k_points_data.extract()


    def _calculate_qvectors_tosca(self, frequencies=None):
        """
        Calculates squared Q vectors for TOSCA and TOSCA-like instruments.
        """
        _freq_squared = frequencies * frequencies
        if self._sample_form == "Powder":
            self._Qvectors.append(item=_freq_squared * Constants.TOSCA_constant)
        else:
            raise ValueError("SingleCrystal user case is not implemented.")


    def _calculate_qvectors_instrument(self):
        """
        Calculates Q vectors for the given instrument.
        """

        self._Qvectors = QData(frequency_dependence=True)
        num_k = len(self._k_points_data)
        self._Qvectors.set_k(k=num_k)
        for k in range(num_k):
            self._functions[self._instrument](self._k_points_data[k]["frequencies"])


    def getQvectors(self):
        """
        Calculates Q vectors and return them. Saves Q vectors to an hdf file.
        @return: Q vectors for the required instrument
        """
        if self._instrument != "None":
            self._calculate_qvectors_instrument()
        else:
            raise ValueError("General case of Q data not implemented yet.")

        self.addNumpyDataset("data", self._Qvectors.extract()) # Q vectors in the form of numpy array
        self.addAttribute("frequency_dependence",self._Qvectors._frequency_dependence)
        self.addAttribute("instrument", self._instrument)
        self.addAttribute("sample_Form", self._sample_form)
        self.addAttribute("filename", self._input_filename)
        self.save()

        return self._Qvectors


    def loadData(self):
        """
        Loads  Q data from hdf file.
        @return: QData object
        """
        data = self.load(list_of_numpy_datasets=["data"], list_of_attributes=["frequency_dependence"])
        freq = data["attributes"]["frequency_dependence"]
        results = QData(frequency_dependence=freq)

        if freq: results.set_k(k=data["datasets"]["data"].shape[0])

        results._data = data["datasets"]["data"]

        return results
