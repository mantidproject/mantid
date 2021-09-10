# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

import numbers
from typing import Optional, Union, Tuple

import numpy as np

from abins import AbinsData, FrequencyPowderGenerator, SData, SDataByAngle
from abins.constants import CM1_2_HARTREE, K_2_HARTREE, FLOAT_TYPE, INT_TYPE, MIN_SIZE
from abins.instruments import Instrument
import abins.parameters
from mantid.api import Progress


class SPowderSemiEmpiricalCalculator:
    """
    Class for calculating S(Q, omega)
    """

    @staticmethod
    def _check_parameters(**kwargs):
        from abins.constants import FUNDAMENTALS, HIGHER_ORDER_QUANTUM_EVENTS

        if isinstance(kwargs.get('filename'), str):
            if kwargs.get('filename').strip() == "":
                raise ValueError("Name of the file cannot be an empty string!")
        else:
            raise ValueError("Invalid name of input file. String was expected!")

        if not isinstance(kwargs.get('temperature'), numbers.Real):
            raise ValueError("Invalid value of the temperature. Number was expected.")
        if kwargs.get('temperature') < 0:
            raise ValueError("Temperature cannot be negative.")

        if not isinstance(kwargs.get('abins_data'), AbinsData):
            raise ValueError("Object of type AbinsData was expected.")

        min_order = FUNDAMENTALS
        max_order = FUNDAMENTALS + HIGHER_ORDER_QUANTUM_EVENTS
        quantum_order_num = kwargs.get('quantum_order_num')

        if not (isinstance(quantum_order_num, int)
                and min_order <= quantum_order_num <= max_order):
            raise ValueError("Invalid number of quantum order events.")

        if kwargs.get('autoconvolution') and abins.parameters.autoconvolution['max_order'] <= quantum_order_num:
            raise ValueError("Autoconvolution max order should be greater than max order of semi-analytic spectra")

        if not isinstance(kwargs.get('instrument'), Instrument):
            raise ValueError("Unknown instrument %s" % kwargs.get('instrument'))

    def __init__(self, *,
                 filename: str,
                 temperature: numbers.Real,
                 abins_data: abins.AbinsData,
                 instrument: Instrument,
                 quantum_order_num: int,
                 autoconvolution: bool = False,
                 bin_width: float = 1.0) -> None:
        """
        :param filename: name of input DFT file (CASTEP: foo.phonon). This is only used for caching, the file will not be read.
        :param temperature: temperature in K for which calculation of S should be done
        :param abins_data: object of type AbinsData with data from phonon file
        :param instrument: name of instrument (str)
        :param quantum_order_num: number of quantum order events to simulate in semi-analytic approximation
        :param autoconvolution:
            approximate spectra up to abins.parameters.autoconvolution['max_order'] using auto-convolution
        :param bin_width: bin width in wavenumber for re-binned spectrum
        """
        self._check_parameters(**locals())

        # Expose input parameters
        self._input_filename = filename
        self._temperature = float(temperature)
        self._abins_data = abins_data
        self._quantum_order_num = quantum_order_num
        self._autoconvolution = autoconvolution
        self._instrument = instrument

        # This is only used as metadata for clerk, like filename
        self._sample_form = "Powder"

        # Get derived properties
        self._num_k = len(self._abins_data.get_kpoints_data())
        self._num_atoms = len(self._abins_data.get_atoms_data())

        # Initialise properties that are set elsewhere
        self._progress_reporter = None
        self._powder_data = None

        # Set up caching
        self._clerk = abins.IO(
            input_filename=filename,
            setting=self._instrument.get_setting(),
            autoconvolution=self._autoconvolution,
            group_name=("{s_data_group}/{instrument}/{sample_form}/{temperature}K").format(
                s_data_group=abins.parameters.hdf_groups['s_data'],
                instrument=self._instrument,
                sample_form=self._sample_form,
                temperature=self._temperature))

        # Set up two sampling grids: _bins for broadening/output
        # and _fine_bins which subdivides _bins to prevent accumulation of binning errors
        # during autoconvolution
        self._bins = np.arange(start=abins.parameters.sampling['min_wavenumber'],
                               stop=(abins.parameters.sampling['max_wavenumber'] + bin_width),
                               step=bin_width,
                               dtype=FLOAT_TYPE)
        self._bin_centres = self._bins[:-1] + (bin_width / 2)

        if self._autoconvolution:
            fine_bin_factor = abins.parameters.autoconvolution['fine_bin_factor']
            self._fine_bins = np.arange(start=abins.parameters.sampling['min_wavenumber'],
                                        stop=(abins.parameters.sampling['max_wavenumber'] + bin_width),
                                        step=(bin_width / fine_bin_factor),
                                        dtype=FLOAT_TYPE)
            self._fine_bin_centres = self._fine_bins[:-1] + (bin_width / fine_bin_factor / 2)

        else:
            self._fine_bins = None
            self._fine_bin_centres = None

    def get_formatted_data(self) -> SData:
        """
        Get structure factor, from cache or calculated as necessary
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

        data.check_thresholds(logging_level='information')
        return data

    def load_formatted_data(self) -> SData:
        """
        Loads S from an hdf file.
        :returns: object of type SData.
        """
        data = self._clerk.load(list_of_datasets=["data"], list_of_attributes=["filename", "order_of_quantum_events"])
        frequencies = data["datasets"]["data"]["frequencies"]

        if self._quantum_order_num > data["attributes"]["order_of_quantum_events"]:
            raise ValueError("User requested a larger number of quantum events to be included in the simulation "
                             "than in the previous calculations. S cannot be loaded from the hdf file.")
        if self._quantum_order_num < data["attributes"]["order_of_quantum_events"]:

            self._report_progress("""
                         User requested a smaller number of quantum events than in the previous calculations.
                         S Data from hdf file which corresponds only to requested quantum order events will be
                         loaded.""")

            atoms_s = {}

            # load atoms_data
            n_atom = len([key for key in data["datasets"]["data"].keys() if "atom" in key])
            for i in range(n_atom):
                atoms_s[f"atom_{i}"] = {"s": dict()}
                for j in range(1, self._quantum_order_num + 1):

                    temp_val = data["datasets"]["data"][f"atom_{i}"]["s"][f"order_{j}"]
                    atoms_s[f"atom_{i}"]["s"].update({f"order_{j}": temp_val})

            # reduce the data which is loaded to only this data which is required by the user
            data["datasets"]["data"] = atoms_s

        else:
            atoms_s = {key: value for key, value in data["datasets"]["data"].items()
                       if key != "frequencies"}

        s_data = abins.SData(temperature=self._temperature, sample_form=self._sample_form,
                             data=atoms_s, frequencies=frequencies)

        if s_data.get_bin_width is None:
            raise Exception("Loaded data does not have consistent frequency spacing")

        return s_data

    def calculate_data(self) -> SData:
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

    @property
    def progress_reporter(self) -> Union[None, Progress]:
        return self._progress_reporter

    @progress_reporter.setter
    def progress_reporter(self, progress_reporter) -> None:
        if isinstance(progress_reporter, (Progress, type(None))):
            self._progress_reporter = progress_reporter
        else:
            raise TypeError("Progress reporter type should be mantid.api.Progress. "
                            "If unavailable, use None.")

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

    def _calculate_s(self) -> SData:
        """Calculate structure factor by dispatching to appropriate 1d or 2d workflow"""
        from abins.constants import ONE_DIMENSIONAL_INSTRUMENTS, TWO_DIMENSIONAL_INSTRUMENTS

        # Compute tensors and traces, write to cache for access during atomic s calculations
        powder_calculator = abins.PowderCalculator(filename=self._input_filename, abins_data=self._abins_data)
        self._powder_data = powder_calculator.get_formatted_data()

        # Dispatch to appropriate routine
        if self._instrument.get_name() in ONE_DIMENSIONAL_INSTRUMENTS:
            return self._calculate_s_powder_1d(
                isotropic_fundamentals=abins.parameters.development.get('isotropic_fundamentals', False))
        elif self._instrument.get_name() in TWO_DIMENSIONAL_INSTRUMENTS:
            return self._calculate_s_powder_2d()
        else:
            raise ValueError('Instrument "{}" is not recognised, cannot perform semi-empirical '
                             'powder averaging.'.format(self._instrument.get_name()))

    @staticmethod
    def _calculate_s_powder_2d() -> SData:
        raise NotImplementedError('2D instruments not supported in this version.')

    def _calculate_s_powder_1d(self, isotropic_fundamentals=False) -> SData:
        """
        Calculates 1D S for the powder case.

        Args:
            isotropic_fundamentals
                If True, use isotropic approximation for Debye-Waller factor of
                fundamental modes. (Otherwise a slower mode-by-mode method is used.)

        :returns: object of type SData with 1D dynamical structure factors for the powder case
        """
        if self.progress_reporter:
            self.progress_reporter.setNumSteps(len(self._instrument.get_angles())
                                               * (self._num_k * self._num_atoms + 1)
                                               # Autoconvolution message if appropriate
                                               + (1 if self._autoconvolution else 0)
                                               # Isotropic DW message if appropriate
                                               + (1 if (isotropic_fundamentals
                                                        or (self._quantum_order_num > 1)
                                                        or self._autoconvolution)
                                                  else 0))

        sdata_by_angle = []
        for angle in self._instrument.get_angles():
            self._report_progress(msg=f'Calculating S for angle: {angle:} degrees',
                                  reporter=self.progress_reporter)
            sdata_by_angle.append(self._calculate_s_powder_over_k(angle=angle,
                                                                  isotropic_fundamentals=isotropic_fundamentals,
                                                                  autoconvolution=self._autoconvolution))

        # Complete set of scattering intensity data including Debye-Waller factors and autocorrelation orders
        sdata_by_angle = SDataByAngle.from_sdata_series(sdata_by_angle, angles=self._instrument.get_angles())

        # Sum and broaden to single set of s_data with instrumental corrections
        s_data = sdata_by_angle.sum_over_angles(average=True)
        broadening_scheme = abins.parameters.sampling['broadening_scheme']
        s_data = self._broaden_sdata(s_data,
                                     broadening_scheme=broadening_scheme)
        return s_data

    def _calculate_s_powder_over_k(self, *, angle: float,
                                   isotropic_fundamentals: bool = False,
                                   autoconvolution: bool = False) -> SData:
        """Calculate S for a given angle in semi-analytic powder-averaging approximation

        This data is averaged over the phonon k-points and Debye-Waller factors
        are included.

        Args:
            angle: Scattering angle used to determine energy-q relationship
            isotropic_fundamentals:
                Replace the mode-by-mode Debye Waller factor for fundamental
                modes with faster isotropic approximation. This method will
                always be used for quantum orders above fundamentals.
            autoconvolution:
                Estimate spectra for higher quantum orders by convolving the
                highest computed spectrum with the fundamentals before applying
                the isotropic Debye-Waller factor.

        Returns:
            SData
        """
        # Initialize the main data container
        sdata = self._get_empty_sdata(use_fine_bins=self._autoconvolution, max_order=self._quantum_order_num)

        # Compute order 1 structure factor and corresponding DW contributions
        if isotropic_fundamentals:
            min_order = 1
        else:
            fundamentals_sdata, fundamentals_sdata_with_dw = self._calculate_fundamentals_over_k(angle=angle)
            if autoconvolution:
                sdata.update(fundamentals_sdata)  # These pre-DW fundamentals are needed for autoconvolution
            min_order = 2

        # Collect SData without DW factors
        if isotropic_fundamentals or self._quantum_order_num > 1:
            for k_index in range(self._num_k):
                _ = self._calculate_s_powder_over_atoms(k_index=k_index, angle=angle,
                                                        bins=(self._fine_bins if self._autoconvolution else self._bins),
                                                        sdata=sdata, min_order=min_order)

        if autoconvolution:
            max_dw_order = abins.parameters.autoconvolution['max_order']
            self._report_progress(f"Finished calculating SData to order {self._quantum_order_num} by "
                                  f"analytic powder-averaging. "
                                  f"Adding autoconvolution data up to order {max_dw_order}.",
                                  reporter=self.progress_reporter)
            sdata.add_autoconvolution_spectra()
            sdata = sdata.rebin(self._bins)  # Don't need fine bins any more, so reduce cost of remaining steps

        else:
            self._report_progress(f"Finished calculating SData to order {self._quantum_order_num} by analytic powder-averaging.",
                                  reporter=self.progress_reporter)
            max_dw_order = self._quantum_order_num

        if isotropic_fundamentals or (self._quantum_order_num > 1) or autoconvolution:
            self._report_progress(f"Applying isotropic Debye-Waller factor to orders {min_order} and above.",
                                  reporter=self.progress_reporter)
            iso_dw = self.calculate_isotropic_dw(angle=angle)
            sdata.apply_dw(iso_dw, min_order=min_order, max_order=max_dw_order)

        if not isotropic_fundamentals:
            sdata.update(fundamentals_sdata_with_dw)  # Replace fundamentals with more sophisticated DW

        return sdata

    def calculate_isotropic_dw(self, *, angle: float) -> np.ndarray:
        """Compute Debye-Waller factor in isotropic approximation for current system

        Returns an N_atoms x N_frequencies array.
        """

        q2 = self._instrument.calculate_q_powder(input_data=self._bin_centres, angle=angle)

        average_a_traces = np.sum([self._powder_data.get_a_traces(k_index) * kpoint.weight
                                  for k_index, kpoint in enumerate(self._abins_data.get_kpoints_data())],
                                  axis=0)
        return self._isotropic_dw(frequencies=self._bin_centres, q2=q2, a_trace=average_a_traces[:, np.newaxis],
                                  temperature=self._temperature)

    @staticmethod
    def _isotropic_dw(*, frequencies, q2, a_trace, temperature):
        """Compute Debye-Waller factor in isotropic approximation"""
        if temperature < np.finfo(type(temperature)).eps:
            coth = 1.
        else:
            coth = 1.0 / np.tanh(frequencies * CM1_2_HARTREE / (2.0 * temperature * K_2_HARTREE))
        return np.exp(-q2 * a_trace / 3.0 * coth * coth)

    def _get_empty_sdata(self, use_fine_bins: bool = False,
                         max_order: Optional[int] = None) -> SData:
        """
        Initialise an appropriate SData object for this calculation
        """
        bin_centres = self._fine_bin_centres if use_fine_bins else self._bin_centres

        if max_order is None:
            max_order = self._quantum_order_num

        return SData.get_empty(frequencies=bin_centres,
                               atom_keys=list(self._abins_data.get_atoms_data().extract().keys()),
                               order_keys=[f'order_{n}' for n in range(1, max_order + 1)],
                               temperature=self._temperature, sample_form=self._sample_form)

    def _broaden_sdata(self, sdata: SData,
                       broadening_scheme: str = 'auto') -> SData:
        """
        Apply instrumental broadening to scattering data
        """
        sdata_dict = sdata.extract()
        frequencies = sdata_dict['frequencies']
        del sdata_dict['frequencies']

        for atom_key in sdata_dict:
            for order_key in sdata_dict[atom_key]['s']:
                _, sdata_dict[atom_key]['s'][order_key] = (
                    self._instrument.convolve_with_resolution_function(
                        frequencies=frequencies, bins=self._bins,
                        s_dft=sdata_dict[atom_key]['s'][order_key],
                        scheme=broadening_scheme))
        return SData(data=sdata_dict, frequencies=self._bin_centres,
                     temperature = sdata.get_temperature(),
                     sample_form = sdata.get_sample_form())

    def _calculate_fundamentals_over_k(self, *, angle: float) -> Tuple[SData, SData]:
        """
        Calculate order-1 incoherent S with and without DW corrections

        Atomic cross-sections are not included at this stage.
        Values are averaged over the k-points included in self._abins_data.

        If autoconvolution is to be applied, the base S values will be binned to a fine energy grid.
        The DW-corrected values are always binned to the standard energy grid.

        returns:
            Tuple of two SData with the base S and Debye-Waller-corrected

        """
        fundamentals_sdata = self._get_empty_sdata(use_fine_bins=self._autoconvolution, max_order=1)
        fundamentals_sdata_with_dw = self._get_empty_sdata(use_fine_bins=False, max_order=1)

        s_bins = self._fine_bins if self._autoconvolution else self._bins

        for k_index, kpoint in enumerate(self._abins_data.get_kpoints_data()):
            frequencies = self._powder_data.get_frequencies()[k_index]
            q2 = self._instrument.calculate_q_powder(input_data=frequencies, angle=angle)

            a_tensors = self._powder_data.get_a_tensors()[k_index]
            a_traces = self._powder_data.get_a_traces(k_index)
            b_tensors = self._powder_data.get_b_tensors()[k_index]
            b_traces = self._powder_data.get_b_traces(k_index)

            for atom_index, atom_label in enumerate(self._abins_data.get_atoms_data().extract()):
                s, s_with_dw = self._calculate_order_one(q2=q2,
                                                         frequencies=frequencies,
                                                         a_tensor=a_tensors[atom_index],
                                                         a_trace=a_traces[atom_index],
                                                         b_tensor=b_tensors[atom_index],
                                                         b_trace=b_traces[atom_index],
                                                         include_dw=True)

                rebinned_s, _ = np.histogram(frequencies, bins=s_bins,
                                             weights=(s * kpoint.weight), density=False)
                rebinned_s_with_dw, _ = np.histogram(frequencies, bins=self._bins,
                                                     weights=(s_with_dw * kpoint.weight), density=False)

                fundamentals_sdata.add_dict({atom_label: {'s': {'order_1': rebinned_s}}})
                fundamentals_sdata_with_dw.add_dict({atom_label: {'s': {'order_1': rebinned_s_with_dw}}})

        return fundamentals_sdata, fundamentals_sdata_with_dw

    def _calculate_s_powder_over_atoms(self, *, k_index: int,
                                       angle: float,
                                       sdata: SData,
                                       bins: np.ndarray,
                                       min_order: int = 1) -> None:
        """
        Evaluates S for all atoms for the given q-point and checks if S is consistent.

        Powder data should have already been initialised and stored in self._powder_data

        :param k_index: Index of k-point from calculated phonon data
        :param angle: Scattering angle determining energy-q relationship
        :param sdata: Data container to which results will be summed in-place
        :param bins: Frequency bins consistent with sdata
        :param min_order: Lowest quantum order to evaluate. (The max is determined by self._quantum_order_num.)

        """
        assert min_order in (1, 2)  # Cannot start higher than 2; need information about combinations

        for atom_index in range(self._num_atoms):
            self._report_progress(msg=f'Calculating S for atom {atom_index}, k-point {k_index}, {angle} degrees',
                                  reporter=self.progress_reporter)
            self._calculate_s_powder_one_atom(atom_index=atom_index, k_index=k_index, angle=angle,
                                              sdata=sdata, bins=bins, min_order=min_order)

    def _calculate_s_powder_one_atom(self, *, atom_index: int, k_index: int, angle: float,
                                     sdata: SData, bins: np.ndarray,
                                     min_order: int = 1) -> None:
        """
        :param atom_index: number of atom
        :param k_index: Index of k-point in phonon data
        :param angle: Scattering angle determining energy-q relationship
        :sdata: Data container to which results will be summed in-place
        :bins: Frequency bins consistent with sdata
        :min_order: Lowest quantum order to evaluate. (The max is determined by self._quantum_order_num.)

        """
        kpoint_weight = self._abins_data.get_kpoints_data()[k_index].weight

        fundamentals = self._powder_data.get_frequencies()[k_index]
        fund_coeff = np.arange(fundamentals.size, dtype=INT_TYPE)

        # Initialise with fundamentals regardless of whether starting with order 1 or 2
        frequencies = np.copy(fundamentals)
        coefficients = np.copy(fund_coeff)

        a_tensor = self._powder_data.get_a_tensors()[k_index][atom_index]
        a_trace = self._powder_data.get_a_traces(k_index)[atom_index]
        b_tensor = self._powder_data.get_b_tensors()[k_index][atom_index]
        b_trace = self._powder_data.get_b_traces(k_index)[atom_index]

        calculate_order = {1: self._calculate_order_one,
                           2: self._calculate_order_two,
                           3: self._calculate_order_three,
                           4: self._calculate_order_four}

        # Chunking to save memory has been removed pending closer examination
        for order in range(min_order, self._quantum_order_num + 1):
            frequencies, coefficients = FrequencyPowderGenerator.construct_freq_combinations(
                previous_array=frequencies,
                previous_coefficients=coefficients,
                fundamentals_array=fundamentals,
                fundamentals_coefficients=fund_coeff,
                quantum_order=order)
            q2 = self._instrument.calculate_q_powder(input_data=frequencies, angle=angle)

            scattering_intensities = calculate_order[order](
                q2=q2, frequencies=frequencies, indices=coefficients,
                a_tensor=a_tensor, a_trace=a_trace, b_tensor=b_tensor, b_trace=b_trace)
            rebinned_spectrum, _ = np.histogram(frequencies, bins=bins,
                                                weights=(scattering_intensities * kpoint_weight),
                                                density=False)
            sdata.add_dict({f'atom_{atom_index}':
                            {'s': {f'order_{order}': rebinned_spectrum}}})

            # Prune modes with low intensity; these are assumed not to contribute to higher orders
            frequencies, coefficients = self._calculate_s_over_threshold(scattering_intensities,
                                                                         freq=frequencies,
                                                                         coeff=coefficients)

    @staticmethod
    def _calculate_s_over_threshold(s=None, freq=None, coeff=None):
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

    def _calculate_order_one(self, *, q2: np.ndarray, frequencies: np.ndarray,
                             a_tensor=None, a_trace=None,
                             b_tensor=None, b_trace=None,
                             indices=None, include_dw=False):
        """
        Calculates S for the first order quantum event for one atom.
        :param q2: squared values of momentum transfer vectors
        :param frequencies: frequencies for which transitions occur
        :param indices: (unused) array which stores information how transitions can be decomposed in terms of fundamentals
        :param a_tensor: total MSD tensor for the given atom
        :param a_trace: total MSD trace for the given atom
        :param b_tensor: frequency dependent MSD tensor for the given atom
        :param b_trace: frequency dependent MSD trace for the given atom
        :param include_dw: Include (mode-dependent) Debye-Waller temperature effect
        :returns: s for the first quantum order event for the given atom
        """
        s = q2 * b_trace / 3.0

        if include_dw:
            trace_ba = np.einsum('kli, il->k', b_tensor, a_tensor)
            if self._temperature < np.finfo(type(self._temperature)).eps:
                coth = 1.
            else:
                coth = 1.0 / np.tanh(frequencies * CM1_2_HARTREE
                                     / (2.0 * self._temperature * K_2_HARTREE))
            dw = np.exp(-q2 * (a_trace + 2.0 * trace_ba / b_trace) / 5.0 * coth * coth)

            s_with_dw = s * dw

            return s, s_with_dw
        else:
            return s

    def _calculate_order_two(self, q2=None, frequencies=None, indices=None, a_tensor=None, a_trace=None,
                             b_tensor=None, b_trace=None):
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

        s = q4 * (np.prod(np.take(b_trace, indices=indices), axis=1)
                  + np.einsum('kli, kil->k',
                              np.take(b_tensor, indices=indices[:, 0], axis=0),
                              np.take(b_tensor, indices=indices[:, 1], axis=0))
                  + np.einsum('kli, kil->k',
                              np.take(b_tensor, indices=indices[:, 1], axis=0),
                              np.take(b_tensor, indices=indices[:, 0], axis=0))) / (30.0 * factor)
        return s

    # noinspection PyUnusedLocal,PyUnusedLocal
    def _calculate_order_three(self, q2=None, frequencies=None, indices=None, a_tensor=None, a_trace=None,
                               b_tensor=None, b_trace=None):
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
        s = 9.0 / 1086.0 * q2 ** 3 * np.prod(np.take(b_trace, indices=indices), axis=1)

        return s

    # noinspection PyUnusedLocal
    def _calculate_order_four(self, q2=None, frequencies=None, indices=None, a_tensor=None, a_trace=None,
                              b_tensor=None, b_trace=None):
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
        s = 27.0 / 49250.0 * q2 ** 4 * np.prod(np.take(b_trace, indices=indices), axis=1)

        return s
