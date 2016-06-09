from IOmodule import IOmodule
import numpy
import Constants

class CalculateQ(IOmodule):
    """
    Class for calculating Q vectors for instrument of choice.
    """
    def __init__(self, filename=None, instrument=None, sample_form=None):
        """
        @param filename: name of input filename (foo.phonon)
        @param instrument: name of instrument
        @param sample_form: form in which sample is (Powder or SingleCrystal)
        """
        super(IOmodule, self).__init__()
        self._filename = filename
        self._instrument = instrument
        self._sample_form = sample_form
        self._frequencies = None
        self._q_vectors_squared = None
        self._q_vectors = None

        # prepare hdf file
        self._prepare_HDF_file(file_name=filename, group_name="Qvectors")

        # _functions and _Q defined in the form of dictionaries with keys as names of instruments. If a name of
        # instrument is set to 'None' then Q vectors does not depend on frequency.
        self._functions={"TOSCA": self._calculate_Qvectors_Tosca}
        self._Qvectors={"Powder": self._q_vectors_squared, "SingleCrystal": self._q_vectors}

    def  collectFrequencies(self, frequencies=None):
        """
        Collect frequencies.
        @param frequencies: frequencies in the form of numpy array
        """
        if not (isinstance(frequencies, numpy.ndarray) or
                all([isinstance(frequencies[item],numpy.float_) for item in range(frequencies.size)])):
            raise ValueError("Invalid value of frequencies!")

        if self._instrument is None:
            raise ValueError("Invalid name of instrument! Please set the name of the instrument before collecting frequencies!")

        if self._instrument == "None":
            raise ValueError("Q vectors do not depend on frequency so collecting  frequencies is not needed!")

        self._frequencies = numpy.copy(frequencies)

    def _calculate_Qvectors_Tosca(self):
        """
        Calculate squared Q vectors for TOSCA and TOSCA-like instruments.
        """
        _freq_squared = self._frequencies * self._frequencies

        if self._sample_form == "Powder":
            self._Qvectors[self._sample_form] = _freq_squared * Constants.TOSCA_constant
        else:
            raise ValueError("SingleCrystal user case is not implemented!")

    def _calculate_Qvectors(self):
        """
        Calculate Q vectors for the given instrument.
        """
        self._functions[self._instrument]()

    def getQvectors(self):
        """
        Calculate Q vectors and return them. Save Q vectors to hdf file.
        @return: Q vectors for the required instrument
        """
        self._calculate_Qvectors()
        self.addDataset("Q vectors", self._Qvectors[self._sample_form])
        self.addAttribute("Instrument", self._instrument)
        self.addAttribute("SampleForm", self._sample_form)
        self.save()

        return self._Qvectors[self._sample_form]