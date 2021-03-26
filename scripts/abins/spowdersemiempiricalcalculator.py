# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import gc
import numpy as np
from typing import List, Optional, Tuple, Union

import abins
from abins.constants import (ANGLE_MESSAGE_INDENTATION, CM1_2_HARTREE, INT_TYPE, K_2_HARTREE, FLOAT_TYPE, FUNDAMENTALS,
                             HIGHER_ORDER_QUANTUM_EVENTS, MAX_ORDER, MIN_SIZE, ONE_DIMENSIONAL_INSTRUMENTS,
                             TWO_DIMENSIONAL_INSTRUMENTS, QUANTUM_ORDER_ONE, QUANTUM_ORDER_TWO, QUANTUM_ORDER_THREE,
                             QUANTUM_ORDER_FOUR, S_LAST_INDEX)
from abins.instruments import Instrument
from abins.sdata import SData, SDataByAngle
from mantid.api import Progress


# noinspection PyMethodMayBeStatic
class SPowderSemiEmpiricalCalculator:
    """
    Class for calculating S(Q, omega)
    """
    def __init__(self,
                 filename=None,
                 temperature=None,
                 abins_data=None,
                 instrument=None,
                 quantum_order_num=None,
                 bin_width=1.0):
        """
        :param filename: name of input DFT file (CASTEP: foo.phonon)
        :param temperature: temperature in K for which calculation of S should be done
        :param sample_form: form in which experimental sample is: Powder or SingleCrystal (str)
        :param abins_data: object of type AbinsData with data from phonon file
        :param instrument: name of instrument (str)
        :param quantum_order_num: number of quantum order events taken into account during the simulation
        :param bin_width: bin width used in rebining in wavenumber
        """
        if not isinstance(temperature, (int, float)):
            raise ValueError("Invalid value of the temperature. Number was expected.")
        if temperature < 0:
            raise ValueError("Temperature cannot be negative.")
        self._temperature = float(temperature)

        self._sample_form = "Powder"

        if isinstance(abins_data, abins.AbinsData):
            self._abins_data = abins_data
        else:
            raise ValueError("Object of type AbinsData was expected.")

        self._num_k = len(self._abins_data.get_kpoints_data())

        min_order = FUNDAMENTALS
        max_order = FUNDAMENTALS + HIGHER_ORDER_QUANTUM_EVENTS
        if isinstance(quantum_order_num, int) and min_order <= quantum_order_num <= max_order:
            self._quantum_order_num = quantum_order_num
        else:
            raise ValueError("Invalid number of quantum order events.")

        if isinstance(instrument, Instrument):
            self._instrument = instrument
        else:
            raise ValueError("Unknown instrument %s" % instrument)

        if isinstance(filename, str):
            if filename.strip() == "":
                raise ValueError("Name of the file cannot be an empty string!")

            self._input_filename = filename

        else:
            raise ValueError("Invalid name of input file. String was expected!")

        self._clerk = abins.IO(input_filename=filename,
                               setting=self._instrument.get_setting(),
                               group_name=("{s_data_group}/{instrument}/{sample_form}/{temperature}K").format(
                                   s_data_group=abins.parameters.hdf_groups['s_data'],
                                   instrument=self._instrument,
                                   sample_form=self._sample_form,
                                   temperature=self._temperature))

        self._freq_generator = abins.FrequencyPowderGenerator()
        self._calculate_order = {
            QUANTUM_ORDER_ONE: self._calculate_order_one,
            QUANTUM_ORDER_TWO: self._calculate_order_two,
            QUANTUM_ORDER_THREE: self._calculate_order_three,
            QUANTUM_ORDER_FOUR: self._calculate_order_four
        }

        self._bins = np.arange(start=abins.parameters.sampling['min_wavenumber'],
                               stop=abins.parameters.sampling['max_wavenumber'] + bin_width,
                               step=bin_width,
                               dtype=FLOAT_TYPE)
        self._frequencies = self._bins[:-1] + (bin_width / 2)
        self._freq_size = self._bins.size - 1

        self._num_atoms = len(self._abins_data.get_atoms_data())

        self._progress_reporter = None
        self._a_traces = None
        self._b_traces = None
        self._fundamentals_freq = None

    def _calculate_s(self):

        # calculate powder data
        powder_calculator = abins.PowderCalculator(filename=self._input_filename, abins_data=self._abins_data)
        powder_calculator.get_formatted_data()

        # calculate S
        calculate_s_powder = None
        if self._instrument.get_name() in ONE_DIMENSIONAL_INSTRUMENTS:
            calculate_s_powder = self._calculate_s_powder_1d
        elif self._instrument.get_name() in TWO_DIMENSIONAL_INSTRUMENTS:
            calculate_s_powder = self._calculate_s_powder_2d
        else:
            raise ValueError('Instrument "{}" is not recognised, cannot perform semi-empirical '
                             'powder averaging.'.format(self._instrument.get_name()))

        s_data = calculate_s_powder()

        return s_data

    def _calculate_s_powder_1d(self) -> SData:
        """
        Calculates 1D S for the powder case.

        :returns: object of type SData with 1D dynamical structure factors for the powder case
        """
        if self.progress_reporter:
            self.progress_reporter.setNumSteps(self._num_k * self._num_atoms + 1)
        s_data = self._calculate_s_powder_over_k().sum_over_angles(average=True)

        return s_data

    def _calculate_s_powder_2d(self) -> SData:
        raise NotImplementedError('2D instruments not supported in this version.')

    def _calculate_s_over_threshold(self, s=None, freq=None, coeff=None):
        """
        Discards frequencies for small S.
        :param s: numpy array with S for the given order quantum event and atom
        :param freq: frequencies which correspond to s
        :param coeff: coefficients which correspond to  freq

        :returns: freq, coeff corresponding to S greater than abins.parameters.sampling['s_absolute_threshold']
        """

        indices = s > abins.parameters.sampling['s_absolute_threshold']

        # Mask out small values, but avoid returning an array smaller than MIN_SIZE
        if np.count_nonzero(indices) >= MIN_SIZE:
            freq = freq[indices]
            coeff = coeff[indices]

        else:
            freq = freq[:MIN_SIZE]
            coeff = coeff[:MIN_SIZE]

        return freq, coeff

    def _get_empty_data(self):
        return SDataByAngle.get_empty(angles=self._instrument.get_angles(),
                                      frequencies=self._frequencies,
                                      atom_keys=list(self._abins_data.get_atoms_data().extract().keys()),
                                      order_keys=[f'order_{n}' for n in range(1, self._quantum_order_num + 1)],
                                      temperature=self._temperature,
                                      sample_form=self._sample_form)

    def _calculate_s_powder_over_k(self, existing_data: Optional[SDataByAngle] = None) -> SDataByAngle:
        """
        Helper function. It calculates S for all q points and all atoms.

        :param existing_data: If provided, add new results to this object.

        :returns: SDataByAngle
        """
        angle_resolved_data = existing_data if existing_data else self._get_empty_data()

        for q_index in range(self._num_k):
            _ = self._calculate_s_powder_over_atoms(q_indx=q_index, existing_data=angle_resolved_data)
        return angle_resolved_data

    def _calculate_s_powder_over_atoms(self,
                                       *,
                                       q_indx: int,
                                       existing_data: Optional[SDataByAngle] = None) -> SDataByAngle:
        """
        Evaluates S for all atoms for the given q-point and checks if S is consistent.
        :param q_indx: Index of q-point from calculated phonon data
        :existing_data: If provided, results will be summed to this existing object

        :returns: SDataByAngle
        """
        self._prepare_data(k_point=q_indx)

        s_by_atom = existing_data if existing_data else self._get_empty_data()

        for atom_index in range(self._num_atoms):
            self._calculate_s_powder_one_atom(atom=atom_index, q_index=q_indx, existing_data=s_by_atom)
            self._report_progress(msg=f"S for atom {atom_index} has been calculated at qpt {q_indx}.",
                                  reporter=self.progress_reporter)

        return s_by_atom

    def _prepare_data(self, k_point=None):
        """
        Sets all necessary fields for 1D calculations. Sorts atom indices to improve parallelism.
        :returns: number of atoms, sorted atom indices
        """
        # load powder data for one k
        clerk = abins.IO(input_filename=self._input_filename, group_name=abins.parameters.hdf_groups['powder_data'])
        powder_data = abins.PowderData.from_extracted(
            clerk.load(list_of_datasets=["powder_data"])["datasets"]["powder_data"])
        self._a_tensors = powder_data.get_a_tensors()[k_point]
        self._b_tensors = powder_data.get_b_tensors()[k_point]

        self._a_traces = np.trace(a=self._a_tensors, axis1=1, axis2=2)
        self._b_traces = np.trace(a=self._b_tensors, axis1=2, axis2=3)

        self._fundamentals_freq = powder_data.get_frequencies()[k_point]

        # load dft data to get k-point weighting
        clerk = abins.IO(input_filename=self._input_filename, group_name=abins.parameters.hdf_groups['ab_initio_data'])
        dft_data = clerk.load(list_of_datasets=["frequencies", "weights"])
        self._weight = dft_data["datasets"]["weights"][k_point]

        # free memory
        gc.collect()

    @property
    def progress_reporter(self) -> Union[None, Progress]:
        return self._progress_reporter

    @progress_reporter.setter
    def progress_reporter(self, progress_reporter) -> None:
        if isinstance(progress_reporter, (Progress, type(None))):
            self._progress_reporter = progress_reporter
        else:
            raise TypeError("Progress reporter type should be mantid.api.Progress. " "If unavailable, use None.")

    @staticmethod
    def _report_progress(msg: str, reporter: Union[None, Progress] = None, notice: bool = False) -> None:
        """
        :param msg:  message to print out
        :param reporter:  Progress object for visual feedback in Workbench
        :param notice:  Log at "notice" level (i.e. visible by default)

        """
        # In order to avoid
        #
        # RuntimeError: Pickling of "mantid.kernel._kernel.Logger"
        # instances is not enabled (http://www.boost.org/libs/python/doc/v2/pickle.html)
        #
        # logger has to be imported locally

        from mantid.kernel import logger

        if reporter:
            reporter.report(msg)

        if notice:
            logger.notice(msg)
        else:
            logger.information(msg)

    def _calculate_s_powder_one_atom(self,
                                     atom=None,
                                     q_index=None,
                                     existing_data: Optional[SDataByAngle] = None) -> SDataByAngle:
        """
        :param atom: number of atom
        :param q_index: Index of q-point in phonon data
        :param existing_data: If provided, results will be added to this existing object

        :returns: s, and corresponding frequencies for all quantum events taken into account
        """
        data = existing_data if existing_data else self._get_empty_data()

        local_freq = np.copy(self._fundamentals_freq)
        local_coeff = np.arange(start=0.0, step=1.0, stop=self._fundamentals_freq.size, dtype=INT_TYPE)
        fund_coeff = np.copy(local_coeff)

        for order in range(FUNDAMENTALS, self._quantum_order_num + S_LAST_INDEX):

            # in case there is large number of transitions chop it into chunks and process chunk by chunk
            if local_freq.size * self._fundamentals_freq.size > abins.parameters.performance['optimal_size']:

                chunked_fundamentals, chunked_fundamentals_coeff = self._prepare_chunks(local_freq=local_freq,
                                                                                        order=order)

                for fund_chunk, fund_coeff_chunk in zip(chunked_fundamentals, chunked_fundamentals_coeff):

                    part_loc_freq = np.copy(local_freq)
                    part_loc_coeff = np.copy(local_coeff)

                    # number of transitions can only go up
                    for lg_order in range(order, self._quantum_order_num + S_LAST_INDEX):
                        part_loc_freq, part_loc_coeff = self._helper_atom(atom=atom,
                                                                          local_freq=part_loc_freq,
                                                                          local_coeff=part_loc_coeff,
                                                                          fundamentals_freq=fund_chunk,
                                                                          fund_coeff=fund_coeff_chunk,
                                                                          order=lg_order,
                                                                          existing_data=data)
                return data

            # if relatively small array of transitions then process it in one shot
            else:
                local_freq, local_coeff = self._helper_atom(atom=atom,
                                                            local_freq=local_freq,
                                                            local_coeff=local_coeff,
                                                            fundamentals_freq=self._fundamentals_freq,
                                                            fund_coeff=fund_coeff,
                                                            order=order,
                                                            existing_data=data)
        return data

    def _prepare_chunks(self, local_freq=None, order=None):
        """
        Helper function for _calculate_s_powder_1d_one_atom in case transitions energies have to be created from
        fundamentals chunks (chunk by chunk).
        :param local_freq: frequency from the previous transition
        :param order:  order of quantum event
        :returns: 2D numpy array with fundamentals chunks, 2D array with corresponding coefficients
        """
        fund_size = self._fundamentals_freq.size
        l_size = local_freq.size
        opt_size = float(abins.parameters.performance['optimal_size'])

        chunk_size = max(1.0, np.floor(opt_size / (l_size * 2**(MAX_ORDER - order))))
        chunk_num = int(np.ceil(float(fund_size) / chunk_size))
        new_dim = int(chunk_num * chunk_size)
        new_fundamentals = np.zeros(shape=new_dim, dtype=FLOAT_TYPE)
        new_fundamentals_coeff = np.zeros(shape=new_dim, dtype=INT_TYPE)
        new_fundamentals[:fund_size] = self._fundamentals_freq
        new_fundamentals_coeff[:fund_size] = np.arange(start=0.0,
                                                       step=1.0,
                                                       stop=self._fundamentals_freq.size,
                                                       dtype=INT_TYPE)

        new_fundamentals = new_fundamentals.reshape(chunk_num, int(chunk_size))
        new_fundamentals_coeff = new_fundamentals_coeff.reshape(chunk_num, int(chunk_size))

        return new_fundamentals, new_fundamentals_coeff

    def _helper_atom(self, *, atom: int, local_freq: np.ndarray, local_coeff: List[Tuple[int, ...]],
                     fundamentals_freq: np.ndarray, fund_coeff: np.ndarray, order: int, existing_data: SDataByAngle):
        """
        Helper function. It calculates S for one atom, q-index, order and for one
        or more angles (detectors).
        :param atom: number of atom
        :param local_freq: frequency from the previous transition
        :param local_coeff: coefficients from the previous transition
        :param fundamentals_freq: fundamental frequencies
        :param fund_coeff: fundamental coefficients
        :param order: order of quantum event
        :param existing_data: object to which re-binned spectra will be added

        :returntype: tuple
        :returns:
            (local_freq, local_coeff)

            - local_freq: frequencies enumerated and used in this quantum order
            - local_coeff: tuples of indices identifying the fundamental
                  frequencies contributing to these frequencies
        """
        local_freq, local_coeff = self._freq_generator.construct_freq_combinations(previous_array=local_freq,
                                                                                   previous_coefficients=local_coeff,
                                                                                   fundamentals_array=fundamentals_freq,
                                                                                   fundamentals_coefficients=fund_coeff,
                                                                                   quantum_order=order)

        angles = self._instrument.get_angles()

        if local_freq.any():  # check if local_freq has non-zero values
            indent = ANGLE_MESSAGE_INDENTATION

            self._report_progress(msg=indent + "Calculation for the detector at angle %s (atom=%s)" % (angles[0], atom))
            q2 = self._instrument.calculate_q_powder(input_data=local_freq, angle=angles[0])

            opt_local_freq, opt_local_coeff, rebinned_broad_spectrum = self._helper_atom_angle(atom=atom,
                                                                                               local_freq=local_freq,
                                                                                               local_coeff=local_coeff,
                                                                                               angle=angles[0],
                                                                                               order=order,
                                                                                               q2=q2)

            existing_data.set_angle_data_from_dict(
                angle_index=0,
                data={f'atom_{atom}': {
                    's': {
                        f'order_{order}': rebinned_broad_spectrum * self._weight
                    }
                }},
                add_to_existing=True)

            for angle_index, angle in list(enumerate(angles))[1:]:
                self._report_progress(msg=indent + "Calculation for the detector at angle %s (atom=%s)" % (angle, atom))
                q2 = self._instrument.calculate_q_powder(input_data=local_freq, angle=angle)
                existing_data.set_angle_data_from_dict(
                    angle_index=angle_index,
                    data={
                        f'atom_{atom}': {
                            's': {
                                f'order_{order}': self._helper_atom_angle(atom=atom,
                                                                          local_freq=local_freq,
                                                                          local_coeff=local_coeff,
                                                                          angle=angle,
                                                                          order=order,
                                                                          return_freq=False,
                                                                          q2=q2) * self._weight
                            }
                        }
                    },
                    add_to_existing=True)

            local_coeff = opt_local_coeff
            local_freq = opt_local_freq

        return local_freq, local_coeff

    def _helper_atom_angle(self,
                           atom=None,
                           local_freq=None,
                           local_coeff=None,
                           angle=None,
                           order=None,
                           return_freq=True,
                           q2=None):
        """
        Helper function. It calculates S for one atom, q-index, order and angle (detector).
        In case 2D instrument rebinning over q is performed.
        :param q2: squared momentum transfer
        :param atom: number of atom
        :param local_freq: frequency from the previous transition
        :param local_coeff: coefficients from the previous transition
        :param angle: scattering angle
        :param order: order of quantum event
        :param return_freq: if true frequencies and corresponding coefficients are returned together with rebinned
                            spectrum; otherwise only rebinned spectrum is returned
        :return: (optionally) frequencies and corresponding coefficients are returned together
                 (always) with rebinned spectrum
        """
        # calculate discrete S for the given quantum order event
        value_dft = self._calculate_order[order](q2=q2,
                                                 frequencies=local_freq,
                                                 indices=local_coeff,
                                                 a_tensor=self._a_tensors[atom],
                                                 a_trace=self._a_traces[atom],
                                                 b_tensor=self._b_tensors[atom],
                                                 b_trace=self._b_traces[atom])

        # convolve with instrumental resolution
        broadening_scheme = abins.parameters.sampling['broadening_scheme']
        _, rebinned_broad_spectrum = self._instrument.convolve_with_resolution_function(frequencies=local_freq,
                                                                                        bins=self._bins,
                                                                                        s_dft=value_dft,
                                                                                        scheme=broadening_scheme)

        # calculate transition energies for construction of higher order quantum event
        local_freq, local_coeff = self._calculate_s_over_threshold(s=value_dft, freq=local_freq, coeff=local_coeff)

        if return_freq:
            return local_freq, local_coeff, rebinned_broad_spectrum
        else:
            return rebinned_broad_spectrum

    # noinspection PyUnusedLocal
    def _calculate_order_one(self,
                             q2=None,
                             frequencies=None,
                             indices=None,
                             a_tensor=None,
                             a_trace=None,
                             b_tensor=None,
                             b_trace=None):
        """
        Calculates S for the first order quantum event for one atom.
        :param q2: squared values of momentum transfer vectors
        :param frequencies: frequencies for which transitions occur
        :param indices: array which stores information how transitions can be decomposed in terms of fundamentals
        :param a_tensor: total MSD tensor for the given atom
        :param a_trace: total MSD trace for the given atom
        :param b_tensor: frequency dependent MSD tensor for the given atom
        :param b_trace: frequency dependent MSD trace for the given atom
        :returns: s for the first quantum order event for the given atom
        """
        trace_ba = np.einsum('kli, il->k', b_tensor, a_tensor)
        coth = 1.0 / np.tanh(frequencies * CM1_2_HARTREE / (2.0 * self._temperature * K_2_HARTREE))

        s = q2 * b_trace / 3.0 * np.exp(-q2 * (a_trace + 2.0 * trace_ba / b_trace) / 5.0 * coth * coth)

        return s

    # noinspection PyUnusedLocal
    def _calculate_order_two(self,
                             q2=None,
                             frequencies=None,
                             indices=None,
                             a_tensor=None,
                             a_trace=None,
                             b_tensor=None,
                             b_trace=None):
        """
        Calculates S for the second order quantum event for one atom.

        :param q2: squared values of momentum transfer vectors
        :param frequencies: frequencies for which transitions occur
        :param indices: array which stores information how transitions can be decomposed in terms of fundamentals
        :param a_tensor: total MSD tensor for the given atom
        :param a_trace: total MSD trace for the given atom
        :param b_tensor: frequency dependent MSD tensor for the given atom
        :param b_trace: frequency dependent MSD trace for the given atom
        :returns: s for the second quantum order event for the given atom
        """
        coth = 1.0 / np.tanh(frequencies * CM1_2_HARTREE / (2.0 * self._temperature * K_2_HARTREE))

        dw = np.exp(-q2 * a_trace / 3.0 * coth * coth)
        q4 = q2**2

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
                                 np.take(b_tensor, indices=indices[:, 0], axis=0))) / (30.0 * factor)
        return s

    # noinspection PyUnusedLocal,PyUnusedLocal
    def _calculate_order_three(self,
                               q2=None,
                               frequencies=None,
                               indices=None,
                               a_tensor=None,
                               a_trace=None,
                               b_tensor=None,
                               b_trace=None):
        """
        Calculates S for the third order quantum event for one atom.
        :param q2: squared values of momentum transfer vectors
        :param frequencies: frequencies for which transitions occur
        :param indices: array which stores information how transitions can be decomposed in terms of fundamentals
        :param a_tensor: total MSD tensor for the given atom
        :param a_trace: total MSD trace for the given atom
        :param b_tensor: frequency dependent MSD tensor for the given atom
        :param b_trace: frequency dependent MSD trace for the given atom
        :returns: s for the third quantum order event for the given atom
        """
        coth = 1.0 / np.tanh(frequencies * CM1_2_HARTREE / (2.0 * self._temperature * K_2_HARTREE))
        s = (9.0 / 1086.0 * q2**3 * np.prod(np.take(b_trace, indices=indices), axis=1) *
             np.exp(-q2 * a_trace / 3.0 * coth * coth))

        return s

    # noinspection PyUnusedLocal
    def _calculate_order_four(self,
                              q2=None,
                              frequencies=None,
                              indices=None,
                              a_tensor=None,
                              a_trace=None,
                              b_tensor=None,
                              b_trace=None):
        """
        Calculates S for the fourth order quantum event for one atom.
        :param q2: q2: squared values of momentum transfer vectors
        :param frequencies: frequencies for which transitions occur
        :param indices: array which stores information how transitions can be decomposed in terms of fundamentals
        :param a_tensor: total MSD tensor for the given atom
        :param a_trace: total MSD trace for the given atom
        :param b_tensor: frequency dependent MSD tensor for the given atom
        :param b_trace: frequency dependent MSD trace for the given atom
        :returns: s for the forth quantum order event for the given atom
        """
        coth = 1.0 / np.tanh(frequencies * CM1_2_HARTREE / (2.0 * self._temperature * K_2_HARTREE))
        s = (27.0 / 49250.0 * q2**4 * np.prod(np.take(b_trace, indices=indices), axis=1) *
             np.exp(-q2 * a_trace / 3.0 * coth * coth))

        return s

    def _rebin_data_full(self, array_x=None, array_y=None):
        """
        Rebins S data so that all quantum events have the same x-axis. The size of rebinned data is equal to _bins.size.
        :param array_x: numpy array with frequencies
        :param array_y: numpy array with S
        :returns: rebinned frequencies
        """
        indices = array_x != self._bins[-1]
        array_x = array_x[indices]
        array_y = array_y[indices]
        maximum_index = min(len(array_x), len(array_y))
        return np.histogram(array_x[:maximum_index], bins=self._bins, weights=array_y[:maximum_index])[0]

    def calculate_data(self):
        """
        Calculates dynamical structure factor S.
        :returns: object of type SData and dictionary with total S.
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
        :returns: object of type SData.
        """
        data = self._clerk.load(list_of_datasets=["data"], list_of_attributes=["filename", "order_of_quantum_events"])
        frequencies = data["datasets"]["data"]["frequencies"]

        if self._quantum_order_num > data["attributes"]["order_of_quantum_events"]:
            raise ValueError("User requested a larger number of quantum events to be included in the simulation "
                             "then in the previous calculations. S cannot be loaded from the hdf file.")
        if self._quantum_order_num < data["attributes"]["order_of_quantum_events"]:

            self._report_progress("""
                         User requested a smaller number of quantum events than in the previous calculations.
                         S Data from hdf file which corresponds only to requested quantum order events will be
                         loaded.""")

            atoms_s = {}

            # load atoms_data
            n_atom = len([key for key in data["datasets"]["data"].keys() if "atom" in key])
            for i in range(n_atom):
                atoms_s["atom_%s" % i] = {"s": dict()}
                for j in range(FUNDAMENTALS, self._quantum_order_num + S_LAST_INDEX):

                    temp_val = data["datasets"]["data"]["atom_%s" % i]["s"]["order_%s" % j]
                    atoms_s["atom_%s" % i]["s"].update({"order_%s" % j: temp_val})

            # reduce the data which is loaded to only this data which is required by the user

            data["datasets"]["data"] = atoms_s

        else:
            atoms_s = {key: value for key, value in data["datasets"]["data"].items() if key != "frequencies"}

        s_data = abins.SData(temperature=self._temperature,
                             sample_form=self._sample_form,
                             data=atoms_s,
                             frequencies=frequencies)

        if s_data.get_bin_width is None:
            raise Exception("Loaded data does not have consistent frequency spacing")

        return s_data

    def get_formatted_data(self):
        """
        Method to obtain data
        :returns: obtained data
        """
        try:

            self._clerk.check_previous_data()
            data = self.load_formatted_data()
            self._report_progress(f"{data} has been loaded from the HDF file.", reporter=self.progress_reporter)

        except (IOError, ValueError):
            self._report_progress("Data not found in cache. Structure factors need to be calculated.", notice=True)
            data = self.calculate_data()

            self._report_progress(f"{data} has been calculated.", reporter=self.progress_reporter)

        data.check_thresholds()
        return data
