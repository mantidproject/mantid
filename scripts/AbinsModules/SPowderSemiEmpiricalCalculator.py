from __future__ import (absolute_import, division, print_function)
import AbinsModules
import  gc
try:
    # noinspection PyUnresolvedReferences
    from pathos.multiprocessing import ProcessingPool
    PATHOS_FOUND = True
except ImportError:
    PATHOS_FOUND = False
import numpy as np


# Helper class for handling stability issues with S threshold for one atom and one quantum event.
class StabilityError(Exception):
    def __init__(self, value=None):
        self._value = value

    def __str__(self):
        return self._value


# Helper class for handling stability issues with S threshold for all atoms and all quantum events.
class StabilityErrorAllAtoms(Exception):
    def __init__(self, value=None):
        self._value = value

    def __str__(self):
        return self._value


# noinspection PyMethodMayBeStatic
class SPowderSemiEmpiricalCalculator(object):
    """
    Class for calculating S(Q, omega)
    """

    def __init__(self, filename=None, temperature=None, abins_data=None, instrument=None, quantum_order_num=None):
        """
        @param filename: name of input DFT file (CASTEP: foo.phonon)
        @param temperature: temperature in K for which calculation of S should be done
        @param sample_form: form in which experimental sample is: Powder or SingleCrystal (str)
        @param abins_data: object of type AbinsData with data from phonon file
        @param instrument: name of instrument (str)
        @param quantum_order_num: number of quantum order events taken into account during the simulation
        """
        if not isinstance(temperature, (int, float)):
            raise ValueError("Invalid value of the temperature. Number was expected.")
        if temperature < 0:
            raise ValueError("Temperature cannot be negative.")
        self._temperature = float(temperature)

        self._sample_form = "Powder"

        if isinstance(abins_data, AbinsModules.AbinsData):
            self._abins_data = abins_data
        else:
            raise ValueError("Object of type AbinsData was expected.")
        self._q2_indices = list(self._abins_data.get_kpoints_data().extract()["k_vectors"].keys())
        self._atoms = self._abins_data.get_atoms_data().extract()

        if isinstance(abins_data, AbinsModules.AbinsData):
            self._abins_data = abins_data
        else:
            raise ValueError("Object of type AbinsData was expected.")

        min_order = AbinsModules.AbinsConstants.FUNDAMENTALS
        max_order = AbinsModules.AbinsConstants.FUNDAMENTALS + AbinsModules.AbinsConstants.HIGHER_ORDER_QUANTUM_EVENTS
        if isinstance(quantum_order_num, int) and min_order <= quantum_order_num <= max_order:
            self._quantum_order_num = quantum_order_num
        else:
            raise ValueError("Invalid number of quantum order events.")

        if isinstance(instrument, AbinsModules.Instruments.Instrument):
            self._instrument = instrument
        else:
            raise ValueError("Unknown instrument %s" % instrument)

        if isinstance(filename, str):
            if filename.strip() == "":
                raise ValueError("Name of the file cannot be an empty string!")

            self._input_filename = filename

        else:
            raise ValueError("Invalid name of input file. String was expected!")

        self._clerk = AbinsModules.IOmodule(
            input_filename=filename,
            group_name=(AbinsModules.AbinsParameters.s_data_group + "/%s" % self._instrument + "/" +
                        self._sample_form + "/%sK" % self._temperature))

        self._freq_generator = AbinsModules.FrequencyPowderGenerator()

        self._calculate_order = {AbinsModules.AbinsConstants.QUANTUM_ORDER_ONE: self._calculate_order_one,
                                 AbinsModules.AbinsConstants.QUANTUM_ORDER_TWO: self._calculate_order_two,
                                 AbinsModules.AbinsConstants.QUANTUM_ORDER_THREE: self._calculate_order_three,
                                 AbinsModules.AbinsConstants.QUANTUM_ORDER_FOUR: self._calculate_order_four}

        step = AbinsModules.AbinsParameters.bin_width
        start = AbinsModules.AbinsParameters.min_wavenumber + step
        stop = AbinsModules.AbinsParameters.max_wavenumber + step
        self._bins = np.arange(start=start, stop=stop, step=step, dtype=AbinsModules.AbinsConstants.FLOAT_TYPE)
        self._freq_size = self._bins.size - 1
        self._frequencies = self._bins[:-1]

        self._q_bins = np.linspace(start=AbinsModules.AbinsConstants.Q_BEGIN,
                                   stop=AbinsModules.AbinsConstants.Q_END,
                                   num=AbinsModules.AbinsParameters.q_size)

        # set initial threshold for s for each atom
        self._num_atoms = len(self._abins_data.get_atoms_data().extract())
        s_threshold = AbinsModules.AbinsParameters.s_relative_threshold
        self._s_threshold_ref = np.asarray([s_threshold for _ in range(self._num_atoms)])
        self._s_current_threshold = np.copy(self._s_threshold_ref)
        self._max_s_previous_order = np.asarray([0.0 for _ in range(self._num_atoms)])
        self._total_s_correction_num_attempt = 0

        self._powder_atoms_data = None
        self._a_traces = None
        self._b_traces = None
        self._atoms_data = None
        self._fundamentals_freq = None

    def _calculate_s(self):

        # calculate powder data
        powder_calculator = AbinsModules.CalculatePowder(filename=self._input_filename, abins_data=self._abins_data)
        powder_calculator.get_formatted_data()

        # free memory
        self._abins_data = None
        gc.collect()

        # calculate S
        if self._instrument.get_name() in AbinsModules.AbinsConstants.ONE_DIMENSIONAL_INSTRUMENTS:
            calculate_s_powder = self._calculate_s_powder_1d
        else:
            calculate_s_powder = self._calculate_s_powder_2d

        s_data = calculate_s_powder()

        return s_data

    def _calculate_s_powder_2d(self):
        """
        Calculates 2D S for the powder case.
        :return:  object of type SData with 2D dynamical structure factors for the powder case
        """
        if self._instrument.get_name() not in AbinsModules.AbinsConstants.TWO_DIMENSIONAL_INSTRUMENTS:
            raise ValueError("Instrument for 2D S map was expected.")

        # calculate 2D  S for all incident energies
        atoms_items = self._calculate_s_powder_2d_core()

        # put 2D S into SData object
        s_data = AbinsModules.SData(temperature=self._temperature, sample_form=self._sample_form)
        s_data.set(items=atoms_items)

        return s_data

    def _calculate_s_powder_2d_core(self):
        """
        Helper function for _calculate_s_powder_2d. It calculates S for all incident energies, all q-point, all angles
        and all atoms.
        :return: dictionary with two dimensional S which correspond to all incident energies and all k-points.
        """
        e_init = AbinsModules.AbinsParameters.e_init
        intend = AbinsModules.AbinsConstants.INCIDENT_ENERGY_MESSAGE_INDENTATION

        energy = e_init[0]
        self._report_progress(msg=intend + "Calculation for incident energy %s [cm^-1]" % energy)
        self._instrument.set_incident_energy(e_init=energy)
        data = self._calculate_s_powder_over_k()

        for energy in e_init[1:]:
            self._report_progress(msg=intend + "Calculation for incident energy %s [cm^-1]" % energy)
            self._instrument.set_incident_energy(e_init=energy)
            local_data = self._calculate_s_powder_over_k()
            self._sum_s(current_val=data, addition=local_data)

        data.update({"frequencies": self._frequencies})

        return data

    def _calculate_s_over_threshold(self, s=None, freq=None, coeff=None, atom=None, order=None):
        """
        Discards frequencies for small S.
        :param s: numpy array with S for the given order quantum event and atom
        :param freq: frequencies which correspond to s
        :param coeff: coefficients which correspond to  freq
        :param atom: number of atom
        :param order: order of quantum event
        :return: large enough s, and corresponding freq, coeff and also if calculation is stable
        """
        s_max = np.max(a=s)
        threshold = max(s_max * self._s_current_threshold[atom], AbinsModules.AbinsParameters.s_absolute_threshold)
        small_s = AbinsModules.AbinsConstants.SMALL_S

        if order == AbinsModules.AbinsConstants.FUNDAMENTALS:
            self._max_s_previous_order[atom] = max(s_max, small_s)
        else:
            max_threshold = AbinsModules.AbinsConstants.MAX_THRESHOLD

            is_not_smaller = s_max - self._max_s_previous_order[atom] > small_s
            max_attempts = self._s_current_threshold[atom] < max_threshold

            if is_not_smaller and max_attempts:

                msg = ("Numerical instability detected. Threshold for S has to be increased." +
                       " Current max S is {} and the previous is {} for order {}."
                       .format(s_max, self._max_s_previous_order[atom], order))
                raise StabilityError(msg)
            else:
                self._max_s_previous_order[atom] = max(s_max, small_s)

        indices = s > threshold
        # indices are guaranteed to be a numpy array (but can be an empty numpy array)
        # noinspection PyUnresolvedReferences
        if indices.any():

            freq = freq[indices]
            coeff = coeff[indices]

        else:

            freq = freq[:AbinsModules.AbinsConstants.MIN_SIZE]
            coeff = coeff[:AbinsModules.AbinsConstants.MIN_SIZE]

        return freq, coeff

    def _check_tot_s(self, tot_s=None):
        """
        Checks if total S for each quantum order event is consistent (it is expected that maximum intensity for n-th
        order is larger than maximum intensity for n+1-th order ).
        :param tot_s: dictionary with S for all atoms and all quantum events
        """
        s_temp = np.zeros_like(tot_s["atom_0"]["s"]["order_1"])
        previous_s_max = 0.0
        for order in range(AbinsModules.AbinsConstants.FUNDAMENTALS,
                           self._quantum_order_num + AbinsModules.AbinsConstants.S_LAST_INDEX):
            s_temp.fill(0.0)
            for atom in range(self._num_atoms):
                s_temp += tot_s["atom_{}".format(atom)]["s"]["order_{}".format(order)]
            if order == AbinsModules.AbinsConstants.FUNDAMENTALS:
                previous_s_max = np.max(s_temp)
            else:
                current_s_max = np.max(s_temp)
                if previous_s_max <= current_s_max:
                    raise StabilityErrorAllAtoms(
                        "Numerical instability detected for all atoms for order {}".format(order))
                else:
                    previous_s_max = current_s_max

    def _s_threshold_up(self, atom=None):
        """
        If index of atom is given then sets new higher threshold for S for the given atom. If no atom is specified then
        threshold is increased for all  atoms.
        :param atom: number of atom
        """
        intend = AbinsModules.AbinsConstants.S_THRESHOLD_CHANGE_INDENTATION
        if atom is None:

            self._s_current_threshold = self._s_threshold_ref * 2.0 ** self._total_s_correction_num_attempt
            self._report_progress(
                intend + "Threshold for S has been changed to {} for all atoms."
                .format(self._s_current_threshold[0]) + " S for all atoms will be calculated from scratch.")

        else:

            self._s_current_threshold[atom] *= 2
            atom_symbol = self._atoms["atom_{}".format(atom)]["symbol"]
            self._report_progress(
                intend + "Threshold for S has been changed to {} for atom {}  ({})."
                .format(self._s_current_threshold[atom], atom, atom_symbol) +
                " S for this atom will be calculated from scratch.")

    def _s_threshold_reset(self):
        """
        Reset threshold for S to the initial value.
        """
        self._s_current_threshold = np.copy(self._s_threshold_ref)
        self._total_s_correction_num_attempt = 0

    def _calculate_s_powder_over_k(self):
        """
        Helper function. It calculates S for all q points  and all atoms.
        :return: dictionary with S
        """
        data = self._calculate_s_powder_over_atoms(q_indx=self._q2_indices[0])

        # iterate over remaining q-points
        for q in self._q2_indices[1:]:
            local_data = self._calculate_s_powder_over_atoms(q_indx=q)
            self._sum_s(current_val=data, addition=local_data)
        return data

    def _sum_s(self, current_val=None, addition=None):
        """
        Helper functions which sums S for all atoms and all quantum events taken into account.
        :param current_val: S accumulated so far
        :param addition: S to be added
        """
        for atom in range(self._num_atoms):
            for order in range(AbinsModules.AbinsConstants.FUNDAMENTALS,
                               self._quantum_order_num + AbinsModules.AbinsConstants.S_LAST_INDEX):
                temp = addition["atom_%s" % atom]["s"]["order_%s" % order]
                current_val["atom_%s" % atom]["s"]["order_%s" % order] += temp

    def _calculate_s_powder_1d(self):
        """
        Calculates 1D S for the powder case.

        :return: object of type SData with 1D dynamical structure factors for the powder case
        """
        # calculate data
        data = self._calculate_s_powder_over_k()
        data.update({"frequencies": self._frequencies})

        # put data to SData object
        s_data = AbinsModules.SData(temperature=self._temperature, sample_form=self._sample_form)
        s_data.set(items=data)

        return s_data

    def _calculate_s_powder_over_atoms(self, q_indx=None):
        """
        Evaluates S for all atoms for the given q-point and checks if S is consistent.
        :return: Python dictionary with S data
        """
        self._s_threshold_reset()
        while True:

            try:

                s_all_atoms = self._calculate_s_powder_over_atoms_core(q_indx=q_indx)
                self._check_tot_s(tot_s=s_all_atoms)
                return s_all_atoms

            except StabilityErrorAllAtoms as e:

                self._report_progress("{}".format(e))
                self._total_s_correction_num_attempt += 1
                self._s_threshold_up()

    def _calculate_s_powder_over_atoms_core(self, q_indx=None):
        """
        Helper function for _calculate_s_powder_1d.
        :return: Python dictionary with S data
        """
        atoms_items = {}
        atoms = range(self._num_atoms)
        self._prepare_data(k_point=q_indx)

        if PATHOS_FOUND:
            p_local = ProcessingPool(nodes=AbinsModules.AbinsParameters.threads)
            result = p_local.map(self._calculate_s_powder_one_atom, atoms)
        else:
            result = []
            for atom in atoms:
                result.append(self._calculate_s_powder_one_atom(atom=atom))

        for atom in range(self._num_atoms):
            atoms_items["atom_%s" % atom] = {"s": result[atoms.index(atom)]}
            self._report_progress(msg="S for atom %s" % atom + " has been calculated.")
        return atoms_items

    def _prepare_data(self, k_point=None):
        """
        Sets all necessary fields for 1D calculations. Sorts atom indices to improve parallelism.
        :return: number of atoms, sorted atom indices
        """
        # load powder data for one k
        clerk = AbinsModules.IOmodule(input_filename=self._input_filename,
                                      group_name=AbinsModules.AbinsParameters.powder_data_group)
        powder_data = clerk.load(list_of_datasets=["powder_data"])
        self._a_tensors = powder_data["datasets"]["powder_data"]["a_tensors"][k_point]
        self._b_tensors = powder_data["datasets"]["powder_data"]["b_tensors"][k_point]
        self._a_traces = np.trace(a=self._a_tensors, axis1=1, axis2=2)
        self._b_traces = np.trace(a=self._b_tensors, axis1=2, axis2=3)

        # load dft data for one k point
        clerk = AbinsModules.IOmodule(input_filename=self._input_filename,
                                      group_name=AbinsModules.AbinsParameters.dft_group)
        dft_data = clerk.load(list_of_datasets=["frequencies", "weights"])

        frequencies = dft_data["datasets"]["frequencies"][int(k_point)]
        indx = frequencies > AbinsModules.AbinsConstants.ACOUSTIC_PHONON_THRESHOLD
        self._fundamentals_freq = frequencies[indx]

        self._weight = dft_data["datasets"]["weights"][int(k_point)]

        # free memory
        gc.collect()

    def _report_progress(self, msg):
        """
        @param msg:  message to print out
        """
        # In order to avoid
        #
        # RuntimeError: Pickling of "mantid.kernel._kernel.Logger"
        # instances is not enabled (http://www.boost.org/libs/python/doc/v2/pickle.html)
        #
        # logger has to be imported locally

        from mantid.kernel import logger
        logger.notice(msg)

    def _calculate_s_powder_one_atom(self, atom=None):

        while True:
            try:
                s = self._calculate_s_powder_one_atom_core(atom=atom)
                return s
            except StabilityError as e:
                self._report_progress("{}".format(e))
                self._s_threshold_up(atom=atom)

    def _calculate_s_powder_one_atom_core(self, atom=None):
        """
        @param atom: number of atom
        @return: s, and corresponding frequencies for all quantum events taken into account
        """
        s = {}

        local_freq = np.copy(self._fundamentals_freq)
        local_coeff = np.arange(start=0.0, step=1.0, stop=self._fundamentals_freq.size,
                                dtype=AbinsModules.AbinsConstants.INT_TYPE)
        fund_coeff = np.copy(local_coeff)

        for order in range(AbinsModules.AbinsConstants.FUNDAMENTALS,
                           self._quantum_order_num + AbinsModules.AbinsConstants.S_LAST_INDEX):

            # in case there is large number of transitions chop it into chunks and process chunk by chunk
            if local_freq.size * self._fundamentals_freq.size > AbinsModules.AbinsParameters.optimal_size:

                chunked_fundamentals, chunked_fundamentals_coeff = self._prepare_chunks(local_freq=local_freq,
                                                                                        order=order, s=s)

                for fund_chunk, fund_coeff_chunk in zip(chunked_fundamentals, chunked_fundamentals_coeff):

                    part_loc_freq = np.copy(local_freq)
                    part_loc_coeff = np.copy(local_coeff)

                    # number of transitions can only go up
                    for lg_order in range(order, self._quantum_order_num + AbinsModules.AbinsConstants.S_LAST_INDEX):

                        part_loc_freq, part_loc_coeff, part_broad_spectrum = self._helper_atom(
                            atom=atom, local_freq=part_loc_freq, local_coeff=part_loc_coeff,
                            fundamentals_freq=fund_chunk, fund_coeff=fund_coeff_chunk, order=lg_order)

                        s["order_%s" % lg_order] += part_broad_spectrum

                return s

            # if relatively small array of transitions then process it in one shot
            else:

                local_freq, local_coeff, s["order_%s" % order] = self._helper_atom(
                    atom=atom, local_freq=local_freq, local_coeff=local_coeff,
                    fundamentals_freq=self._fundamentals_freq, fund_coeff=fund_coeff, order=order)

        return s

    def _prepare_chunks(self, local_freq=None, order=None, s=None):
        """
        Helper function for _calculate_s_powder_1d_one_atom in case transitions energies have to be created from
        fundamentals chunks (chunk by chunk).
        :param local_freq: frequency from the previous transition
        :param order:  order of quantum event
        :param s:  dictionary with s data
        :return: 2D numpy array with fundamentals chunks, 2D array with corresponding coefficients
        """
        fund_size = self._fundamentals_freq.size
        l_size = local_freq.size
        opt_size = float(AbinsModules.AbinsParameters.optimal_size)

        chunk_size = max(1.0, np.floor(opt_size / (l_size * 2**(AbinsModules.AbinsConstants.MAX_ORDER - order))))
        chunk_num = int(np.ceil(float(fund_size) / chunk_size))
        new_dim = int(chunk_num * chunk_size)
        new_fundamentals = np.zeros(shape=new_dim, dtype=AbinsModules.AbinsConstants.FLOAT_TYPE)
        new_fundamentals_coeff = np.zeros(shape=new_dim, dtype=AbinsModules.AbinsConstants.INT_TYPE)
        new_fundamentals[:fund_size] = self._fundamentals_freq
        new_fundamentals_coeff[:fund_size] = np.arange(start=0.0, step=1.0, stop=self._fundamentals_freq.size,
                                                       dtype=AbinsModules.AbinsConstants.INT_TYPE)

        new_fundamentals = new_fundamentals.reshape(chunk_num, int(chunk_size))
        new_fundamentals_coeff = new_fundamentals_coeff.reshape(chunk_num, int(chunk_size))

        total_size = self._freq_size
        for lg_order in range(order, self._quantum_order_num + AbinsModules.AbinsConstants.S_LAST_INDEX):
            s["order_%s" % lg_order] = np.zeros(shape=total_size, dtype=AbinsModules.AbinsConstants.FLOAT_TYPE)

        return new_fundamentals, new_fundamentals_coeff

    def _helper_atom(self, atom=None, local_freq=None, local_coeff=None, fundamentals_freq=None, fund_coeff=None,
                     order=None):
        """
        Helper function. It calculates S for one atom, q-index, order and for one
        or more angles (detectors).
        :param atom: number of atom
        :param local_freq: frequency from the previous transition
        :param local_coeff: coefficients from the previous transition
        :param fundamentals_freq: fundamental frequencies
        :param fund_coeff: fundamental coefficients
        :param order: order of quantum event
        """
        local_freq, local_coeff = self._freq_generator.construct_freq_combinations(
            previous_array=local_freq,
            previous_coefficients=local_coeff,
            fundamentals_array=fundamentals_freq,
            fundamentals_coefficients=fund_coeff,
            quantum_order=order)

        if local_freq.any():  # check if local_freq has non-zero values

            if self._instrument.get_name() in AbinsModules.AbinsConstants.ONE_DIMENSIONAL_INSTRUMENTS:

                q2 = self._instrument.calculate_q_powder(input_data=local_freq)
                local_freq, local_coeff, rebined_broad_spectrum = self._helper_atom_angle(
                    atom=atom, local_freq=local_freq, local_coeff=local_coeff, order=order, q2=q2)

            elif self._instrument.get_name() in AbinsModules.AbinsConstants.TWO_DIMENSIONAL_INSTRUMENTS:

                angles = AbinsModules.AbinsParameters.angles
                first_angle = angles[0]
                self._instrument.set_detector_angle(angle=first_angle)
                intend = AbinsModules.AbinsConstants.ANGLE_MESSAGE_INDENTATION
                q2 = self._instrument.calculate_q_powder(input_data=local_freq)
                self._report_progress(msg=intend + "Calculation for the detector at angle %s (atom=%s)" %
                                                   (first_angle, atom))
                opt_local_freq, opt_local_coeff, rebined_broad_spectrum = self._helper_atom_angle(
                    atom=atom, local_freq=local_freq, local_coeff=local_coeff, order=order, q2=q2)

                for angle in angles[1:]:
                    self._report_progress(msg=intend + "Calculation for the detector at angle %s (atom=%s)" %
                                                       (angle, atom))
                    self._instrument.set_detector_angle(angle=angle)
                    q2 = self._instrument.calculate_q_powder(input_data=local_freq)
                    temp = self._helper_atom_angle(atom=atom, local_freq=local_freq, local_coeff=local_coeff,
                                                   order=order, return_freq=False, q2=q2)

                    rebined_broad_spectrum += temp

                local_coeff = opt_local_coeff
                local_freq = opt_local_freq

            else:
                raise ValueError("Unsupported instrument.")

        else:
            rebined_broad_spectrum = self._fix_empty_array()

        # multiply by k-point weight
        rebined_broad_spectrum = rebined_broad_spectrum * self._weight

        return local_freq, local_coeff, rebined_broad_spectrum

    def _helper_atom_angle(self, atom=None, local_freq=None, local_coeff=None, order=None, return_freq=True, q2=None):
        """
        Helper function. It calculates S for one atom, q-index, order and angle (detector).
        In case 2D instrument rebining over q is performed.
        :param q2: squared momentum transfer
        :param atom: number of atom
        :param local_freq: frequency from the previous transition
        :param local_coeff: coefficients from the previous transition
        :param order: order of quantum event
        :param return_freq: if true frequencies and corresponding coefficients are returned together with rebined
                            spectrum; otherwise only rebined spectrum is returned
        :return: (optionally) frequencies and corresponding coefficients are returned together
                 (always) with rebined spectrum
        """
        # calculate discrete S for the given quantum order event
        value_dft = self._calculate_order[order](q2=q2,
                                                 frequencies=local_freq,
                                                 indices=local_coeff,
                                                 a_tensor=self._a_tensors[atom],
                                                 a_trace=self._a_traces[atom],
                                                 b_tensor=self._b_tensors[atom],
                                                 b_trace=self._b_traces[atom])

        # rebin if necessary
        rebined_freq, rebined_spectrum = self._rebin_data_opt(array_x=local_freq, array_y=value_dft)

        # convolve with instrumental resolution
        freq, broad_spectrum = self._instrument.convolve_with_resolution_function(frequencies=rebined_freq,
                                                                                  s_dft=rebined_spectrum)
        # rebin to the fixed size
        rebined_broad_spectrum = self._rebin_data_full(array_x=freq, array_y=broad_spectrum)
        rebined_broad_spectrum = self._fix_empty_array(array_y=rebined_broad_spectrum)

        # in case of 2D instrument rebin over q
        if self._instrument.get_name() in AbinsModules.AbinsConstants.TWO_DIMENSIONAL_INSTRUMENTS:
            temp_full_s = np.zeros(shape=(AbinsModules.AbinsParameters.q_size + 1, rebined_broad_spectrum.size),
                                   dtype=AbinsModules.AbinsConstants.FLOAT_TYPE)

            all_q2 = self._instrument.calculate_q_powder(input_data=self._frequencies)
            all_q = np.sqrt(all_q2)
            shift = AbinsModules.AbinsConstants.PYTHON_INDEX_SHIFT
            bins_q = np.digitize(all_q, self._q_bins) - shift
            small_q_indx = all_q < AbinsModules.AbinsConstants.Q_END
            temp_full_s[bins_q[small_q_indx], small_q_indx] = rebined_broad_spectrum[small_q_indx]
            # the last q-bin stores data outside the range requested by a user so should be neglected
            rebined_broad_spectrum = temp_full_s[:-1]

        # calculate transition energies for construction of higher order quantum event
        local_freq, local_coeff = self._calculate_s_over_threshold(s=value_dft, freq=local_freq,
                                                                   coeff=local_coeff, atom=atom, order=order)

        if return_freq:
            return local_freq, local_coeff, rebined_broad_spectrum
        else:
            return rebined_broad_spectrum

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
        coth = 1.0 / np.tanh(frequencies * AbinsModules.AbinsConstants.CM1_2_HARTREE /
                             (2.0 * self._temperature * AbinsModules.AbinsConstants.K_2_HARTREE))

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
        coth = 1.0 / np.tanh(frequencies * AbinsModules.AbinsConstants.CM1_2_HARTREE /
                             (2.0 * self._temperature * AbinsModules.AbinsConstants.K_2_HARTREE))

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
        coth = 1.0 / np.tanh(frequencies * AbinsModules.AbinsConstants.CM1_2_HARTREE /
                             (2.0 * self._temperature * AbinsModules.AbinsConstants.K_2_HARTREE))
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
        coth = 1.0 / np.tanh(frequencies * AbinsModules.AbinsConstants.CM1_2_HARTREE /
                             (2.0 * self._temperature * AbinsModules.AbinsConstants.K_2_HARTREE))
        s = 27.0 / 9850.0 * q2 ** 4 * np.prod(np.take(b_trace, indices=indices), axis=1) * \
            np.exp(-q2 * a_trace / 3.0 * coth * coth)

        return s

    def _rebin_data_full(self, array_x=None, array_y=None):
        """
        Rebins S data so that all quantum events have the same x-axis. The size of rebined data is equal to _bins.size.
        :param array_x: numpy array with frequencies
        :param array_y: numpy array with S
        :return: rebined frequencies, rebined S
        """
        inds = np.digitize(x=array_x, bins=self._bins) - AbinsModules.AbinsConstants.PYTHON_INDEX_SHIFT
        output_array_y = np.asarray(
            a=[array_y[inds == i].sum() for i in range(self._freq_size)],
            dtype=AbinsModules.AbinsConstants.FLOAT_TYPE)

        return output_array_y

    def _rebin_data_opt(self, array_x=None, array_y=None):
        """
        Rebins S data in optimised way: the size of rebined data may be smaller then _bins.size.
        :param array_x: numpy array with frequencies
        :param array_y: numpy array with S
        :return: rebined frequencies, rebined S
        """
        if self._bins.size > array_x.size:
            output_array_x = array_x
            output_array_y = array_y
        else:
            inds = np.digitize(x=array_x, bins=self._bins) - AbinsModules.AbinsConstants.PYTHON_INDEX_SHIFT
            output_array_x = self._frequencies
            output_array_y = np.asarray(
                a=[array_y[inds == i].sum() for i in range(self._freq_size)],
                dtype=AbinsModules.AbinsConstants.FLOAT_TYPE)

        return output_array_x, output_array_y

    def _fix_empty_array(self, array_y=None):
        """
        Fixes empty numpy arrays which occur in case of heavier atoms.
        :return: numpy array filled with zeros of dimension _bins.size - AbinsConstants.FIRST_BIN_INDEX
        """
        if array_y is None:
            # number of frequencies = self._bins.size - AbinsConstants.FIRST_BIN_INDEX
            output_y = np.zeros(shape=self._bins.size - AbinsModules.AbinsConstants.FIRST_BIN_INDEX,
                                dtype=AbinsModules.AbinsConstants.FLOAT_TYPE)
        elif array_y.any():
            output_y = self._rebin_data_full(array_x=self._bins[AbinsModules.AbinsConstants.FIRST_BIN_INDEX:],
                                             array_y=array_y)
        else:
            output_y = np.zeros(shape=self._bins.size - AbinsModules.AbinsConstants.FIRST_BIN_INDEX,
                                dtype=AbinsModules.AbinsConstants.FLOAT_TYPE)

        return output_y

    def calculate_data(self):
        """
        Calculates dynamical structure factor S.
        @return: object of type SData and dictionary with total S.
        """
        data = self._calculate_s()

        self._clerk.add_file_attributes()
        self._clerk.add_attribute(name="order_of_quantum_events", value=self._quantum_order_num)
        self._clerk.add_data("data", data.extract())
        self._clerk.save()

        return data

    def load_formatted_data(self):
        """
        Loads S from an hdf file.
        @return: object of type SData.
        """
        data = self._clerk.load(list_of_datasets=["data"], list_of_attributes=["filename", "order_of_quantum_events"])
        if self._quantum_order_num > data["attributes"]["order_of_quantum_events"]:
            raise ValueError("User requested a larger number of quantum events to be included in the simulation "
                             "then in the previous calculations. S cannot be loaded from the hdf file.")
        if self._quantum_order_num < data["attributes"]["order_of_quantum_events"]:

            self._report_progress("""
                         User requested a smaller number of quantum events than in the previous calculations.
                         S Data from hdf file which corresponds only to requested quantum order events will be
                         loaded.""")

            temp_data = {"frequencies": data["datasets"]["data"]["frequencies"]}

            # load atoms_data
            n_atom = len([key for key in data["datasets"]["data"].keys() if "atom" in key])
            for i in range(n_atom):
                temp_data["atom_%s" % i] = {"s": dict()}
                for j in range(AbinsModules.AbinsConstants.FUNDAMENTALS,
                               self._quantum_order_num + AbinsModules.AbinsConstants.S_LAST_INDEX):

                    temp_val = data["datasets"]["data"]["atom_%s" % i]["s"]["order_%s" % j]
                    temp_data["atom_%s" % i]["s"].update({"order_%s" % j: temp_val})

            # reduce the data which is loaded to only this data which is required by the user
            data["datasets"]["data"] = temp_data

        s_data = AbinsModules.SData(temperature=self._temperature, sample_form=self._sample_form)
        s_data.set(items=data["datasets"]["data"])

        return s_data

    def get_formatted_data(self):
        """
        Method to obtain data
        @return: obtained data
        """
        try:

            self._clerk.check_previous_data()
            data = self.load_formatted_data()
            self._report_progress(str(data) + " has been loaded from the HDF file.")

        except (IOError, ValueError) as err:

            self._report_progress("Warning: " + str(err) + " Data has to be calculated.")
            data = self.calculate_data()
            self._report_progress(str(data) + " has been calculated.")

        return data
