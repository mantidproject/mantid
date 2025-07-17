# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from functools import cached_property, partial
import json
from operator import attrgetter
from pathlib import Path
from typing import Dict, Tuple, Union

from euphonic import ureg
from euphonic.spectra import (
    Spectrum1D,
    Spectrum2D,
    Spectrum1DCollection,
    Spectrum2DCollection,
)

import numpy as np
from pydantic import Field, validate_call
from pydantic.types import PositiveFloat
from scipy.special import factorial

from abins import AbinsData, FrequencyPowderGenerator
from abins.constants import FLOAT_TYPE, INT_TYPE, MIN_SIZE
from abins.instruments import Instrument
import abins.parameters
from abins.sdata import (
    add_autoconvolution_spectra,
    apply_kinematic_constraints,
    check_thresholds,
)

from mantid.api import Progress

SpectrumCollection = Spectrum1DCollection | Spectrum2DCollection

# Raw S contributions as a dict keyed by (atom_index, quantum_order)
SByAtomAndOrder = Dict[Tuple[int, int], np.ndarray]


class SPowderSemiEmpiricalCalculator:
    """
    Class for calculating S(Q, omega)
    """

    q_unit = ureg("1 / angstrom")
    freq_unit = ureg("1 / cm")
    s_unit = ureg("barn") / freq_unit

    @validate_call(config=dict(arbitrary_types_allowed=True, strict=True))
    def __init__(
        self,
        *,
        filename: str = Field(min_length=1),
        temperature: PositiveFloat,
        abins_data: AbinsData,
        instrument: Instrument,
        quantum_order_num: int = Field(ge=1, le=2),
        autoconvolution_max: int = 0,
        cache_directory: Path,
    ) -> None:
        """
        :param filename: name of input DFT file (CASTEP: foo.phonon). This is only used for caching, the file will not be read.
        :param temperature: temperature in K for which calculation of S should be done
        :param abins_data: object of type AbinsData with data from phonon file
        :param instrument: simulated INS instrument
        :param quantum_order_num: number of quantum order events to simulate in semi-analytic approximation
        :param autoconvolution_max:
            approximate spectra up to this order using auto-convolution
        :param cache_directory: location for .hdf5
        """
        # Expose input parameters
        self._input_filename = filename
        self._temperature = float(temperature)
        self._abins_data = abins_data
        self._quantum_order_num = quantum_order_num
        self._autoconvolution_max = autoconvolution_max
        self._instrument = instrument
        self._cache_directory = cache_directory

        # This is only used as metadata for clerk, like filename
        self._sample_form = "Powder"

        # Get derived properties
        self._num_k = len(self._abins_data.get_kpoints_data())
        self._num_atoms = len(self._abins_data.get_atoms_data())
        self._use_autoconvolution: bool = self._autoconvolution_max > 1

        # Initialise properties that are set elsewhere
        self._progress_reporter = None
        self._powder_data = None

        self._isotropic_fundamentals = abins.parameters.development.get("isotropic_fundamentals", False)

        # Set up caching
        self._clerk = abins.IO(
            input_filename=filename,
            setting=self._instrument.get_setting(),
            autoconvolution=self._autoconvolution_max,
            group_name=("{s_data_group}/{instrument}/{sample_form}/{temperature}K").format(
                s_data_group=abins.parameters.hdf_groups["s_data"],
                instrument=self._instrument,
                sample_form=self._sample_form,
                temperature=self._temperature,
            ),
            cache_directory=self._cache_directory,
        )

        # Set up two sampling grids: _bins for broadening/output
        # and _fine_bins which subdivides _bins to prevent accumulation of binning errors
        # during autoconvolution
        self._bins = self._instrument.get_energy_bins()
        self._bin_centres = (self._bins[:-1] + self._bins[1:]) / 2
        bin_width = instrument.get_energy_bin_width()

        if self._use_autoconvolution:
            self._fine_bin_factor = abins.parameters.autoconvolution["fine_bin_factor"]
            self._fine_bins = np.arange(
                start=self._instrument.get_min_wavenumber(),
                stop=(self._instrument.get_max_wavenumber() + bin_width),
                step=(bin_width / self._fine_bin_factor),
                dtype=FLOAT_TYPE,
            )
            self._fine_bin_centres = self._fine_bins[:-1] + (bin_width / self._fine_bin_factor / 2)
            self._fine_bin_width = self._fine_bins[1] - self._fine_bins[0]

        else:
            self._fine_bins = None
            self._fine_bin_centres = None

        # If operating in 2-D mode, there is also an explicit set of q bins
        if self.is_2d:
            params = abins.parameters.instruments[self._instrument.get_name()]
            q_min, q_max = self._instrument.get_q_bounds()
            self._q_bins = np.linspace(q_min, q_max, params.get("q_size") + 1)
            self._q_bin_centres = (self._q_bins[:-1] + self._q_bins[1:]) / 2
        else:
            self._q_bins = None
            self._q_bin_centres = None

    @cached_property
    def is_2d(self) -> bool:
        from abins.constants import ONE_DIMENSIONAL_INSTRUMENTS, TWO_DIMENSIONAL_INSTRUMENTS

        """Validate instrument name and get dimensionality from known instruments"""
        if self._instrument.get_name() in ONE_DIMENSIONAL_INSTRUMENTS:
            return False
        if self._instrument.get_name() in TWO_DIMENSIONAL_INSTRUMENTS:
            return True

        raise ValueError(
            'Instrument "{}" is not recognised, cannot perform semi-empirical powder averaging.'.format(self._instrument.get_name())
        )

    @staticmethod
    def _get_s(spectrum: Spectrum1D | Spectrum2D) -> np.ndarray:
        if isinstance(spectrum, Spectrum1D):
            return spectrum.y_data.magnitude
        else:
            return spectrum.z_data.magnitude

    def get_formatted_data(self) -> SpectrumCollection:
        """
        Get structure factor, from cache or calculated as necessary
        :returns: obtained data
        """
        try:
            self._clerk.check_previous_data()
            spectra = self.load_formatted_data()
            self._report_progress("Spectrum data has been loaded from the HDF file.", reporter=self.progress_reporter)

        except (IOError, ValueError):
            self._report_progress("Data not found in cache. Structure factors need to be calculated.", notice=True)
            spectra = self.calculate_data()

            self._report_progress("Spectrum data has been calculated.", reporter=self.progress_reporter)

            check_thresholds(
                ((spectrum.metadata["atom_index"], spectrum.metadata["quantum_order"], self._get_s(spectrum)) for spectrum in spectra),
                logging_level="information",
            )

        return spectra

    def load_formatted_data(self) -> SpectrumCollection:
        """
        Loads S from an hdf file.
        """
        data = self._clerk.load(list_of_datasets=["data"], list_of_attributes=["filename", "order_of_quantum_events"])

        if self._quantum_order_num > data["attributes"]["order_of_quantum_events"]:
            raise ValueError(
                "User requested a larger number of quantum events to be included in the simulation "
                "than in the previous calculations. S cannot be loaded from the hdf file."
            )

        data_dict = data["datasets"]["data"]
        data_dict["metadata"] = json.loads(data_dict["metadata"])

        def _stringify(key: str | bytes) -> str:
            if isinstance(key, bytes):
                return key.decode("utf8")
            else:
                return key

        # Some of the strings get stored as bytes, safer to just parse strings
        data_dict = {key: _stringify(value) for key, value in data_dict.items()}

        if self.is_2d:
            spectra = Spectrum2DCollection.from_dict(data_dict)
        else:
            spectra = Spectrum1DCollection.from_dict(data_dict)

        return spectra

    def calculate_data(self) -> SpectrumCollection:
        """
        Calculates dynamical structure factor S.

        2-D writing is currently slow compared to 2-D calculation, so only 1-D
        results will be cached.
        """
        spectra = self._calculate_s()

        spectra_dict = spectra.to_dict()
        # Metadata dict is not very HDF5-friendly, dump to string
        spectra_dict["metadata"] = json.dumps(spectra_dict["metadata"])

        if not self.is_2d:
            self._clerk.add_file_attributes()
            self._clerk.add_attribute(name="order_of_quantum_events", value=self._quantum_order_num)
            self._clerk.add_data("data", spectra_dict)
            self._clerk.save()

        return spectra

    @property
    def progress_reporter(self) -> Union[None, Progress]:
        return self._progress_reporter

    @progress_reporter.setter
    def progress_reporter(self, progress_reporter) -> None:
        if isinstance(progress_reporter, (Progress, type(None))):
            self._progress_reporter = progress_reporter
        else:
            raise TypeError("Progress reporter type should be mantid.api.Progress. If unavailable, use None.")

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

    def _calculate_s(self) -> SpectrumCollection:
        """Calculate structure factor by dispatching to appropriate 1d or 2d workflow

        If self._isotropic_fundamentals is True, order-1 will use the same Debye-Waller approximation
        as higher orders.
        """
        # Compute tensors and traces, write to cache for access during atomic s calculations
        powder_calculator = abins.PowderCalculator(
            filename=self._input_filename, abins_data=self._abins_data, temperature=self._temperature, cache_directory=self._cache_directory
        )
        self._powder_data = powder_calculator.get_formatted_data()

        # Dispatch to appropriate routine
        if self.is_2d:
            return self._calculate_s_powder_2d()

        return self._calculate_s_powder_1d()

    def _calculate_s_powder_2d(self) -> Spectrum2DCollection:
        spectra = self._calculate_s_powder_over_k_and_q()
        apply_kinematic_constraints(spectra, self._instrument)

        return spectra

    def _calculate_s_powder_1d(self) -> Spectrum1DCollection:
        """
        Calculate 1-D S(q,w) using geometry-constrained energy-q relationships

        Includes sum over scattering angles if appropriate.

        Returns:
            Spectrum1DCollection populated with spectrum contributions,
            separated by atom and quantum order (as indicated in metadata).

        """
        broadening_scheme = abins.parameters.sampling["broadening_scheme"]

        if self.progress_reporter:
            self.progress_reporter.setNumSteps(
                len(self._instrument.get_angles()) * (self._num_k * self._num_atoms + 1)
                # Autoconvolution message if appropriate
                + (1 if self._use_autoconvolution else 0)
                # Isotropic DW message if appropriate
                + (1 if (self._isotropic_fundamentals or (self._quantum_order_num > 1) or self._use_autoconvolution) else 0)
            )

        spectra_by_angle = []
        for angle in self._instrument.get_angles():
            self._report_progress(msg=f"Calculating S for angle: {angle:} degrees", reporter=self.progress_reporter)
            one_angle_spectra = self._calculate_s_powder_over_k(angle=angle)
            one_angle_spectra.metadata["angle"] = f"{angle:.16f}"
            spectra_by_angle.append(one_angle_spectra)

        # Sum and broaden to single set of s_data with instrumental corrections
        _iter_spectra_by_angle = iter(spectra_by_angle)
        spectra = sum(_iter_spectra_by_angle, start=next(_iter_spectra_by_angle))
        spectra = spectra.group_by("atom_index", "quantum_order")

        spectra = self._broaden_spectra(spectra, broadening_scheme=broadening_scheme)

        return spectra

    def _calculate_s_isotropic(self, q2: np.ndarray, broaden: bool = False) -> SpectrumCollection:
        """Calculate S(q,ω) components that use isotropic Debye-Waller term

        This is:
        - fundamentals (only if self._isotropic_fundamentals; otherwise left empty)
        - order 2 (if self._quantum_order_num > 1)
        - orders 3-10 (if self._autoconvolution_max > 2)

        Args:
            q2: q^2 values for intensity and Debye-Waller calculation. This
                should have dimensions (N_ENERGY_BINS,) for a 1-D S(ω) calculation
                with fixed q-energy relationship (i.e. a fixed angle) or
                (N_Q_BINS, 1) for a calculation over independent q, energy
                (i.e. when constructing a 2D S(|q|, ω) map.)

            broaden: perform instrumental broadening. (For 2-D maps it is more
                efficient to broaden inside this method, as energy broadening
                can be applied before q-dependence is added. For 1-D
                calculations over multiple angles, it is more efficient to
                broaden outside this method, after angle contributions have
                been summed.)

        Returns:
            Debye-Waller corrected scattering intensities at the calculated
            orders, for all atoms.
        """
        from abins.constants import FLOAT_TYPE, MASS_STR_FORMAT
        from collections import defaultdict

        # Calculate fundamentals and order-2 in isotropic powder-averaging approximation
        if self._quantum_order_num == 1 or self._use_autoconvolution:
            min_order = 1  # Need fundamentals without DW
        else:
            min_order = 2  # Skip fundamentals to be calculated separately

        # Get numpy broadcasting right: we need to convert an array from (order,)
        # to (order, energy) or (order, q, energy) format.
        if self.is_2d:
            order_expansion_slice = np.s_[:, np.newaxis, np.newaxis]
        else:
            order_expansion_slice = np.s_[:, np.newaxis]

        # Collect S(w) at q = 1/Å without DW factors
        bins = self._fine_bins if self._use_autoconvolution else self._bins
        bin_centres = self._fine_bin_centres if self._use_autoconvolution else self._bin_centres
        s_by_atom_and_order: SByAtomAndOrder = defaultdict(lambda: np.zeros_like(bin_centres, dtype=FLOAT_TYPE))

        atoms_data = self._abins_data.get_atoms_data()

        if self._isotropic_fundamentals or self._quantum_order_num > 1 or self._use_autoconvolution:
            for k_index in range(self._num_k):
                s_by_atom_and_order = self._add_s_contributions(
                    self._calculate_s_powder_over_atoms(
                        k_index=k_index,
                        q2=1.0,
                        bins=bins,
                        min_order=min_order,
                    ),
                    s_by_atom_and_order,
                )

            spectra = Spectrum1DCollection(
                x_data=(bin_centres * self.freq_unit),
                y_data=np.array([value for value in s_by_atom_and_order.values()]) * self.s_unit,
                metadata={
                    "scattering": "incoherent",
                    "line_data": [
                        {
                            "atom_index": atom_index,
                            "quantum_order": order,
                            "symbol": atoms_data[atom_index]["symbol"],
                            "mass": MASS_STR_FORMAT.format(atoms_data[atom_index]["mass"]),
                        }
                        for atom_index, order in s_by_atom_and_order.keys()
                    ],
                },
            )
        # Otherwise there is nothing to calculate, return suitable empty-ish data structure
        elif self.is_2d:
            return Spectrum2DCollection(
                x_data=(self._q_bins * self.q_unit),
                y_data=(bin_centres * self.freq_unit),
                z_data=np.zeros((1, len(self._q_bin_centres), len(bin_centres))) * self.s_unit,
                metadata={"quantum_order": 1},
            )
        else:
            return Spectrum1DCollection(
                x_data=(bin_centres * self.freq_unit), y_data=np.zeros((1, len(bin_centres))) * self.s_unit, metadata={"quantum_order": 1}
            )

        if self._use_autoconvolution:
            max_dw_order = self._autoconvolution_max
            self._report_progress(
                f"Finished calculating S(q,ω) to order {self._quantum_order_num} by "
                f"analytic powder-averaging. "
                f"Adding autoconvolution data up to order {max_dw_order}.",
                reporter=self.progress_reporter,
            )
            spectra = add_autoconvolution_spectra(spectra, max_order=self._autoconvolution_max, output_bins=(self._bins * self.freq_unit))
            # Tweak x-axis definition for compatibility with other objects
            spectra.x_data = self._bin_centres * self.freq_unit

        else:
            # (order, q, energy)
            max_dw_order = self._quantum_order_num

        # Compute appropriate q-dependence for each order, along with 1/(n!) term
        factorials = factorial(range(1, max_dw_order + 1))[order_expansion_slice]
        q2_order_corrections = q2 ** np.arange(1, max_dw_order + 1)[order_expansion_slice] / factorials

        if broaden:
            self._report_progress("Applying instrumental broadening to all orders with simple q-dependence")
            broadening_scheme = abins.parameters.sampling["broadening_scheme"]
            spectra = self._broaden_spectra(spectra, broadening_scheme=broadening_scheme)

        self._report_progress("Applying q^2n / n! q-dependence")
        if self.is_2d:
            # convert 1-D spectra sampled at q=1 to 2-D spectra by applying q-dependent factor
            z_data = np.empty((spectra.y_data.shape[0], q2.shape[0], spectra.y_data.shape[1]), dtype=FLOAT_TYPE) * ureg(spectra.y_data_unit)
            for i, (y_data_1d, metadata) in enumerate(zip(spectra.y_data, spectra.iter_metadata())):
                z_data[i] = y_data_1d * q2_order_corrections[metadata["quantum_order"] - 1]

            spectra = Spectrum2DCollection(
                x_data=(self._q_bins * self.q_unit), y_data=spectra.x_data, z_data=z_data, metadata=spectra.metadata
            )
        else:
            for i, metadata in enumerate(spectra.iter_metadata()):
                spectra._y_data[i] *= q2_order_corrections[metadata["quantum_order"] - 1]

        if isinstance(spectra, Spectrum1DCollection):
            get_raw_s = attrgetter("_y_data")
        else:
            get_raw_s = attrgetter("_z_data")

        if self._isotropic_fundamentals or (self._quantum_order_num > 1) or self._use_autoconvolution:
            self._report_progress(
                f"Applying isotropic Debye-Waller factor to orders {min_order} and above.", reporter=self.progress_reporter
            )
            iso_dw = self.calculate_isotropic_dw(q2=q2[order_expansion_slice[:-1]])

            for i, metadata in enumerate(spectra.iter_metadata()):
                if metadata["quantum_order"] < min_order:
                    continue

                atom_index = metadata["atom_index"]
                get_raw_s(spectra)[i] *= iso_dw[atom_index]

        return spectra

    def _calculate_s_powder_over_k_and_q(self) -> Spectrum2DCollection:
        """Calculate S along a set of q-points in semi-analytic powder-averaging approximation

        This data is averaged over the phonon k-points and Debye-Waller factors
        are included.

        Instrumental broadening is applied in-place: for all terms with simple q-dependence,
        broadening is applied in 1D before q-dependence is applied. For fundamentals with
        mode-dependent Debye-Waller factor, broadening must be calculated for each q bin.

        The resulting spectra have the shape (n_qpts, n_ebins)
        """
        q2 = (self._q_bin_centres**2)[:, np.newaxis]

        isotropic_spectra = self._calculate_s_isotropic(q2, broaden=True)

        self._report_progress("Calculating fundamentals with mode-dependent Debye-Waller factor", reporter=self.progress_reporter)

        fundamentals_spectra_with_dw = self._calculate_fundamentals_over_k(q2=q2)

        self._report_progress("Broadening fundamentals", reporter=self.progress_reporter)
        broadening_scheme = abins.parameters.sampling["broadening_scheme"]

        fundamentals_spectra_with_dw = self._broaden_spectra(fundamentals_spectra_with_dw, broadening_scheme=broadening_scheme)

        if multiphonon_indices := [i for i, metadata in enumerate(isotropic_spectra.iter_metadata()) if metadata["quantum_order"] > 1]:
            return isotropic_spectra[multiphonon_indices] + fundamentals_spectra_with_dw

        else:
            return fundamentals_spectra_with_dw

    def _calculate_s_powder_over_k(self, *, angle: float) -> SpectrumCollection:
        """Calculate S for a given angle in semi-analytic powder-averaging approximation

        This data is averaged over the phonon k-points and Debye-Waller factors
        are included.

        Args:
            angle: Scattering angle used to determine energy-q relationship
        """
        # Get q^2 series corresponding to energy bins
        q2 = self._instrument.calculate_q_powder(input_data=self._bin_centres, angle=angle)

        spectra = self._calculate_s_isotropic(q2, broaden=False)

        # Finally we (re)calculate the first-order spectrum with more accurate DW method
        if not self._isotropic_fundamentals:
            fundamentals_spectra_with_dw = self._calculate_fundamentals_over_k(angle=angle)
            isotropic_multiphonon_indices = [i for i, metadata in enumerate(spectra.iter_metadata()) if metadata["quantum_order"] > 1]
            if isotropic_multiphonon_indices:
                spectra = spectra[isotropic_multiphonon_indices] + fundamentals_spectra_with_dw
            else:
                spectra = fundamentals_spectra_with_dw

        return spectra

    def calculate_isotropic_dw(self, *, q2: np.ndarray) -> np.ndarray:
        """Compute Debye-Waller factor in isotropic approximation for current system

        For 1-D data, q2 should be a row vector corresponding to frequency bin centres

        For 2-D data, q2 should have shape (n_q, 1, 1) to support numpy
        broadcasting over atoms and frequencies

        Returns an N_atoms x N_frequencies array.
        """
        average_a_traces = np.sum(
            [self._powder_data.get_a_traces(k_index) * kpoint.weight for k_index, kpoint in enumerate(self._abins_data.get_kpoints_data())],
            axis=0,
        )
        iso_dw = self._isotropic_dw(q2=q2, a_trace=average_a_traces[:, None])

        # For 2-D case we need to reshuffle axes after numpy broadcasting
        if len(iso_dw.shape) == 3:
            # From q, atom, freq -> atom, q, freq
            iso_dw = np.swapaxes(iso_dw, 0, 1)

        return iso_dw

    @staticmethod
    def _isotropic_dw(*, q2, a_trace):
        """Compute Debye-Waller factor in isotropic approximation"""
        return np.exp(-q2 * a_trace / 3)

    @staticmethod
    def _apply_resolution(frequencies: np.ndarray, bins: np.ndarray, s_dft: np.ndarray, scheme: str, instrument: Instrument) -> np.ndarray:
        """Apply instrument resolution function to a row of data

        This is a pure function for use with multiprocessing Pool.map;
        otherwise we have to pickle self and that fails when hitting the
        logger.  (In principle the problem could also be avoided by
        composition; a simpler class with the calculation state/parameters
        could be stored.)

        """
        return instrument.convolve_with_resolution_function(frequencies=frequencies, bins=bins, s_dft=s_dft, scheme=scheme)[1]

    def _broaden_spectra(self, spectra: SpectrumCollection, broadening_scheme: str = "auto") -> SpectrumCollection:
        """
        Apply instrumental broadening to scattering data

        If the data is 2D, process line-by-line.
        (There is room for improvement, by reworking all the broadening
        implementations to accept 2-D input.)
        """
        if isinstance(spectra, Spectrum1DCollection):
            frequencies = spectra.x_data.to(self.freq_unit).magnitude

            broadened_spectra = map(
                partial(self._apply_resolution, frequencies, self._bins, scheme=broadening_scheme, instrument=self._instrument),
                spectra._y_data,
            )
            broadened_spectra = list(broadened_spectra)

            return Spectrum1DCollection(
                x_data=spectra.x_data,
                y_data=(np.asarray(broadened_spectra, dtype=FLOAT_TYPE) * spectra.y_data.units),
                metadata=spectra.metadata,
            )

        else:  # 2-D data, parallelize over columns
            broadened_spectra = []

            frequencies = spectra.get_bin_centres(bin_ax="y").to(self.freq_unit).magnitude

            # For efficient parallel map() operation, we view the 3-D
            # (contributions, q_rows, frequencies) as a 2-D (contributions*q_rows, frequencies)
            # sequence of spectra, and reshape afterwards

            # Perform resolution precomputations (if appropriate) before parallel map
            self._instrument.prepare_resolution()

            z_data_magnitude = spectra.z_data.to(self.s_unit).magnitude
            s_rows = np.reshape(z_data_magnitude, (-1, z_data_magnitude.shape[-1]))

            broadened_spectra = map(
                partial(self._apply_resolution, frequencies, self._bins, scheme=broadening_scheme, instrument=self._instrument),
                s_rows,
            )
            broadened_spectra = list(broadened_spectra)

            return Spectrum2DCollection(
                x_data=spectra.x_data,
                y_data=spectra.y_data,
                z_data=ureg.Quantity(np.asarray(broadened_spectra).reshape(z_data_magnitude.shape), self.s_unit),
                metadata=spectra.metadata,
            )

    def _calculate_fundamentals_over_k(self, angle: float = None, q2: np.ndarray = None) -> SpectrumCollection:
        """
        Calculate order-1 incoherent S with mode-dependent DW correction

        Atomic cross-sections are not included at this stage.
        Values are averaged over the k-points included in self._abins_data.

        If autoconvolution is to be applied, the base S values will be binned to a fine energy grid.
        The DW-corrected values are always binned to the standard energy grid.

        Either angle or q2 must be provided.

        Args:
            angle:
                Scattering angle in degrees (used to calculate q-points corresponding to energies)
            q2:
                Fixed q-point(s) - related to energies by numpy broadcasting
                (i.e. scalar used for all energies, row vector corresponds to
                energies, column vector adds q-sampling dimension).

        returns:
            SpectrumCollection for fundamentals including mode-dependent Debye-Waller factor

        """
        from abins.constants import FLOAT_TYPE, MASS_STR_FORMAT

        self._report_progress("Calculating fundamentals with mode-dependent Debye-Waller factor.", reporter=self.progress_reporter)

        if (angle is None) == (q2 is None):  # XNOR
            raise ValueError("Exactly one should be set: angle or q2")

        atoms_data = self._abins_data.get_atoms_data()

        if angle is not None:
            s_array = np.zeros((len(atoms_data), len(self._bin_centres)))
        elif len(q2.shape) == 2:
            n_q, _ = q2.shape
            s_array = np.zeros((len(atoms_data), n_q, len(self._bin_centres)), dtype=FLOAT_TYPE)
        else:
            raise Exception("Expected a scattering angle or 2-D q array")

        for k_index, kpoint in enumerate(self._abins_data.get_kpoints_data()):
            frequencies = self._powder_data.get_frequencies()[k_index]
            if angle is not None:
                q2 = self._instrument.calculate_q_powder(input_data=frequencies, angle=angle)

            a_tensors = self._powder_data.get_a_tensors()[k_index]
            a_traces = self._powder_data.get_a_traces(k_index)
            b_tensors = self._powder_data.get_b_tensors()[k_index]
            b_traces = self._powder_data.get_b_traces(k_index)

            for atom_index, atom_label in enumerate(self._abins_data.get_atoms_data().extract()):
                s = self._calculate_order_one(
                    q2=q2,
                    frequencies=frequencies,
                    a_tensor=a_tensors[atom_index],
                    a_trace=a_traces[atom_index],
                    b_tensor=b_tensors[atom_index],
                    b_trace=b_traces[atom_index],
                )

                dw = self._calculate_order_one_dw(
                    q2=q2,
                    frequencies=frequencies,
                    a_tensor=a_tensors[atom_index],
                    a_trace=a_traces[atom_index],
                    b_tensor=b_tensors[atom_index],
                    b_trace=b_traces[atom_index],
                )
                weights = s * dw * kpoint.weight
                if len(weights.shape) == 1:
                    weights = weights[np.newaxis, :]

                rebinned_s_with_dw = np.array(
                    [np.histogram(frequencies, bins=self._bins, weights=row, density=False)[0] for row in weights]
                )

                if len(rebinned_s_with_dw) == 1:
                    rebinned_s_with_dw = rebinned_s_with_dw[0]

                s_array[atom_index] += rebinned_s_with_dw

        atoms_data = self._abins_data.get_atoms_data()
        metadata = {
            "scattering": "incoherent",
            "quantum_order": 1,
            "line_data": [
                {"atom_index": atom_index, "symbol": atom_data["symbol"], "mass": MASS_STR_FORMAT.format(atom_data["mass"])}
                for atom_index, atom_data in enumerate(atoms_data)
            ],
        }

        match q2.shape:
            case (n_q, 1):
                return Spectrum2DCollection(
                    x_data=(self._q_bins * self.q_unit),
                    y_data=(self._bin_centres * self.freq_unit),
                    z_data=(s_array * self.s_unit),
                    metadata=metadata,
                )

            case (n_q,):
                return Spectrum1DCollection(x_data=(self._bin_centres * self.freq_unit), y_data=(s_array * self.s_unit), metadata=metadata)
            case _:
                raise ValueError("Unexpected shape of q2 array")

    @staticmethod
    def _add_s_contributions(a: SByAtomAndOrder, b: SByAtomAndOrder) -> SByAtomAndOrder:
        """Combine contributions to S(atom, order) from different calculations

        a and b are assumed to have the same set of keys: this is not checked
        """
        return {key: a[key] + b[key] for key in a}

    def _calculate_s_powder_over_atoms(self, *, k_index: int, q2: np.ndarray, bins: np.ndarray, min_order: int = 1) -> SByAtomAndOrder:
        """
        Evaluates S for all atoms for the given q-point and checks if S is consistent.

        Debye-Waller terms are not included at this stage.

        Powder data should have already been initialised and stored in self._powder_data

        :param k_index: Index of k-point from calculated phonon data
        :param q2: Array of squared absolute q-point values in angstrom^-2. (Columns correspond to energies.)
        :param bins: Frequency bins
        :param min_order: Lowest quantum order to evaluate. (The max is determined by self._quantum_order_num.)

        """
        assert min_order in (1, 2)  # Cannot start higher than 2; need information about combinations

        results: SByAtomAndOrder = {}

        for atom_index in range(self._num_atoms):
            self._report_progress(msg=f"Calculating S for atom {atom_index}, k-point {k_index}", reporter=self.progress_reporter)
            results.update(self._calculate_s_powder_one_atom(atom_index=atom_index, k_index=k_index, q2=q2, bins=bins, min_order=min_order))

        return results

    def _calculate_s_powder_one_atom(
        self, *, atom_index: int, k_index: int, q2: np.ndarray, bins: np.ndarray, min_order: int = 1
    ) -> SByAtomAndOrder:
        """
        :param atom_index: number of atom
        :param k_index: Index of k-point in phonon data
        :param q2: Array of squared absolute q-point values in angstrom^-2. (Columns correspond to energies.)
        :bins: Frequency bins
        :min_order: Lowest quantum order to evaluate. (The max is determined by self._quantum_order_num.)

        """
        results: SByAtomAndOrder = {}
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

        calculate_order = {1: self._calculate_order_one, 2: self._calculate_order_two}

        # Chunking to save memory has been removed pending closer examination
        for order in range(min_order, self._quantum_order_num + 1):
            frequencies, coefficients = FrequencyPowderGenerator.construct_freq_combinations(
                previous_array=frequencies,
                previous_coefficients=coefficients,
                fundamentals_array=fundamentals,
                fundamentals_coefficients=fund_coeff,
                quantum_order=order,
            )

            scattering_intensities = calculate_order[order](
                q2=q2, frequencies=frequencies, indices=coefficients, a_tensor=a_tensor, a_trace=a_trace, b_tensor=b_tensor, b_trace=b_trace
            )
            rebinned_spectrum, _ = np.histogram(frequencies, bins=bins, weights=(scattering_intensities * kpoint_weight), density=False)

            results[(atom_index, order)] = rebinned_spectrum

            # Prune modes with low intensity; these are assumed not to contribute to higher orders
            frequencies, coefficients = self._calculate_s_over_threshold(scattering_intensities, freq=frequencies, coeff=coefficients)

        return results

    @staticmethod
    def _calculate_s_over_threshold(s=None, freq=None, coeff=None):
        """
        Discards frequencies for small S.
        :param s: numpy array with S for the given order quantum event and atom
        :param freq: frequencies which correspond to s
        :param coeff: coefficients which correspond to  freq

        :returns: freq, coeff corresponding to S greater than abins.parameters.sampling['s_absolute_threshold']
        """

        indices = s > abins.parameters.sampling["s_absolute_threshold"]

        # Mask out small values, but avoid returning an array smaller than MIN_SIZE
        if np.count_nonzero(indices) >= MIN_SIZE:
            freq = freq[indices]
            coeff = coeff[indices]

        else:
            freq = freq[:MIN_SIZE]
            coeff = coeff[:MIN_SIZE]

        return freq, coeff

    def _calculate_order_one_dw(
        self,
        *,
        q2: np.ndarray,
        frequencies: np.ndarray,
        a_tensor=None,
        a_trace=None,
        b_tensor=None,
        b_trace=None,
    ):
        """
        Calculate mode-dependent Debye-Waller factor for the first order quantum event for one atom.
        :param q2: squared values of momentum transfer vectors
        :param frequencies: frequencies for which transitions occur
        :param a_tensor: total MSD tensor for the given atom
        :param a_trace: total MSD trace for the given atom
        :param b_tensor: frequency dependent MSD tensor for the given atom
        :param b_trace: frequency dependent MSD trace for the given atom
        :returns: s for the first quantum order event for the given atom
        """
        trace_ba = np.einsum("kli, il->k", b_tensor, a_tensor)

        dw = np.exp(-q2 * (a_trace + 2.0 * trace_ba / b_trace) / 5.0)

        return dw

    def _calculate_order_one(
        self, *, q2: np.ndarray, frequencies: np.ndarray, a_tensor=None, a_trace=None, b_tensor=None, b_trace=None, indices=None
    ):
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

        return q2 * b_trace / 3.0

    def _calculate_order_two(self, q2=None, frequencies=None, indices=None, a_tensor=None, a_trace=None, b_tensor=None, b_trace=None):
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

        # fmt: off
        s = q4 * (np.prod(np.take(b_trace, indices=indices), axis=1)
                  + np.einsum('kli, kil->k',
                              np.take(b_tensor, indices=indices[:, 0], axis=0),
                              np.take(b_tensor, indices=indices[:, 1], axis=0))
                  + np.einsum('kli, kil->k',
                              np.take(b_tensor, indices=indices[:, 1], axis=0),
                              np.take(b_tensor, indices=indices[:, 0], axis=0))
                  ) / (15. * factor)
        # fmt: on
        return s
