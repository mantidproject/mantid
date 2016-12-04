import numpy as np
from copy import copy

from mantid.kernel import logger

from IOmodule import IOmodule
from AbinsData import AbinsData
from InstrumentProducer import InstrumentProducer
from CalculatePowder import CalculatePowder
from CrystalData import CrystalData
from PowderData import PowderData
from SData import SData
from AbinsModules import FrequencyPowderGenerator

import AbinsParameters
import AbinsConstants


class CalculateS(IOmodule, FrequencyPowderGenerator):
    """
    Class for calculating S(Q, omega)
    """

    def __init__(self, filename=None, temperature=None, sample_form=None, abins_data=None, instrument_name=None,
                 quantum_order_num=None):
        """
        @param filename: name of input DFT file (CASTEP: foo.phonon)
        @param temperature: temperature in K for which calculation of S should be done
        @param sample_form: form in which experimental sample is: Powder or SingleCrystal (str)
        @param abins_data: object of type AbinsData with data from phonon file
        @param instrument_name: name of instrument (str)
        @param quantum_order_num: number of quantum order events taken into account during the simulation
        """
        if not isinstance(temperature, (int, float)):
            raise ValueError("Invalid value of the temperature. Number was expected.")
        if temperature < 0:
            raise ValueError("Temperature cannot be negative.")
        self._temperature = float(temperature)

        if sample_form in AbinsConstants.ALL_SAMPLE_FORMS:
            self._sample_form = sample_form
        else:
            raise ValueError("Invalid sample form %s" % sample_form)

        if isinstance(abins_data, AbinsData):
            self._abins_data = abins_data
        else:
            raise ValueError("Object of type AbinsData was expected.")

        min_order = AbinsConstants.FUNDAMENTALS
        max_order = AbinsConstants.FUNDAMENTALS + AbinsConstants.HIGHER_ORDER_QUANTUM_EVENTS
        if isinstance(quantum_order_num, int) and min_order <= quantum_order_num <= max_order:
            self._quantum_order_num = quantum_order_num
        else:
            raise ValueError("Invalid number of quantum order events.")

        if instrument_name in AbinsConstants.ALL_INSTRUMENTS:
            self._instrument_name = instrument_name
            _instrument_producer = InstrumentProducer()
            self._instrument = _instrument_producer.produce_instrument(name=self._instrument_name)

        else:
            raise ValueError("Unknown instrument %s" % instrument_name)

        IOmodule.__init__(self,
                          input_filename=filename,
                          group_name=(AbinsParameters.s_data_group + "/" + self._instrument_name + "/" +
                                      self._sample_form + "/%sK" % self._temperature))
        FrequencyPowderGenerator.__init__(self)

        self._calculate_order = {AbinsConstants.QUANTUM_ORDER_ONE: self._calculate_order_one,
                                 AbinsConstants.QUANTUM_ORDER_TWO: self._calculate_order_two,
                                 AbinsConstants.QUANTUM_ORDER_THREE: self._calculate_order_three,
                                 AbinsConstants.QUANTUM_ORDER_FOUR: self._calculate_order_four}

    def _calculate_s(self):

        # Powder case: calculate A and B tensors
        if self._sample_form == "Powder":

            _powder_calculator = CalculatePowder(filename=self._input_filename, abins_data=self._abins_data)
            _powder_data = _powder_calculator.get_data()
            _s_data = self._calculate_s_powder_1d(powder_data=_powder_data)

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
            if np.linalg.norm(k_data["k_vectors"][k]) < AbinsConstants.SMALL_K:
                gamma_pkt_index = k
                break
        if gamma_pkt_index == -1:
            raise ValueError("Gamma point not found.")

        k_points = {"weights": np.asarray([k_data["weights"][gamma_pkt_index]]),
                    "k_vectors": np.asarray([k_data["k_vectors"][gamma_pkt_index]]),
                    "frequencies": np.asarray([k_data["frequencies"][gamma_pkt_index]]),
                    "atomic_displacements": np.asarray([k_data["atomic_displacements"][gamma_pkt_index]])}
        return k_points

    def _calculate_s_powder_1d(self, powder_data=None):
        """
        Calculates S for the powder case.

        @param powder_data: object of type PowderData with mean square displacements and Debye-Waller factors for
                            the case of powder
        @return: object of type SData with dynamical structure factors for the powder case
        """
        if not isinstance(powder_data, PowderData):
            raise ValueError("Input parameter 'powder_data' should be of type PowderData.")

        atom_items = {}
        powder_atoms_data = powder_data.extract()
        num_atoms = powder_atoms_data["a_tensors"].shape[0]
        a_traces = np.trace(a=powder_atoms_data["a_tensors"], axis1=1, axis2=2)
        b_traces = np.trace(a=powder_atoms_data["b_tensors"], axis1=2, axis2=3)
        abins_data_extracted = self._abins_data.extract()
        atom_data = abins_data_extracted["atoms_data"]
        k_points_data = self._get_gamma_data(abins_data_extracted["k_points_data"])
        fundamentals_freq = np.multiply(k_points_data["frequencies"][0][AbinsConstants.FIRST_OPTICAL_PHONON:],
                                        1.0 / AbinsConstants.CM1_2_HARTREE)

        # calculate s for each atom
        for atom in range(num_atoms):

            s_frequencies, s = self._calculate_s_powder_1d_one_atom(atom=atom, powder_atoms_data=powder_atoms_data,
                                                                    a_traces=a_traces, b_traces=b_traces,
                                                                    fundamentals_freq=fundamentals_freq)

            logger.notice("S for atom %s" % atom + " has been calculated.")

            atom_items["atom_%s" % atom] = {"s": copy(s),
                                            "frequencies": s_frequencies,
                                            "symbol": atom_data[atom]["symbol"],
                                            "sort": atom_data[atom]["sort"]}

        s_data = SData(temperature=self._temperature, sample_form=self._sample_form)
        s_data.set(items=atom_items)

        return s_data

    def _calculate_s_powder_1d_one_atom(self, atom=None, powder_atoms_data=None, a_traces=None,
                                        b_traces=None, fundamentals_freq=None):
        """

        @param atom: number of atom
        @param powder_atoms_data:  powder data
        @param a_traces: a_traces for all atoms
        @param b_traces: b_traces for all atoms
        @param fundamentals_freq: fundamental frequencies
        @return: s, and corresponding frequencies for all quantum events taken into account
        """
        s = {}
        s_frequencies = {}

        local_freq = np.copy(fundamentals_freq)
        local_coeff = np.arange(fundamentals_freq.size, dtype=AbinsConstants.INT_TYPE)

        for order in range(AbinsConstants.FUNDAMENTALS, self._quantum_order_num + AbinsConstants.S_LAST_INDEX):

            local_freq, local_coeff = self.construct_freq_combinations(previous_array=local_freq,
                                                                       previous_coefficients=local_coeff,
                                                                       fundamentals_array=fundamentals_freq,
                                                                       quantum_order=order)

            q2 = self._instrument._calculate_q_powder(frequencies=local_freq)

            value_dft = self._calculate_order[order](q2=q2,
                                                     frequencies=local_freq,
                                                     indices=local_coeff,
                                                     a_tensor=powder_atoms_data["a_tensors"][atom],
                                                     a_trace=a_traces[atom],
                                                     b_tensor=powder_atoms_data["b_tensors"][atom],
                                                     b_trace=b_traces[atom])

            # neglect S below S_THRESHOLD
            indices = value_dft > AbinsConstants.S_THRESHOLD
            value_dft = value_dft[indices]
            local_freq = local_freq[indices]
            local_coeff = local_coeff[indices]

            rebined_freq, rebined_spectrum = self._rebin_data(array_x=local_freq,
                                                              array_y=value_dft)

            freq, broad_spectrum = self._instrument.convolve_with_resolution_function(frequencies=rebined_freq,
                                                                                      s_dft=rebined_spectrum)

            rebined_broad_freq, rebined_broad_spectrum = self._rebin_data(array_x=freq, array_y=broad_spectrum)
            s["order_%s" % order] = rebined_broad_spectrum
            s_frequencies["order_%s" % order] = rebined_broad_freq

        return s_frequencies, s

    # noinspection PyUnusedLocal
    def _calculate_order_one(self, q2=None, frequencies=None, indices=None, a_tensor=None, a_trace=None,
                             b_tensor=None, b_trace=None):
        """
        Calculates S for the first order quantum event for one atom.
        @param q2: squared values of momentum transfer vectors
        @param frequencies: frequencies for which transitions occur
        @param indices: array which stores information how transitions can be decomposed in terms of fundamentals
        @param a_tensor: total MSD tensor for the given atom
        @param a_trace: total MSD trace for the given atom
        @param b_tensor: frequency dependent MSD tensor for the given atom
        @param b_trace: frequency dependent MSD trace for the given atom
        @return: s for the first quantum order event for the given atom
        """
        trace_ba = np.einsum('kli, il->k', b_tensor, a_tensor)
        coth = 1.0 / np.tanh(frequencies * AbinsConstants.CM1_2_HARTREE /
                             (2.0 * self._temperature * AbinsConstants.K_2_HARTREE))

        s = q2 * b_trace / 3.0 * np.exp(-q2 * (a_trace + 2.0 * trace_ba / b_trace) / 5.0 * coth * coth)

        return s

    # noinspection PyUnusedLocal
    def _calculate_order_two(self, q2=None, frequencies=None, indices=None, a_tensor=None, a_trace=None,
                             b_tensor=None, b_trace=None):
        """
        Calculates S for the second order quantum event for one atom.


        @param q2: squared values of momentum transfer vectors
        @param frequencies: frequencies for which transitions occur
        @param indices: array which stores information how transitions can be decomposed in terms of fundamentals
        @param a_tensor: total MSD tensor for the given atom
        @param a_trace: total MSD trace for the given atom
        @param b_tensor: frequency dependent MSD tensor for the given atom
        @param b_trace: frequency dependent MSD trace for the given atom
        @return: s for the second quantum order event for the given atom
        """
        coth = 1.0 / np.tanh(frequencies * AbinsConstants.CM1_2_HARTREE /
                             (2.0 * self._temperature * AbinsConstants.K_2_HARTREE))

        dw = np.exp(-q2 * a_trace / 3.0 * coth * coth)
        q4 = q2 ** 2

        # in case indices are the same factor is 2 otherwise it is 1
        factor = (indices[:, 0] == indices[:, 1]) + 1

        # Explanation of used symbols in aCLIMAX manual p. 15

        # num_freq -- total number of transition energies
        # indices[num_freq]
        # b_trace[num_freq]
        # b_tensor[num_freq, 3, 3]
        # factor[num_freq]

        # Tr B_v_i * Tr B_v_k ->  np.prod(np.take(b_trace, indices=indices), axis=1)
        #
        # Operation ":" is a contraction of tensors
        # B_v_i : B_v_k ->
        # np.einsum('kli, kil->k', np.take(b_tensor, indices=indices[:, 0], axis=0),
        # np.take(b_tensor, indices=indices[:, 1], axis=0))
        #
        # B_v_k : B_v_i ->
        # np.einsum('kli, kil->k', np.take(b_tensor, indices=indices[:, 1], axis=0),
        # np.take(b_tensor, indices=indices[:, 0], axis=0)))

        s = q4 * dw * (np.prod(np.take(b_trace, indices=indices), axis=1) +

                       np.einsum('kli, kil->k', np.take(b_tensor, indices=indices[:, 0], axis=0),
                       np.take(b_tensor, indices=indices[:, 1], axis=0)) +

                       np.einsum('kli, kil->k', np.take(b_tensor, indices=indices[:, 1], axis=0),
                       np.take(b_tensor, indices=indices[:, 0], axis=0))) / (15.0 * factor)

        return s

    # noinspection PyUnusedLocal,PyUnusedLocal
    def _calculate_order_three(self, q2=None, frequencies=None, indices=None, a_tensor=None, a_trace=None,
                               b_tensor=None, b_trace=None):
        """
        Calculates S for the third order quantum event for one atom.
        @param q2: squared values of momentum transfer vectors
        @param frequencies: frequencies for which transitions occur
        @param indices: array which stores information how transitions can be decomposed in terms of fundamentals
        @param a_tensor: total MSD tensor for the given atom
        @param a_trace: total MSD trace for the given atom
        @param b_tensor: frequency dependent MSD tensor for the given atom
        @param b_trace: frequency dependent MSD trace for the given atom
        @return: s for the third quantum order event for the given atom
        """
        coth = 1.0 / np.tanh(frequencies * AbinsConstants.CM1_2_HARTREE /
                             (2.0 * self._temperature * AbinsConstants.K_2_HARTREE))
        s = 9.0 / 543.0 * q2 ** 3 * np.prod(np.take(b_trace, indices=indices), axis=1) * \
            np.exp(-q2 * a_trace / 3.0 * coth * coth)

        return s

    # noinspection PyUnusedLocal
    def _calculate_order_four(self, q2=None, frequencies=None, indices=None, a_tensor=None, a_trace=None,
                              b_tensor=None, b_trace=None):
        """
        Calculates S for the fourth order quantum event for one atom.
        @param q2: q2: squared values of momentum transfer vectors
        @param frequencies: frequencies for which transitions occur
        @param indices: array which stores information how transitions can be decomposed in terms of fundamentals
        @param a_tensor: total MSD tensor for the given atom
        @param a_trace: total MSD trace for the given atom
        @param b_tensor: frequency dependent MSD tensor for the given atom
        @param b_trace: frequency dependent MSD trace for the given atom
        @return: s for the forth quantum order event for the given atom
        """
        coth = 1.0 / np.tanh(frequencies * AbinsConstants.CM1_2_HARTREE /
                             (2.0 * self._temperature * AbinsConstants.K_2_HARTREE))
        s = 27.0 / 9850.0 * q2 ** 4 * np.prod(np.take(b_trace, indices=indices), axis=1) * \
            np.exp(-q2 * a_trace / 3.0 * coth * coth)

        return s

    def _calculate_s_crystal(self, crystal_data=None):

        if not isinstance(crystal_data, CrystalData):
            raise ValueError("Input parameter should be of type CrystalData.")
            # TODO: implement calculation of S for the single crystal scenario

    def _rebin_data(self, array_x=None, array_y=None):
        """
        Rebins S data so that all quantum events have the same x-axis.
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

    def calculate_data(self):
        """
        Calculates dynamical structure factor S.
        @return: object of type SData and dictionary with total S.
        """
        data = self._calculate_s()
        self.add_file_attributes()
        self.add_attribute(name="order_of_quantum_events", value=self._quantum_order_num)
        self.add_data("data", data.extract())
        self.save()

        return data

    def load_data(self):
        """
        Loads S from an hdf file.
        @return: object of type SData.
        """
        data = self.load(list_of_datasets=["data"], list_of_attributes=["filename", "order_of_quantum_events"])
        if self._quantum_order_num > data["attributes"]["order_of_quantum_events"]:
            raise ValueError("User requested a larger number of quantum events to be included in the simulation "
                             "then in the previous calculations. S cannot be loaded from the hdf file.")
        if self._quantum_order_num < data["attributes"]["order_of_quantum_events"]:
            logger.notice("User requested a smaller number of quantum events than in the previous calculations. "
                          "S Data from hdf file which corresponds only to requested quantum order events will be "
                          "loaded.")

            temp_data = {}

            # load atoms_data
            n_atom = len(data["datasets"]["data"])
            for i in range(n_atom):
                sort = data["datasets"]["data"]["atom_%s" % i]["sort"]
                symbol = data["datasets"]["data"]["atom_%s" % i]["symbol"]
                temp_data["atom_%s" % i] = {"sort": sort,  "symbol":  symbol, "s": {}, "frequencies": {}}
                for j in range(AbinsConstants.FUNDAMENTALS, self._quantum_order_num + AbinsConstants.S_LAST_INDEX):

                    temp_val = data["datasets"]["data"]["atom_%s" % i]["s"]["order_%s" % j]
                    temp_data["atom_%s" % i]["s"]["order_%s" % j] = temp_val

                    temp_val = data["datasets"]["data"]["atom_%s" % i]["frequencies"]["order_%s" % j]
                    temp_data["atom_%s" % i]["frequencies"]["order_%s" % j] = temp_val

            # reduce the data which is loaded to only this data which is required by the user
            data["datasets"]["data"] = temp_data

        s_data = SData(temperature=self._temperature, sample_form=self._sample_form)
        s_data.set(items=data["datasets"]["data"])

        return s_data
