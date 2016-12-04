import numpy as np
from pathos.multiprocessing import ProcessingPool
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

        self._powder_atoms_data = None
        self._a_traces = None
        self._b_traces = None
        self._atoms_data = None
        self._fundamentals_freq = None

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

    def _calculate_s_over_threshold(self, s=None, freq=None, coeff=None):
        """
        Discards small S and corresponding frequencies.
        @param s: numpy array with S for the given order quantum event and atom
        @param freq: frequencies which correspond to s
        @param coeff: coefficients which correspond to  freq
        @return: large enough s, and corresponding freq, coeff
        """
        threshold = max(np.max(a=s) * AbinsParameters.s_relative_threshold, AbinsParameters.s_absolute_threshold)

        indices = s > threshold
        s = s[indices]
        freq = freq[indices]
        coeff = coeff[indices]

        return s, freq, coeff

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

        self._powder_atoms_data = powder_data.extract()
        num_atoms = self._powder_atoms_data["a_tensors"].shape[0]
        self._a_traces = np.trace(a=self._powder_atoms_data["a_tensors"], axis1=1, axis2=2)
        self._b_traces = np.trace(a=self._powder_atoms_data["b_tensors"], axis1=2, axis2=3)
        abins_data_extracted = self._abins_data.extract()
        self._atoms_data = abins_data_extracted["atoms_data"]
        k_points_data = self._get_gamma_data(abins_data_extracted["k_points_data"])
        self._fundamentals_freq = k_points_data["frequencies"][0][AbinsConstants.FIRST_OPTICAL_PHONON:]

        atoms_items = dict()
        atoms = range(num_atoms)

        p_local = ProcessingPool(ncpus=AbinsParameters.atoms_threads)
        result = p_local.map(self._calculate_s_powder_1d_one_atom, atoms)

        for atom in range(num_atoms):

            atoms_items["atom_%s" % atom] = {"frequencies": result[atom][0],
                                             "s": result[atom][1],
                                             "symbol": self._atoms_data[atom]["symbol"],
                                             "sort": self._atoms_data[atom]["sort"]}

            self._report_progress(msg="S for atom %s" % atom + " has been calculated.")

        s_data = SData(temperature=self._temperature, sample_form=self._sample_form)
        s_data.set(items=atoms_items)

        return s_data

    def _report_progress(self, msg):
        """
        @param msg:  message to print out
        """
        from mantid.kernel import logger
        logger.notice(msg)

    def _calculate_s_powder_1d_one_atom(self, atom=None):
        """
        @param atom: number of atom
        @return: s, and corresponding frequencies for all quantum events taken into account
        """
        s = {}
        s_frequencies = {}

        local_freq = np.copy(self._fundamentals_freq)
        local_coeff = np.arange(start=0.0, step=1.0, stop=self._fundamentals_freq.size, dtype=AbinsConstants.INT_TYPE)
        fund_coeff = np.copy(local_coeff)

        for order in range(AbinsConstants.FUNDAMENTALS, self._quantum_order_num + AbinsConstants.S_LAST_INDEX):

            # in case there is large number of transitions chop it into chunks and process chunk by chunk
            if local_freq.size * self._fundamentals_freq.size > AbinsConstants.LARGE_SIZE:

                fund_size = self._fundamentals_freq.size
                l_size = local_freq.size
                opt_size = float(AbinsParameters.optimal_size)

                chunk_size = max(1.0, np.floor(opt_size / l_size))
                chunk_num = int(np.ceil(float(fund_size) / chunk_size))
                new_dim = int(chunk_num * chunk_size)
                new_fundamentals = np.zeros(shape=new_dim, dtype=AbinsConstants.FLOAT_TYPE)
                new_fundamentals_coeff = np.zeros(shape=new_dim, dtype=AbinsConstants.INT_TYPE)
                new_fundamentals[:fund_size] = self._fundamentals_freq
                new_fundamentals_coeff[:fund_size] = fund_coeff

                new_fundamentals = new_fundamentals.reshape(chunk_num, int(chunk_size))
                new_fundamentals_coeff = new_fundamentals_coeff.reshape(chunk_num, int(chunk_size))

                total_size = int(np.ceil((AbinsParameters.max_wavenumber - AbinsParameters.min_wavenumber) /
                                         AbinsParameters.bin_width)) - 1

                # large number of transition energies so always  freq.size > partial_broad_freq.size
                rebined_broad_freq = np.arange(start=AbinsParameters.min_wavenumber,
                                               stop=AbinsParameters.max_wavenumber,
                                               step=AbinsParameters.bin_width,
                                               dtype=AbinsConstants.FLOAT_TYPE)

                for lg_order in range(order, self._quantum_order_num + AbinsConstants.S_LAST_INDEX):
                    s["order_%s" % lg_order] = np.zeros(shape=total_size, dtype=AbinsConstants.FLOAT_TYPE)
                    s_frequencies["order_%s" % lg_order] = rebined_broad_freq

                for fund_chunk, fund_coeff_chunk in zip(new_fundamentals, new_fundamentals_coeff):

                    part_local_freq = np.copy(local_freq)
                    part_local_coeff = np.copy(local_coeff)

                    # number of transitions can only go up
                    for lg_order in range(order, self._quantum_order_num + AbinsConstants.S_LAST_INDEX):

                        part_local_freq, part_local_coeff = \
                            self.construct_freq_combinations(previous_array=part_local_freq,
                                                             previous_coefficients=part_local_coeff,
                                                             fundamentals_array=fund_chunk,
                                                             fundamentals_coefficients=fund_coeff_chunk,
                                                             quantum_order=lg_order)

                        q2 = self._instrument._calculate_q_powder(frequencies=part_local_freq)

                        part_value_dft = \
                            self._calculate_order[lg_order](q2=q2,
                                                            frequencies=part_local_freq,
                                                            indices=part_local_coeff,
                                                            a_tensor=self._powder_atoms_data["a_tensors"][atom],
                                                            a_trace=self._a_traces[atom],
                                                            b_tensor=self._powder_atoms_data["b_tensors"][atom],
                                                            b_trace=self._b_traces[atom])

                        part_value_dft, part_local_freq, part_local_coeff = \
                            self._calculate_s_over_threshold(s=part_value_dft,
                                                             freq=part_local_freq,
                                                             coeff=part_local_coeff)

                        part_freq, part_spectrum = self._rebin_data(array_x=part_local_freq, array_y=part_value_dft)

                        part_freq, part_broad_spectrum = \
                            self._instrument.convolve_with_resolution_function(frequencies=part_freq,
                                                                               s_dft=part_spectrum)

                        part_broad_freq, part_broad_spectrum = \
                            self._rebin_data(array_x=part_freq, array_y=part_broad_spectrum)

                        if part_broad_spectrum.size > 0:
                            s["order_%s" % lg_order] = s["order_%s" % lg_order] + part_broad_spectrum

                return s_frequencies, s

            # if relatively small array of transitions then process it in one shot
            else:

                local_freq, local_coeff = self.construct_freq_combinations(previous_array=local_freq,
                                                                           previous_coefficients=local_coeff,
                                                                           fundamentals_array=self._fundamentals_freq,
                                                                           fundamentals_coefficients=fund_coeff,
                                                                           quantum_order=order)

                q2 = self._instrument._calculate_q_powder(frequencies=local_freq)

                value_dft = self._calculate_order[order](q2=q2,
                                                         frequencies=local_freq,
                                                         indices=local_coeff,
                                                         a_tensor=self._powder_atoms_data["a_tensors"][atom],
                                                         a_trace=self._a_traces[atom],
                                                         b_tensor=self._powder_atoms_data["b_tensors"][atom],
                                                         b_trace=self._b_traces[atom])

                value_dft, local_freq, local_coeff = self._calculate_s_over_threshold(s=value_dft,
                                                                                      freq=local_freq,
                                                                                      coeff=local_coeff)

                rebined_freq, rebined_spectrum = self._rebin_data(array_x=local_freq, array_y=value_dft)

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

                       np.einsum('kli, kil->k',
                       np.take(b_tensor, indices=indices[:, 0], axis=0),
                       np.take(b_tensor, indices=indices[:, 1], axis=0)) +

                       np.einsum('kli, kil->k',
                       np.take(b_tensor, indices=indices[:, 1], axis=0),
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

        bins = np.arange(start=AbinsParameters.min_wavenumber,
                         stop=AbinsParameters.max_wavenumber,
                         step=AbinsParameters.bin_width,
                         dtype=AbinsConstants.FLOAT_TYPE)
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

            self._report_progress("""
                         User requested a smaller number of quantum events than in the previous calculations.
                         S Data from hdf file which corresponds only to requested quantum order events will be
                         loaded.""")

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
