import numpy as np

# ABINS modules
import AbinsParameters
from IOmodule import IOmodule
from QData import  QData
from KpointsData import KpointsData
from Instruments import Instrument


class CalculateQ(IOmodule):
    """
    Class for calculating Q vectors for instrument of choice.
    """

    def __init__(self, filename=None, instrument=None, sample_form=None):
        """
        @param filename: name of input filename (CASTEP: foo.phonon)
        @param instrument: object of type  Instrument
        @param sample_form: form in which sample is (Powder or SingleCrystal)
        """
        if isinstance(instrument, Instrument):
            self._instrument = instrument
        elif instrument is None:
            self._instrument = "None"
        else:
            raise ValueError("Invalid instrument.")

        if not sample_form in AbinsParameters.all_sample_forms:
            raise ValueError("Invalid value of the sample form. Please specify one of the two options: 'SingleCrystal', 'Powder'.")
        self._sample_form = sample_form

        self._k_points_data = None
        self._Qvectors = None # data with Q vectors

        super(CalculateQ, self).__init__(input_filename=filename, group_name=AbinsParameters.Q_data_group + "/%s"%self._instrument + "/" + self._sample_form)


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

        self._k_points_data = k_points_data


    def _calculate_qvectors_instrument(self):
        """
        Calculates Q vectors for the given instrument.
        """

        self._Qvectors = QData(frequency_dependence=True)
        _k_data = self._k_points_data.extract()
        num_k = _k_data["k_vectors"].shape[0]
        self._Qvectors.set_k(k=num_k)

        if self._sample_form == "Powder":
            for k in range(num_k):
                self._Qvectors._append(self._instrument.calculate_q(frequencies=np.multiply(_k_data["frequencies"][k], 1.0 / AbinsParameters.cm1_2_hartree)))
        else:
            raise ValueError("SingleCrystal user case is not implemented.")


    def calculateData(self):
        """
        Calculates Q vectors and return them. Saves Q vectors to an hdf file.
        @return: Q vectors for the required instrument
        """
        if isinstance(self._instrument, Instrument):
            self._calculate_qvectors_instrument()
        else:
            raise ValueError("General case of Q data not implemented yet.")

        self.addNumpyDataset("data", self._Qvectors.extract()) # Q vectors in the form of numpy array
        self.addFileAttributes()
        self.save()

        return self._Qvectors


    def loadData(self):
        """
        Loads  Q data from hdf file.
        @return: QData object
        """
        data = self.load(list_of_numpy_datasets=["data"])
        freq =  self._instrument != "None"
        results = QData(frequency_dependence=freq)

        if freq: results.set_k(k=data["datasets"]["data"].shape[0])

        results.set(data["datasets"]["data"])

        return results
