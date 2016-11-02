import numpy as np
import math
from copy import copy

from mantid.kernel import logger

from IOmodule import IOmodule
from AbinsData import AbinsData
from QData import QData
from CalculateQ import CalculateQ
from InstrumentProducer import InstrumentProducer
from CalculatePowder import CalculatePowder
from CrystalData import CrystalData
from PowderData import PowderData
from SData import SData
from Instruments import Instrument
from AbinsModules import FrequencyPowderGenerator

import AbinsParameters
import AbinsConstants


class CalculateS(IOmodule, FrequencyPowderGenerator):
    """
    Class for calculating S(Q, omega)
    """

    def __init__(self, filename=None, temperature=None, sample_form=None, abins_data=None, instrument_name=None,
                 overtones=None, combinations=None):
        """
        @param filename: name of input DFT file (CASTEP: foo.phonon)
        @param temperature: temperature in K for which calculation of S should be done
        @param sample_form: form in which experimental sample is: Powder or SingleCrystal (str)
        @param abins_data: object of type AbinsData with data from phonon file
        @param instrument_name: name of instrument (str)
        @param overtones: True if overtones should be included in calculations, otherwise False
        @param combinations: True if combinations should be calculated, otherwise False
        """
        if not (isinstance(temperature, float) or isinstance(temperature, int)):
            raise ValueError("Invalid value of the temperature. Number was expected.")
        if temperature < 0:
            raise ValueError("Temperature cannot be negative.")
        self._temperature = float(temperature)

        if sample_form in AbinsConstants.all_sample_forms:
            self._sample_form = sample_form
        else:
            raise ValueError("Invalid sample form %s" % sample_form)

        if isinstance(abins_data, AbinsData):
            self._abins_data = abins_data
        else:
            raise ValueError("Object of type AbinsData was expected.")

        if isinstance(overtones, bool):
            self._evaluate_overtones = overtones
        else:
            raise ValueError("Invalid value of overtones. Expected values are: True, False.")

        if isinstance(combinations, bool):
            self._evaluate_combinations = combinations
        else:
            raise ValueError("Invalid value of combinations. Expected values are: True, False.")

        if self._evaluate_overtones:
            overtones_folder = "overtones_true"
            if self._evaluate_combinations:
                combinations_folder = "combinations_true"
            else:
                combinations_folder = "combinations_false"
        else:
            overtones_folder = "overtones_false"
            combinations_folder = "combinations_false"

        if instrument_name in AbinsConstants.all_instruments:
            self._instrument_name = instrument_name
        else:
            raise ValueError("Unknown instrument %s" % instrument_name)

        IOmodule.__init__(self,
                          input_filename=filename,
                          group_name=(AbinsParameters.S_data_group + "/" + self._instrument_name + "/" +
                                      self._sample_form + "/%sK" % self._temperature + "/" + overtones_folder + "/" +
                                      combinations_folder))
        FrequencyPowderGenerator.__init__(self)

        self._calculate_order = {AbinsConstants.quantum_order_one: self._calculate_order_one,
                                 AbinsConstants.quantum_order_two: self._calculate_order_two,
                                 AbinsConstants.quantum_order_three: self._calculate_order_three,
                                 AbinsConstants.quantum_order_four: self._calculate_order_four}

    def _calculate_s(self):

        # Produce instrument object
        _instrument_producer = InstrumentProducer()
        _instrument = _instrument_producer.produceInstrument(name=self._instrument_name)

        # Calculate Q
        _q_calculator = CalculateQ(filename=self._input_filename,
                                   instrument=_instrument,
                                   sample_form=self._sample_form,
                                   k_points_data=self._abins_data.getKpointsData(),
                                   overtones=self._evaluate_overtones,
                                   combinations=self._evaluate_combinations)

        _q_vectors = _q_calculator.getData()

        # Powder case: calculate MSD and DW
        if self._sample_form == "Powder":

            _powder_calculator = CalculatePowder(filename=self._input_filename, abins_data=self._abins_data,
                                                 temperature=self._temperature)
            _powder_data = _powder_calculator.getData()
            _s_data = self._calculate_s_powder(q_data=_q_vectors, powder_data=_powder_data, instrument=_instrument)

        # Crystal case: calculate DW
        else:
            raise ValueError("SingleCrystal case not implemented yet.")

        return _s_data

    def _get_gamma_data(self, k_data=None):
        """
        Extracts k points data only for Gamma point.
        @param k_data: numpy array with k-points data.
        @return:
        """
        gamma_pkt_index = -1

        # look for index of Gamma point
        num_k = k_data["k_vectors"].shape[0]
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

    # noinspection PyTypeChecker
    def _calculate_s_powder(self, q_data=None, powder_data=None, instrument=None):
        """
        Calculates S for the powder case.

        @param q_data:  data with squared Q vectors; it is an object of type QData
        @param powder_data: object of type PowderData with mean square displacements and Debye-Waller factors for
                            the case of powder
        @param instrument: object of type Instrument; instance of the instrument for which the whole simulation is
                           performed
        @return: object of type SData with dynamical structure factors for the powder case
        """
        if not isinstance(q_data, QData):
            raise ValueError("Input parameter 'q_data'  should be of type QData.")

        if not isinstance(powder_data, PowderData):
            raise ValueError("Input parameter 'powder_data' should be of type PowderData.")

        if not isinstance(instrument, Instrument):
            raise ValueError("Input parameter 'instrument' should be of type Instrument.")

        atom_items = {}
        powder_atoms_data = powder_data.extract()
        num_atoms = powder_atoms_data["a_tensors"].shape[0]
        a_traces = np.trace(a=powder_atoms_data["a_tensors"], axis1=1, axis2=2)
        b_traces = np.trace(a=powder_atoms_data["b_tensors"], axis1=2, axis2=3)
        extracted_q_data = q_data.extract()
        abins_data_extracted = self._abins_data.extract()
        atom_data = abins_data_extracted["atoms_data"]
        k_points_data = self._get_gamma_data(abins_data_extracted["k_points_data"])
        fundamentals_freq = np.multiply(k_points_data["frequencies"][0][AbinsConstants.first_optical_phonon:],
                                        1.0 / AbinsConstants.cm1_2_hartree)

        if self._evaluate_overtones:
            s_total_dim = AbinsConstants.fundamentals_dim + AbinsConstants.higher_order_quantum_effects_dim
        else:
            s_total_dim = AbinsConstants.fundamentals_dim

        s = {}
        s_frequencies = {}

        # calculate frequencies
        local_freq = fundamentals_freq
        local_coeff = np.eye(local_freq.size, dtype=AbinsConstants.int_type)
        generated_frequencies = []
        generated_coefficients = []

        for order in range(AbinsConstants.fundamentals, s_total_dim + AbinsConstants.s_last_index):

            if self._evaluate_combinations:

                local_freq, local_coeff = self.construct_freq_combinations(previous_array=local_freq,
                                                                           previous_coefficients=local_coeff,
                                                                           fundamentals_array=fundamentals_freq,
                                                                           quantum_order=order)

            else:

                local_freq, local_coeff = self.construct_freq_overtones(fundamentals_array=fundamentals_freq,
                                                                        quantum_order=order)

            generated_frequencies.append(local_freq)
            generated_coefficients.append(local_coeff)

        for atom in range(num_atoms):

            for order in range(AbinsConstants.fundamentals, s_total_dim + AbinsConstants.s_last_index):

                q2 = extracted_q_data["order_%s" % order]
                order_indx = order - AbinsConstants.python_index_shift

                value_dft = self._calculate_order[order](q2=q2,
                                                         frequencies=generated_frequencies[order_indx],
                                                         coefficients=generated_coefficients[order_indx],
                                                         a_tensor=powder_atoms_data["a_tensors"][atom],
                                                         a_trace=a_traces[atom],
                                                         b_tensor=powder_atoms_data["b_tensors"][atom],
                                                         b_trace=b_traces[atom])

                rebined_freq, rebined_spectrum = self._rebin_data(array_x=generated_frequencies[order_indx],
                                                                  array_y=value_dft)

                freq, broad_spectrum = instrument.convolve_with_resolution_function(frequencies=rebined_freq,
                                                                                    s_dft=rebined_spectrum)

                rebined_broad_freq, rebined_broad_spectrum = self._rebin_data(array_x=freq, array_y=broad_spectrum)
                s["order_%s" % order] = rebined_broad_spectrum

                if atom == 1:
                    s_frequencies["order_%s" % order] = rebined_broad_freq

            logger.notice("S for atom %s" % atom + " has been calculated.")

            atom_items["atom_%s" % atom] = {"s": copy(s),
                                            "symbol": atom_data[atom]["symbol"],
                                            "sort": atom_data[atom]["sort"]}

        s_data = SData(temperature=self._temperature, sample_form=self._sample_form)
        s_data.set(items={"atoms_data": atom_items, "frequencies": s_frequencies})

        return s_data

    def _calculate_order_one(self, q2=None, frequencies=None, coefficients=None, a_tensor=None, a_trace=None,
                             b_tensor=None, b_trace=None):
        """
        Calculates S for the first order quantum event for one atom.
        @param q2: squared values of momentum transfer vectors
        @param frequencies: frequencies for which transitions occur
        @param coefficients: array which stores information how transitions can be decomposed in terms fundamentals
        @param a_tensor: total MSD tensor for the given atom
        @param a_trace: total MSD trace for the given atom
        @param b_tensor: frequency dependent MSD tensor for the given atom
        @param b_trace: frequency dependent MSD trace for the given atom
        @return: s for the first quantum order effect for the given atom
        """
        n_freq = frequencies.size
        s = np.zeros(shape=n_freq, dtype=AbinsConstants.float_type)

        for omega in range(n_freq):

            trace_ba = np.trace(np.dot(b_tensor[omega], a_tensor))
            q2_el = q2[omega]

            s[omega] = q2_el * b_trace[omega] / 3.0 * math.exp(
                -q2_el * (a_trace + 2.0 * trace_ba / b_trace[omega]) / 5.0)

        return s

    def _calculate_order_two(self, q2=None, frequencies=None, coefficients=None, a_tensor=None, a_trace=None,
                             b_tensor=None, b_trace=None):
        """
        Calculates S for the second order quantum effect for one atom.


        @param q2: squared values of momentum transfer vectors
        @param frequencies: frequencies for which transitions occur
        @param coefficients: array which stores information how transitions can be decomposed in terms fundamentals
        @param a_tensor: total MSD tensor for the given atom
        @param a_trace: total MSD trace for the given atom
        @param b_tensor: frequency dependent MSD tensor for the given atom
        @param b_trace: frequency dependent MSD trace for the given atom
        @return: s for the second quantum order effect for the given atom
        """
        n_freq = frequencies.size
        s = np.zeros(shape=n_freq, dtype=AbinsConstants.float_type)
        for omega in range(n_freq):

            indices = []

            # extract indices of a and b tensor which should be used to evaluate S for the given transition

            for indx, coeff in np.ndenumerate(coefficients[omega]):
                while coeff != AbinsConstants.empty_slot:
                    indices.append(indx[0])
                    coeff -= 1

                # we collected all necessary indices
                if len(indices) == AbinsConstants.quantum_order_two:
                    break

            q2_w = q2[omega]
            dw = math.exp(-q2_w * a_trace / 3.0)

            if indices[0] == indices[1]:

                i = indices[0]
                b_tr = b_trace[i]
                trace_bb = np.trace(np.dot(b_tensor[i], b_tensor[i]))
                s[omega] = q2_w ** 2 / 30.0 * (b_tr * b_tr + 2 * trace_bb) * dw

            else:

                i = indices[0]
                j = indices[1]
                tr_b0 = b_trace[i]
                tr_b1 = b_trace[j]
                tr_b0b1 = np.trace(np.dot(b_tensor[i], b_tensor[j]))
                tr_b1b0 = np.trace(np.dot(b_tensor[j], b_tensor[i]))
                s[omega] = q2_w ** 2 / 15.0 * (tr_b0 * tr_b1 + tr_b0b1 + tr_b1b0) * dw

        return s

    # def _enhance_overtones(self, frequencies_combinations=None, frequencies_overtones=None):
    #     """
    #     Correction to intensities in case only overtones are selected. Methods counts occurrences of frequencies
    #     which would be used in case of full overtone + combination calculations in bins defined by overtone
    #     frequencies.
    #     @param frequencies_overtones: numpy array with overtone frequencies
    #     @param frequencies_combinations: numpy array with frequencies which would be used in case of combination
    #                                      calculations
    #     @return: numpy array with counts of combination frequencies in bins created by overtone frequencies.
    #     """
    #     if not (isinstance(frequencies_combinations, np.ndarray) and len(frequencies_combinations.shape) == 1):
    #         raise ValueError("Frequencies in the form of one dimentional are expected.")
    #
    #     if not (isinstance(frequencies_overtones, np.ndarray) and len(frequencies_overtones.shape) == 1):
    #         raise ValueError("Frequencies in the form of one dimentional  are expected.")
    #
    #     if frequencies_combinations.size <= frequencies_overtones.size:
    #         raise ValueError("Combinations frequencies should be more then overtones frequencies.")
    #
    #     counts = np.ones(shape=frequencies_combinations.size, dtype=AbinsConstants.float_type)
    #     inds = np.digitize(frequencies_combinations, frequencies_overtones)
    #
    #     return np.asarray([counts[inds == i].sum() for i in range(frequencies_overtones.size)])

    def _calculate_order_three(self, q2=None, frequencies=None, coefficients=None, a_tensor=None, a_trace=None,
                               b_tensor=None, b_trace=None):
        """
        Calculates S for the third order quantum effect for one atom.
        @param q2: squared values of momentum transfer vectors
        @param frequencies: frequencies for which transitions occur
        @param coefficients: array which stores information how transitions can be decomposed in terms fundamentals
        @param a_tensor: total MSD tensor for the given atom
        @param a_trace: total MSD trace for the given atom
        @param b_tensor: frequency dependent MSD tensor for the given atom
        @param b_trace: frequency dependent MSD trace for the given atom
        @return: s for the third quantum order effect for the given atom
        """
        n_freq = frequencies.size
        s = np.zeros(shape=n_freq, dtype=AbinsConstants.float_type)
        for omega in range(n_freq):

            indices = []

            # extract indices of a and b tensor which should be used to evaluate S for the given transition
            for indx, coeff in np.ndenumerate(coefficients[omega]):
                while coeff != AbinsConstants.empty_slot:
                    indices.append(indx[0])
                    coeff -= 1

                # we collected all necessary indices
                if len(indices) == AbinsConstants.quantum_order_three:
                    break
            q2_w = q2[omega]
            dw = math.exp(-q2_w * a_trace / 3.0)
            i = indices[0]
            j = indices[1]
            k = indices[2]

            s[omega] = 9.0 / 543.0 * q2_w ** 3 * b_trace[i] * b_trace[j] * b_trace[k] * dw

        return s

    def _calculate_order_four(self, q2=None, frequencies=None, coefficients=None, a_tensor=None, a_trace=None,
                              b_tensor=None, b_trace=None):
        """
        Calculates S for the fourth order quantum effect for one atom.
        @param q2: q2: squared values of momentum transfer vectors
        @param frequencies: frequencies for which transitions occur
        @param coefficients: array which stores information how transitions can be decomposed in terms fundamentals
        @param a_tensor: total MSD tensor for the given atom
        @param a_trace: total MSD trace for the given atom
        @param b_tensor: frequency dependent MSD tensor for the given atom
        @param b_trace: frequency dependent MSD trace for the given atom
        @return: s for the forth quantum order effect for the given atom
        """
        n_freq = frequencies.size
        s = np.zeros(shape=n_freq, dtype=AbinsConstants.float_type)
        for omega in range(n_freq):

            indices = []

            # extract indices of a and b tensor which should be used to evaluate S for the given transition
            for indx, coeff in np.ndenumerate(coefficients[omega]):
                while coeff != AbinsConstants.empty_slot:
                    indices.append(indx[0])
                    coeff -= 1

                # we collected all necessary indices
                if len(indices) == AbinsConstants.quantum_order_four:
                    break

            q2_w = q2[omega]
            dw = math.exp(-q2_w * a_trace / 3.0)
            i = indices[0]
            j = indices[1]
            k = indices[2]
            l = indices[3]

            s[omega] = 27.0 / 9850.0 * q2_w ** 4 * b_trace[i] * b_trace[j] * b_trace[k] * b_trace[l] * dw

        return s

    def _calculate_s_crystal(self, crystal_data=None):

        if not isinstance(crystal_data, CrystalData):
            raise ValueError("Input parameter should be of type CrystalData.")
            # TODO: implement calculation of S for the single crystal scenario

    def _rebin_data(self, array_x=None, array_y=None):
        """
        Rebins S data so that all quantum effects have the same x-axis.
        @param array_x: numpy array with frequencies
        @param array_y: numpy array with S
        @return: rebined frequencies, rebined S
        """

        bins = np.arange(AbinsParameters.min_wavenumber, AbinsParameters.max_wavenumber, AbinsParameters.bin_width)
        if bins.size > array_x.size:
            return array_x, array_y
        else:

            inds = np.digitize(array_x, bins)
            rebined_y = np.asarray([array_y[inds == i].sum() for i in range(1, bins.size)])
            return bins[1:], rebined_y

    def calculateData(self):
        """
        Calculates dynamical structure factor S.
        @return: object of type SData and dictionary with total S.
        """

        data = self._calculate_s()
        self.addFileAttributes()
        self.addData("data", data.extract())
        self.save()

        return data

    def loadData(self):
        """
        Loads S from an hdf file.
        @return: object of type SData.
        """
        data = self.load(list_of_datasets=["data"], list_of_attributes=["filename"])
        s_data = SData(temperature=self._temperature, sample_form=self._sample_form)
        s_data.set(items=data["datasets"]["data"])

        return s_data
