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
                 quantum_order_events_num=None):
        """
        @param filename: name of input DFT file (CASTEP: foo.phonon)
        @param temperature: temperature in K for which calculation of S should be done
        @param sample_form: form in which experimental sample is: Powder or SingleCrystal (str)
        @param abins_data: object of type AbinsData with data from phonon file
        @param instrument_name: name of instrument (str)
        @param quantum_order_events_num: number of quantum order events taken into account during the simulation
        """
        if not isinstance(temperature, (int, float)):
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

        min_order = AbinsConstants.fundamentals
        max_order = AbinsConstants.fundamentals + AbinsConstants.higher_order_quantum_events
        if isinstance(quantum_order_events_num, int) and min_order <= quantum_order_events_num <= max_order:
            self._quantum_order_events_num = quantum_order_events_num
        else:
            raise ValueError("Invalid number of quantum order events.")

        if instrument_name in AbinsConstants.all_instruments:
            self._instrument_name = instrument_name
        else:
            raise ValueError("Unknown instrument %s" % instrument_name)

        IOmodule.__init__(self,
                          input_filename=filename,
                          group_name=(AbinsParameters.S_data_group + "/" + self._instrument_name + "/" +
                                      self._sample_form + "/%sK" % self._temperature))
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
        _q_calculator = CalculateQ(filename=self._input_filename, instrument=_instrument, sample_form=self._sample_form,
                                   k_points_data=self._abins_data.getKpointsData(),
                                   quantum_order_events_num=self._quantum_order_events_num)

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

        s = {}
        s_frequencies = {}

        # calculate frequencies
        local_freq = fundamentals_freq
        local_coeff = np.eye(local_freq.size, dtype=AbinsConstants.int_type)
        generated_frequencies = []
        # generated_coefficients = []
        generated_reduced_coefficients = []

        for order in range(AbinsConstants.fundamentals, self._quantum_order_events_num + AbinsConstants.s_last_index):

            local_freq, local_coeff = self.construct_freq_combinations(previous_array=local_freq,
                                                                       previous_coefficients=local_coeff,
                                                                       fundamentals_array=fundamentals_freq,
                                                                       quantum_order=order)

            outer_indx = []
            for i in range(local_freq.size):

                inner_indices = []
                for indx, coeff in np.ndenumerate(local_coeff[i]):
                    while coeff != AbinsConstants.empty_slot:
                        inner_indices.append(indx[0])
                        coeff -= 1

                    # we collected all necessary indices
                    if len(inner_indices) == order:
                        break

                outer_indx.append(inner_indices)

            generated_frequencies.append(local_freq)
            # generated_coefficients.append(local_coeff)
            generated_reduced_coefficients.append(np.asarray(outer_indx))

        # calculate s for each atom
        for atom in range(num_atoms):

            for order in range(AbinsConstants.fundamentals, self._quantum_order_events_num + AbinsConstants.s_last_index):

                q2 = extracted_q_data["order_%s" % order]
                order_indx = order - AbinsConstants.python_index_shift

                value_dft = self._calculate_order[order](q2=q2,
                                                         frequencies=generated_frequencies[order_indx],
                                                         indices=generated_reduced_coefficients[order_indx],
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
        trace_ba = np.tensordot(a=b_tensor, b=a_tensor, axes=([1, 2], [0, 1]))
        s = q2 * b_trace / 3.0 * np.exp(-q2 * (a_trace + 2.0 * trace_ba / b_trace) / 5.0)

        return s

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

        dw = np.exp(-q2 * a_trace / 3.0)
        q4 = q2 ** 2
        n_freq = frequencies.size

        s = np.zeros(shape=n_freq, dtype=AbinsConstants.float_type)

        for i in range(n_freq):

            if indices[i, 0] == indices[i, 1]:

                b_ten = b_tensor[indices[i, 0]]
                bb_trace = np.einsum('ij,ji->', b_ten, b_ten)
                s[i] = (np.prod(b_trace[indices[i]]) + 2 * bb_trace) / 30.0

            else:

                ii = indices[i, 0]
                jj = indices[i, 1]
                tr_b0b1 = np.einsum('ij,ji->', b_tensor[ii], b_tensor[jj])
                tr_b1b0 = np.einsum('ij,ji->', b_tensor[jj], b_tensor[ii])
                s[i] = (np.prod(b_trace[indices[i]]) + tr_b0b1 + tr_b1b0) / 15.0

        s *= q4 * dw

        return s

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
        s = 9.0 / 543.0 * q2 ** 3 * np.prod(np.take(b_trace, indices=indices), axis=1) * np.exp(-q2 * a_trace / 3.0)

        return s

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
        s = 27.0 / 9850.0 * q2 ** 4 * np.prod(np.take(b_trace, indices=indices), axis=1) * np.exp(-q2 * a_trace / 3.0)

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

    def calculateData(self):
        """
        Calculates dynamical structure factor S.
        @return: object of type SData and dictionary with total S.
        """
        data = self._calculate_s()
        self.addFileAttributes()
        self.addAttribute(name="order_of_quantum_events", value=self._quantum_order_events_num)
        self.addData("data", data.extract())
        self.save()

        return data

    def loadData(self):
        """
        Loads S from an hdf file.
        @return: object of type SData.
        """
        data = self.load(list_of_datasets=["data"], list_of_attributes=["filename", "order_of_quantum_events"])
        if self._quantum_order_events_num > data["attributes"]["order_of_quantum_events"]:
            raise ValueError("User requested a larger number of quantum events to be included in the simulation "
                             "then in the previous calculations. S cannot be loaded from the hdf file.")
        if self._quantum_order_events_num < data["attributes"]["order_of_quantum_events"]:
            logger.notice("User requested a smaller number of quantum events than in the previous calculations. "
                          "S Data from hdf file which corresponds only to requested quantum order events will be "
                          "loaded.")
            temp_data = {"frequencies": {}, "atoms_data": {}}

            # load necessary frequencies
            for i in range(AbinsConstants.fundamentals, self._quantum_order_events_num + AbinsConstants.s_last_index):
                temp_data["frequencies"]["order_%s" % i] = data["datasets"]["data"]["frequencies"]["order_%s" % i]

            # load atoms_data
            n_atom = len(data["datasets"]["data"]["atoms_data"])
            for i in range(n_atom):
                sort = data["datasets"]["data"]["atoms_data"]["atom_%s" % i]["sort"]
                symbol = data["datasets"]["data"]["atoms_data"]["atom_%s" % i]["symbol"]
                temp_data["atoms_data"]["atom_%s" % i] = {"sort": sort,  "symbol":  symbol, "s": {}}
                for j in range(AbinsConstants.fundamentals,
                               self._quantum_order_events_num + AbinsConstants.s_last_index):
                    temp_val = data["datasets"]["data"]["atoms_data"]["atom_%s" % i]["s"]["order_%s" % j]
                    temp_data["atoms_data"]["atom_%s" % i]["s"] = {"order_%s" % j: temp_val}

            # reduce the data which is loaded to only this data which is required by the user
            data["datasets"]["data"] = temp_data

        s_data = SData(temperature=self._temperature, sample_form=self._sample_form)
        s_data.set(items=data["datasets"]["data"])

        return s_data
