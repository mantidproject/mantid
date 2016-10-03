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

    def __init__(self, filename=None, instrument=None, sample_form=None, k_points_data=None, overtones=None):
        """
        @param filename: name of input filename (CASTEP: foo.phonon)
        @param instrument: object of type  Instrument
        @param sample_form: form in which sample is (Powder or SingleCrystal)
        @param k_points_data: object of type KpointsData with data from DFT calculations
        @param overtones: True if overtones should be included in calculations, otherwise False
        """
        if not isinstance(instrument, Instrument):
            raise ValueError("Invalid instrument.")
        self._instrument = instrument
            
        if not sample_form in AbinsParameters.all_sample_forms:
            raise ValueError("Invalid value of the sample form. Please specify one of the two options: 'SingleCrystal', 'Powder'.")
        self._sample_form = sample_form

        if not isinstance(k_points_data, KpointsData):
            raise ValueError("Invalid value of k-points data.")
        self._k_points_data = k_points_data

        if isinstance(overtones, bool):
            self._overtones = overtones
        else:
            raise ValueError("Invalid value of overtones. Expected values are: True, False ")

        if self._overtones:
            overtones_folder = "overtones_true"
        else:
            overtones_folder = "overtones_false"

        self._Qvectors = None # data with Q vectors

        super(CalculateQ, self).__init__(input_filename=filename, group_name=AbinsParameters.Q_data_group + "/%s"%self._instrument + "/" + self._sample_form + "/" + overtones_folder)


    def _calculate_qvectors_instrument(self):
        """
        Calculates Q vectors for the given instrument.
        """
        num_k = self._k_points_data.extract()["k_vectors"].shape[0]
        self._Qvectors = QData(num_k=num_k, overtones=self._overtones)
        self._instrument.collect_K_data(k_points_data=self._k_points_data)
        if self._sample_form == "Powder":
            self._Qvectors.set(self._instrument.calculate_q_powder(overtones=self._overtones))
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
            raise ValueError("Invalid instrument.")

        self.addData("data", self._Qvectors.extract()) # Q vectors in the form of numpy array
        self.addFileAttributes()
        self.save()

        return self._Qvectors


    def loadData(self):
        """
        Loads  Q data from hdf file.
        @return: QData object
        """
        data = self.load(list_of_datasets=["data"])
        results = QData(num_k = self._k_points_data.extract()["k_vectors"].shape[0])
        results.set(data["datasets"]["data"])

        return results
