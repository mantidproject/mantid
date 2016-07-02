import numpy

import Constants
from  IOmodule import IOmodule
from QData import  QData


class CalculateQ(IOmodule):
    """
    Class for calculating Q vectors for instrument of choice.
    """
    _all_instruments = ["TOSCA", "None"]

    def __init__(self, filename=None, instrument=None, sample_form=None):
        """
        @param filename: name of input filename (foo.phonon)
        @param instrument: name of instrument
        @param sample_form: form in which sample is (Powder or SingleCrystal)
        """
        super(CalculateQ, self).__init__(input_filename=filename, group_name="Q_vectors")

        if not isinstance(instrument, str):
            raise ValueError("Invalid name of instrument! Please set the name of the instrument before collecting frequencies!")
        self._instrument = instrument

        if not instrument in self._all_instruments:
            raise ValueError("Unsupported instrument!")

        if not sample_form in ["SingleCrystal", "Powder"]:
            raise ValueError("Invalid value of the sample! Please specify one of two options: 'SingleCrystal', 'Powder' ")
        self._sample_form = sample_form

        self._frequencies = None
        self._q_format = False # whether Q vectors are stored as a vectors or scalars

         # _functions and _Q defined in the form of dictionaries with keys as names of instruments. If a name of
        # instrument is set to 'None' then Q vectors does not depend on frequency.
        self._functions = {"TOSCA": self._calculate_Qvectors_Tosca}
        self._Qvectors = None # numpy array with Q vectors

    def  collectFrequencies(self, frequencies=None):
        """
        Collects frequencies.
        @param frequencies: frequencies in the form of numpy array
        """
        if not (isinstance(frequencies, numpy.ndarray) and
                all([isinstance(frequencies[item],numpy.float_) for item in range(frequencies.size)])):
            raise ValueError("Invalid value of frequencies!")



        if self._instrument == "None":
            raise ValueError("Q vectors do not depend on frequency so collecting  frequencies is not needed!")

        self._frequencies = frequencies

    def _calculate_Qvectors_Tosca(self):
        """
        Calculate squared Q vectors for TOSCA and TOSCA-like instruments.
        """
        _freq_squared = self._frequencies * self._frequencies

        if self._sample_form == "Powder":
            self._Qvectors = QData(q_format="scalars")
            self._Qvectors.set(items=_freq_squared * Constants.TOSCA_constant)
        else:
            raise ValueError("SingleCrystal user case is not implemented!")

    def _calculate_Qvectors(self):
        """
        Calculates Q vectors for the given instrument.
        """
        self._functions[self._instrument]()

    def getQvectors(self):
        """
        Calculates Q vectors and return them. Save Q vectors to hdf file.
        @return: Q vectors for the required instrument
        """
        self._calculate_Qvectors()
        self.addNumpyDataset("data", self._Qvectors.extract()) # Q vectors in the form of numpy array
        self.addAttribute("q_format",self._Qvectors._q_format)
        self.addAttribute("instrument", self._instrument)
        self.addAttribute("sample_Form", self._sample_form)
        self.save()

        return self._Qvectors


    def loadData(self):
        data = self.load(list_of_numpy_datasets=["data"], list_of_attributes=["q_format"])
        results = QData(q_format=data["attributes"]["q_format"])
        results.set(data["datasets"]["data"])
        return results
