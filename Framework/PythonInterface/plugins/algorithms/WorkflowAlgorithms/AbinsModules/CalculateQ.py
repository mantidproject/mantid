from IOmodule import IOmodule


class CalculateQ(IOmodule):
    def __init__(self, filename=None, instrument=None):
        super(IOmodule, self).__init__()
        self._filename = filename
        self._instrument = instrument
        self._frequencies = None
        self._q_vectors = None

        # prepare hdf file
        self._prepare_HDF_file(file_name=filename, group_name="Qvectors")

        self.functions={"TOSCA":self.calculateQvectorsTosca}

    def  collectFrequencies(self, frequencies=None):
        """
        Collect frequencies.
        @param frequencies: frequencies in the form of numpy array
        """


    def calculateQvectorsTosca(self):
        self.functions[self._instrument]()

    def  calculateQvectors(self):
        pass
