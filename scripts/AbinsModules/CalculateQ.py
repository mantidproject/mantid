import numpy as np
from mantid.kernel import logger

# ABINS modules
import AbinsParameters
import AbinsConstants
from IOmodule import IOmodule
from QData import QData
from KpointsData import KpointsData
from Instruments import Instrument


class CalculateQ(IOmodule):
    """
    Class for calculating Q vectors for instrument of choice.
    """

    def __init__(self, filename=None, instrument=None, sample_form=None, k_points_data=None,
                 quantum_order_events_num=None):
        """
        @param filename: name of input filename (CASTEP: foo.phonon)
        @param instrument: object of type  Instrument
        @param sample_form: form in which sample is (Powder or SingleCrystal)
        @param k_points_data: object of type KpointsData with data from DFT calculations
        @param quantum_order_events_num: number of quantum order events taken into account during the simulation
        """
        if not isinstance(instrument, Instrument):
            raise ValueError("Invalid instrument.")
        self._instrument = instrument
            
        if sample_form not in AbinsConstants.all_sample_forms:
            raise ValueError("Invalid value of the sample form. Please specify one of the "
                             "two options: 'SingleCrystal', 'Powder'.")
        self._sample_form = sample_form

        if not isinstance(k_points_data, KpointsData):
            raise ValueError("Invalid value of k-points data.")
        self._k_points_data = k_points_data

        min_order = AbinsConstants.fundamentals
        max_order = AbinsConstants.fundamentals + AbinsConstants.higher_order_quantum_events
        if isinstance(quantum_order_events_num, int) and min_order <= quantum_order_events_num <= max_order:
            self._quantum_order_events_num = quantum_order_events_num
        else:
            raise ValueError("Invalid value of quantum order events.")

        self._Qvectors = None  # data with Q vectors

        super(CalculateQ, self).__init__(input_filename=filename,
                                         group_name=AbinsParameters.Q_data_group +
                                         "/%s" % self._instrument + "/" + self._sample_form)

    def _get_gamma_data(self, k_data_obj=None):
        """
        Extracts k points data only for Gamma point.
        @param k_data_obj:  object of type KpointsData
        @return: KpointsData object only with data for Gamma point
        """
        if not isinstance(k_data_obj, KpointsData):
            raise ValueError("Invalid value of k-points data.")

        gamma_pkt_index = -1
        k_data = k_data_obj.extract()
        num_k = k_data["k_vectors"].shape[0]
        # look for index of Gamma point
        for k in range(num_k):
            if np.linalg.norm(k_data["k_vectors"][k]) < AbinsConstants.small_k:
                gamma_pkt_index = k
                break
        if gamma_pkt_index == -1:
            raise ValueError("Gamma point not found.")

        k_points = {"weights": np.asarray([k_data["weights"][gamma_pkt_index]]),
                    "k_vectors": np.asarray([k_data["k_vectors"][gamma_pkt_index]]),
                    "frequencies": np.asarray([k_data["frequencies"][gamma_pkt_index]]),
                    "atomic_displacements": np.asarray([k_data["atomic_displacements"][gamma_pkt_index]])}

        return k_points

    def _calculate_qvectors_instrument(self):
        """
        Calculates Q vectors for the given instrument.
        """
        if self._sample_form == "Powder":
            self._instrument.collect_K_data(k_points_data=self._get_gamma_data(k_data_obj=self._k_points_data))
            self._Qvectors = QData(quantum_order_events_num=self._quantum_order_events_num)
            self._Qvectors.set(self._instrument.calculate_q_powder(
                quantum_order_events_num=self._quantum_order_events_num))
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

        self.addData("data", self._Qvectors.extract())  # Q vectors in the form of numpy array
        self.addFileAttributes()
        self.addAttribute(name="order_of_quantum_events", value=self._quantum_order_events_num)
        self.save()

        return self._Qvectors

    def loadData(self):
        """
        Loads  Q data from hdf file.
        @return: QData object
        """
        data = self.load(list_of_datasets=["data"], list_of_attributes=["order_of_quantum_events"])
        if self._quantum_order_events_num > data["attributes"]["order_of_quantum_events"]:
            raise ValueError("User requested a larger number of quantum events to be included in the simulation "
                             "then in the previous calculations. Q data cannot be loaded from the hdf file.")
        if self._quantum_order_events_num < data["attributes"]["order_of_quantum_events"]:
            logger.notice("User requested a smaller number of quantum events than in the previous calculations. "
                          "Q data from hdf file which corresponds only to requested quantum order events will be "
                          "loaded.")
            temp_data = {}
            for j in range(AbinsConstants.fundamentals,
                           self._quantum_order_events_num + AbinsConstants.s_last_index):
                temp_data["order_%s" % j] = data["datasets"]["data"]["order_%s" % j]
            data["datasets"]["data"] = temp_data

        results = QData(quantum_order_events_num=self._quantum_order_events_num)
        results.set(data["datasets"]["data"])

        return results
