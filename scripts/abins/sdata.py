# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import collections.abc
from copy import deepcopy
from itertools import repeat
from numbers import Integral, Real
from operator import itemgetter
import re
from typing import (
    Any,
    Dict,
    Generator,
    Iterable,
    List,
    Literal,
    Optional,
    overload,
    Sequence,
    Tuple,
    TypeVar,
    Union,
)
from typing_extensions import Self

from euphonic import Quantity, ureg
from euphonic.spectra import Spectrum, Spectrum1DCollection, Spectrum2D
from euphonic.validate import _check_constructor_inputs, _check_unit_conversion
import numpy as np
from pydantic import BaseModel, ConfigDict, PositiveFloat, validate_call
from scipy.signal import convolve

import abins
from abins.constants import ALL_KEYWORDS_ATOMS_S_DATA, ALL_SAMPLE_FORMS, ALL_SAMPLE_FORMS_TYPE, ATOM_LABEL, FLOAT_TYPE, S_LABEL
from abins.instruments.directinstrument import DirectInstrument
from abins.logging import get_logger, Logger
import abins.parameters

# Type annotation for atom items e.g. data['atom_1']
OneAtomSData = Dict[str, np.ndarray]

# Type annotation for dat input {'atom_0': {'s': {'order_1': array( ...
SDataInputDict = Dict[str, Dict[str, Dict[str, np.ndarray]]]

SD = TypeVar("SD", bound="SData")
SDBA = TypeVar("SDBA", bound="SDataByAngle")


class SData(collections.abc.Sequence, BaseModel):
    """
    Class for storing S(Q, omega) with relevant metadata

    Indexing will return dict(s) of S by quantum order for atom(s)
    corresponding to index/slice.

    Args:
        data:
            Scattering data as 1-D or 2-D arrays, arranged by atom and quantum order

                    {'atom_0': {'s': {'order_1': array([[s11, s12, s13, ...]
                                                  [s21, s22, s23, ...], ...])
                                                  'order_2': ...}},
                     'atom_1': ...}

                where array rows correspond to q-points and columns correspond
                to frequencies. If q-points and energies are not independent
                (e.g. indirect-geometry spectrum at a given angle) 1-D arrays are used.

    """

    model_config = ConfigDict(arbitrary_types_allowed=True, strict=True)

    from pydantic import PrivateAttr

    _data: Dict[str, Any] = PrivateAttr()

    frequencies: np.ndarray
    temperature: Optional[PositiveFloat] = None
    sample_form: ALL_SAMPLE_FORMS_TYPE = "Powder"
    q_bins: Optional[np.ndarray] = None

    # Usually the custom validation steps go into model_post_init(), but this
    # would run before we assigned self._data so here we use __init__ to sequence things
    @validate_call(config=ConfigDict(arbitrary_types_allowed=True, strict=True))
    def __init__(self, data: SDataInputDict, **kwargs):
        super().__init__(**kwargs)
        self._data = data
        self._check_data()

    def model_post_init(self, __context):
        self.frequencies = np.asarray(self.frequencies, dtype=FLOAT_TYPE)
        self._check_frequencies()

        if self.q_bins is not None:
            self.q_bins = np.asarray(self.q_bins, dtype=FLOAT_TYPE)
        self._check_q_bins()

    def get_spectrum_collection(
        self, symbols: Iterable[str] = None, masses: Iterable[float] = None
    ) -> Union["AbinsSpectrum1DCollection", "AbinsSpectrum2DCollection"]:
        from abins.constants import MASS_STR_FORMAT

        if symbols is None:
            symbols = repeat(None)

        if masses is None:
            masses = repeat(None)
        else:
            masses = (MASS_STR_FORMAT.format(mass) for mass in masses)

        frequencies = self.get_frequencies() * ureg("1/cm")
        metadata = {"scattering": "incoherent", "line_data": []}

        s_array = []

        for atom_index, symbol, mass in zip(range(len(self)), symbols, masses):
            atom_data = self[atom_index]
            for order, order_data in atom_data.items():
                s_array.append(order_data)
                metadata["line_data"].append(
                    {"atom_index": atom_index, "symbol": symbol, "mass": mass, "quantum_order": int(order.split("_")[-1])}
                )

        if self.get_q_bins() is not None:
            q_bins = self.get_q_bins() * ureg("1 / angstrom")
            return AbinsSpectrum2DCollection(
                x_data=q_bins, y_data=frequencies, z_data=np.asarray(s_array) * ureg("barn / (1/cm)"), metadata=metadata
            )

        return AbinsSpectrum1DCollection(x_data=frequencies, y_data=np.asarray(s_array) * ureg("barn / (1/cm)"), metadata=metadata)

    def update(self, sdata: "SData") -> None:
        """Update the data by atom and order

        This can be used to change values or to append additional atoms/quantum orders

        Args:
            sdata: another SData instance with the same frequency series.
                Spectra will be updated by atom and by quantum order; i.e.
                - elements in both old and new sdata will be replaced with new value
                - elements that only exist in the old data will be untouched
                - elements that only exist in the new data will be appended as
                  new entries to the old data
        """

        if not np.allclose(self.frequencies, sdata.get_frequencies()):
            raise ValueError("Cannot update SData with inconsistent frequencies")

        for atom_key, atom_data in sdata._data.items():
            if atom_key in self._data:
                for order, order_data in sdata._data[atom_key]["s"].items():
                    self._data[atom_key]["s"][order] = order_data
            else:
                self._data[atom_key] = atom_data

    def add_dict(self, data: dict) -> None:
        """Add data in dict form to existing values.

        These atoms/orders must already be present; use self.update() to add new data.
        """

        for atom_key, atom_data in data.items():
            for order, order_data in atom_data["s"].items():
                self._data[atom_key]["s"][order] += order_data

    def apply_dw(self, dw: np.array, min_order=1, max_order=2) -> None:
        """Multiply S by frequency-dependent scale factor for all atoms

        Args:
            dw: Numpy array with dimensions (N_atoms, N_frequencies)
            min_order: Lowest quantum order of data to process
            max_order: Highest quantum order of data to process
        """
        for atom_index, dw_row in enumerate(dw):
            atom_key = f"atom_{atom_index}"
            atom_data = self._data.get(atom_key)
            if atom_data is None:
                raise IndexError("Atoms in SData do not match dimensions of Debye-Waller data")

            for order in range(min_order, max_order + 1):
                order_key = f"order_{order}"
                self._data[atom_key]["s"][order_key] *= dw_row

    @classmethod
    def get_empty(
        cls: SD, *, frequencies: np.ndarray, atom_keys: Sequence[str], order_keys: Sequence[str], n_rows: Optional[int] = None, **kwargs
    ) -> SD:
        """Construct data container with zeroed arrays of appropriate dimensions

        This is useful as a starting point for accumulating data in a loop.

        Args:

            angles: inelastic scattering angles

            frequencies: inelastic scattering energies

            atom_keys:
                keys for atom data sets, corresponding to keys of ``data=``
                init argument of SData() of SDataByAngle(). Usually this is
                ['atom_0', 'atom_1', ...]

            order_keys:
                keys for quantum order

            n_rows:
                If provided, SData is filled with 2-D arrays of dimensions
                (n_rows, len(frequencies)).

            **kwargs:
                remaining keyword arguments will be passed to class constructor
                (Usually these would be ``temperature=`` and ``sample_form=``.)

        Returns:
            Empty data collection with appropriate dimensions and metadata
        """

        n_frequencies = len(frequencies)
        if n_rows is None:
            shape = n_frequencies
        else:
            shape = (n_rows, n_frequencies)

        data = {atom_key: {"s": {order_key: np.zeros(shape) for order_key in order_keys}} for atom_key in atom_keys}

        return cls(data=data, frequencies=frequencies, **kwargs)

    def get_frequencies(self) -> np.ndarray:
        return self.frequencies.copy()

    def get_q_bins(self) -> np.ndarray:
        if self.q_bins is None:
            return None
        else:
            return self.q_bins.copy()

    def get_q_bin_centres(self) -> np.ndarray:
        q_bins = self.get_q_bins()
        if q_bins is None:
            raise ValueError("No q_bins on this SData")
        else:
            return (q_bins[1:] + q_bins[:-1]) / 2.0

    def set_q_bins(self, q_bins: np.ndarray) -> None:
        """Update q-bins stored on SData

        Args:
            q_bins: 1-D set of q-point bin edges surrounding rows in S data
                (e.g. for q-bins=[1., 2., 3.], S arrays will have two rows
                 corresponding to q =1 to 2 Å^-1, q =2 to 3 Å^-1; typically these
                 are evaluated at the bin centres 1.5, 2.5 Å^-1.)
        """
        self.q_bins = q_bins
        self._check_q_bins()
        self._check_data()

    def get_temperature(self) -> Union[float, None]:
        return self.temperature

    def get_sample_form(self) -> str:
        return self.sample_form

    def get_bin_width(self) -> Union[float, None]:
        """Check frequency series and return the bin size

        If the frequency series does not have a consistent step size, return None
        """
        self._check_frequencies()
        step_size = (self.frequencies[-1] - self.frequencies[0]) / (self.frequencies.size - 1)

        if np.allclose(step_size, self.frequencies[1:] - self.frequencies[:-1]):
            return step_size
        else:
            return None

    def get_total_intensity(self) -> np.ndarray:
        """Sum over all atoms and quantum orders to a single spectrum"""

        # Find a spectrum to get initial shape
        for atom_data in self:
            for order_key, data in atom_data.items():
                total = np.zeros_like(data)
                break
            break

        for atom_data in self:
            for order_key, data in atom_data.items():
                total += data
        return total

    def _check_frequencies(self):
        # Check frequencies are ordered low to high
        if not np.allclose(np.sort(self.frequencies), self.frequencies):
            raise ValueError("Frequencies not sorted low to high")

    def _check_q_bins(self):
        if (self.q_bins is not None) and (len(self.q_bins.shape) != 1):
            raise IndexError("Q-bins should be a 1-D array")

    def _check_data(self):
        """Check data set is consistent and has correct types"""
        n_frequencies = self.frequencies.size
        if self.q_bins is None:
            expected_shapes = [(n_frequencies,), (1, n_frequencies)]
        else:
            expected_shapes = [
                (self.q_bins.size - 1, n_frequencies),
            ]

        for key, item in self._data.items():
            if not re.match(rf"{ATOM_LABEL}_\d+", key):
                raise ValueError("Data keys must have form {ATOM_LABEL}_1, {ATOM_LABEL}_2, ...")

            if sorted(item.keys()) != sorted(ALL_KEYWORDS_ATOMS_S_DATA):
                raise ValueError("Invalid structure of the dictionary.")

            for order in item[S_LABEL]:
                if item[S_LABEL][order].shape not in expected_shapes:
                    raise ValueError(
                        f"SData not dimensionally consistent with frequencies / q-bins "
                        f"for {key}, {order}. "
                        f"Expected shape " + " or ".join(map(str, expected_shapes)) + f"; got shape {item[S_LABEL][order].shape}"
                    )

    def extract(self):
        """
        Returns the data.
        :returns: data
        """
        # Use a shallow copy so that 'frequencies' is not added to self._data
        full_data = self._data.copy()
        full_data.update({"frequencies": self.frequencies})
        if self.q_bins is not None:
            full_data.update({"q_bins": self.q_bins})
        return full_data

    def rebin(self, bins: np.array) -> "SData":
        """Re-bin the data to a new set of frequency bins

        Data is resampled using np.histogram; no smoothing/interpolation takes
        place, this is generally intended for moving to a coarser grid.

        Args: New sampling bin edges.

        Returns:
            A new SData object with resampled data.
        """
        old_frequencies = self.get_frequencies()
        new_frequencies = (bins[:-1] + bins[1:]) / 2

        new_data = {
            atom_key: {
                "s": {
                    order_key: np.histogram(old_frequencies, bins=bins, weights=order_data, density=0)[0]
                    for order_key, order_data in atom_data["s"].items()
                }
            }
            for atom_key, atom_data in self._data.items()
        }

        return self.__class__(
            data=new_data, frequencies=new_frequencies, temperature=self.get_temperature(), sample_form=self.get_sample_form()
        )

    @staticmethod
    def _get_highest_existing_order(atom_data: dict) -> int:
        """Check atom_data['s'] for highest existing data order

        Assumes that there are no gaps, so will run order_1, order_2... until
        a missing key is identified.

        If there is no existing order_1, return 0.
        """
        from itertools import count

        for order_index in count(start=1):
            if f"order_{order_index}" not in atom_data["s"]:
                break

        return order_index - 1

    def add_autoconvolution_spectra(self, max_order: Optional[int] = None) -> None:
        """
        Atom-by-atom, add higher order spectra by convolution with fundamentals

        Strictly this is only autoconvolution when forming order-2 from order-1;
        higher orders are formed by repeated convolution with the fundamentals.

        Data should not have been broadened before applying this operation,
        or this will lead to repeated broadening of higher orders.

        The process will begin with the highest existing order, and repeat until
        a spectrum of MAX_ORDER is obtained.
        """
        if max_order is None:
            max_order = abins.parameters.autoconvolution["max_order"]

        for atom_key, atom_data in self._data.items():
            fundamental_spectrum = atom_data["s"]["order_1"]

            for order_index in range(self._get_highest_existing_order(atom_data), max_order):
                spectrum = convolve(atom_data["s"][f"order_{order_index}"], fundamental_spectrum, mode="full")[: fundamental_spectrum.size]
                self._data[atom_key]["s"][f"order_{order_index + 1}"] = spectrum

    def check_thresholds(self, logger: Optional[Logger] = None, logging_level: str = "warning") -> List[Tuple[int, int, float]]:
        """
        Compare the S data values to minimum thresholds and warn if the threshold appears large relative to the data

        Warnings will be raised if [max(S) * s_relative_threshold] is less than s_absolute_threshold. These
        thresholds are defined in the abins.parameters.sampling dictionary.

        :param logger: Alternative logging object. (Defaults to Mantid logger)
        :param logging_level: logging level of warnings that a significant
            portion of S is being removed. Usually this will be 'information' or 'warning'.

        :returns: a list of cases which failed the test, as tuples of
            ``(atom_key, order_number, max(S))``.

        """
        return check_thresholds(self._unpack_data(), logger=logger, logging_level=logging_level)

    def _unpack_data(self) -> Generator[Tuple[int, int, np.ndarray], None, None]:
        for atom_index, OneAtomSData in enumerate(self):
            for order_key, s in OneAtomSData.items():
                order_index = int(order_key.split("_")[-1])
                yield (atom_index, order_index, s)

    def apply_kinematic_constraints(self, instrument: DirectInstrument) -> None:
        """Replace inaccessible intensity bins with NaN

        This passes frequencies to instrument.get_abs_q_limits() method to get
        accessible q-range at each energy, and masks out other bins with NaN.
        Data must be 2-D and q_bins must be available.

        Values will be modified in-place.

        Args:
            instrument - this must have the get_abs_q_limits() method
                (i.e. inherit DirectInstrument).
        """
        q_lower, q_upper = instrument.get_abs_q_limits(self.get_frequencies())
        q_values = self.get_q_bin_centres()
        mask = np.logical_or(q_values[:, np.newaxis] < q_lower[np.newaxis, :], q_values[:, np.newaxis] > q_upper[np.newaxis, :])
        for atom_data in self:
            for _, order_data in atom_data.items():
                order_data[mask] = float("nan")

    def __mul__(self, other: np.ndarray) -> "SData":
        """Multiply S data by an array over energies and orders

        Columns correspond to energies, rows correspond to quantum orders.
        All atoms will be included; for data over atoms and energies use the
        .apply_dw() method.

        """
        new_sdata = SData(
            data=deepcopy(self._data),
            frequencies=self.get_frequencies(),
            temperature=self.get_temperature(),
            sample_form=self.get_sample_form(),
            q_bins=self.get_q_bins(),
        )
        new_sdata *= other

        return new_sdata

    def __imul__(self, other: Union[float, np.ndarray]) -> None:
        """Multiply S data in-place by an array over energies and orders

        Columns correspond to energies, rows correspond to quantum orders.
        All atoms will be included; for data over atoms and energies use the
        .apply_dw() method.

        """
        if isinstance(other, float):
            for atom_data in self:
                for order, weights in atom_data.items():
                    weights *= other
            return self

        if isinstance(other, np.ndarray) and len(other.shape) == 1:
            other = other[np.newaxis, :]
        elif isinstance(other, np.ndarray) and len(other.shape) in (2, 3):
            pass
        else:
            raise IndexError("Can only multiply SData by a scalar float, 1- or 2-D array. ")

        for order_index, order_multiplier in enumerate(other):
            for atom_data in self:
                if len(atom_data[f"order_{order_index + 1}"].shape) == 1 and len(other.shape) == 3:
                    atom_data[f"order_{order_index + 1}"] = atom_data[f"order_{order_index + 1}"][np.newaxis, :] * order_multiplier
                else:
                    atom_data[f"order_{order_index + 1}"] *= order_multiplier

        return self

    def __str__(self):
        return "Dynamical structure factors data"

    def __len__(self) -> int:
        return len(self._data)

    @overload  # F811
    def __getitem__(self, item: int) -> OneAtomSData: ...

    @overload  # F811
    def __getitem__(self, item: slice) -> List[OneAtomSData]:  # F811
        ...

    def __getitem__(self, item):  # F811
        if isinstance(item, int):
            try:
                return self._data[f"atom_{item}"]["s"]
            except KeyError:
                raise IndexError(item)
        elif isinstance(item, slice):
            return [self[i] for i in range(len(self))[item]]
        else:
            raise TypeError("Indices must be integers or slices, not {}.".format(type(item)))


def _iter_check_thresholds(items: Iterable[Tuple[int, int, np.ndarray]]) -> Generator[Tuple[int, int, float], None, None]:
    """Compare S data values to minimum thresholds, return items with low intensity

    Items have form (atom_index, quantum_order_index, s_array)

    returned items have form (atom_index, quantum_order, max(s_array))
    """

    absolute_threshold = abins.parameters.sampling["s_absolute_threshold"]
    relative_threshold = abins.parameters.sampling["s_relative_threshold"]

    for atom_index, quantum_order, s in items:
        if (max_s := max(s.flatten())) * relative_threshold < absolute_threshold:
            yield (atom_index, quantum_order, max_s)


def check_thresholds(
    items: Iterable[Tuple[int, int, np.ndarray]], logger: Optional[Logger] = None, logging_level: str = "warning"
) -> List[Tuple[int, int, float]]:
    """
    Compare the S data values to minimum thresholds and warn if the threshold appears large relative to the data

    Warnings will be raised if [max(S) * s_relative_threshold] is less than s_absolute_threshold. These
    thresholds are defined in the abins.parameters.sampling dictionary.

    :param return_cases: If True, return a list of cases where S was small compared to threshold.
    :type return_cases: bool
    :param logger: Alternative logging object. (Defaults to Mantid logger)
    :param logging_level: logging level of warnings that a significant
        portion of S is being removed. Usually this will be 'information' or 'warning'.

    """
    absolute_threshold = abins.parameters.sampling["s_absolute_threshold"]
    relative_threshold = abins.parameters.sampling["s_relative_threshold"]

    logger = get_logger(logger=logger)
    logger_call = getattr(logger, logging_level)

    warning_cases = list(_iter_check_thresholds(items))

    if len(warning_cases) > 0:
        logger_call("Warning: some contributions had small S compared to threshold.")
        logger_call(
            f"The minimum S threshold ({absolute_threshold}) is greater than "
            f"{relative_threshold * 100}% of the maximum S for the following:"
        )

    for case in sorted(warning_cases, key=itemgetter(0, 1)):
        logger_call("{0}, {1}: max S {2:10.4E}".format(*case))

    return warning_cases


class SDataByAngle(collections.abc.Sequence):
    def __init__(
        self,
        *,
        data: Dict[str, OneAtomSData],
        angles: Sequence[float],
        frequencies: np.ndarray,
        temperature: Optional[float] = None,
        sample_form: ALL_SAMPLE_FORMS = "Powder",
    ) -> None:
        """Container for scattering spectra resolved by angle and atom

        Args:

            data:
                Scattering data as 2-d arrays arranged by atom and order::

                    {'atom_0': {'s': {'order_1': array([[s11, s12, s13, ...]
                                                  [s21, s22, s23, ...], ...])
                                                  'order_2': ...}},
                     'atom_1': ...}

                where array rows correspond to angles and columns correspond to
                frequencies.

            angles:
                scattering angles in degrees, corresponding to data

            frequencies:
                Inelastic scattering energies in cm^-1, corresponding to data

            temperature:
                Simulated scattering temperature

            sample_form:
                Sample form (used to track calculation method)
        """

        super().__init__()
        n_angles = len(angles)
        n_frequencies = len(frequencies)

        for atom_key, atom_data in data.items():
            for order, order_data in atom_data["s"].items():
                if order_data.shape != (n_angles, n_frequencies):
                    raise IndexError("SDataByAngle input should have 2D array in (angles, frequencies)")

        self.angles = list(angles)
        self._data = data
        self._metadata = {"frequencies": frequencies, "temperature": temperature, "sample_form": sample_form}

        self.frequencies = self._metadata["frequencies"]
        self.temperature = self._metadata["temperature"]
        self.sample_form = self._metadata["sample_form"]

    def __len__(self) -> int:
        return len(self.angles)

    @overload  # F811
    def __getitem__(self, item: int) -> SData: ...

    @overload  # F811
    def __getitem__(self: SDBA, item: slice) -> SDBA:  # F811
        ...

    def __getitem__(self, item):  # F811
        if isinstance(item, (int, slice)):
            data = {
                atom_index: {
                    "s": {order_index: self._data[atom_index]["s"][order_index][item, :] for order_index in self._data[atom_index]["s"]}
                }
                for atom_index in self._data
            }
        else:
            raise TypeError("Indices must be integers or slices, not {}.".format(type(item)))

        if isinstance(item, int):
            return SData(data=data, **self._metadata)

        else:  # Must be a slice, return angle-resolved data
            return type(self)(data=data, angles=self.angles[item], **self._metadata)

    @classmethod
    def get_empty(
        cls: SDBA, *, angles: Sequence[float], frequencies: np.ndarray, atom_keys: Sequence[str], order_keys: Sequence[str], **kwargs
    ) -> SDBA:
        """Construct data container with zeroed arrays of appropriate dimensions

        This is useful as a starting point for accumulating data in a loop.

        Args:

            angles: inelastic scattering angles

            frequencies: inelastic scattering energies

            atom_keys:
                keys for atom data sets, corresponding to keys of ``data=``
                init argument of SData() of SDataByAngle(). Usually this is
                ['atom_0', 'atom_1', ...]

            order_keys:
                keys for quantum order

            **kwargs:
                remaining keyword arguments will be passed to class constructor
                (Usually these would be ``temperature=`` and ``sample_form=``.)

        Returns:
            Empty data collection with appropriate dimensions and metadata
        """

        n_angles, n_frequencies = len(angles), len(frequencies)

        data = {atom_key: {"s": {order_key: np.zeros((n_angles, n_frequencies)) for order_key in order_keys}} for atom_key in atom_keys}

        return cls(data=data, angles=angles, frequencies=frequencies, **kwargs)

    def set_angle_data(self, angle_index: int, sdata: SData, add_to_existing: bool = False) -> None:
        """Set data for one angle from SData object

        Args:
            angle_index:
                Index (in self.angles) of angle corresponding to data

            sdata:
                New S values to replace current content at given angle

            add_to_existing:
                Instead of replacing existing data, values are summed together

        """

        data = sdata.extract()

        if "frequencies" in data:
            del data["frequencies"]

        self.set_angle_data_from_dict(angle_index, data, add_to_existing=add_to_existing)

    def set_angle_data_from_dict(self, angle_index: int, data: Dict[str, OneAtomSData], add_to_existing: bool = False) -> None:
        for atom_key, atom_data in data.items():
            for order_key, order_data in atom_data["s"].items():
                if add_to_existing:
                    self._data[atom_key]["s"][order_key][angle_index, :] += order_data
                else:
                    self._data[atom_key]["s"][order_key][angle_index, :] = order_data

    @classmethod
    @validate_call(config=ConfigDict(arbitrary_types_allowed=True, strict=True))
    def from_sdata_series(cls, data: Sequence[SData], *, angles: Sequence[float]) -> Self:
        metadata = {}

        if len(data) != len(angles):
            raise IndexError("Number of angles is not consistent with length of SData series")

        def near_enough(item, other):
            from math import isclose

            if isinstance(item, np.ndarray):
                return np.allclose(item, other)
            elif isinstance(item, float):
                return isclose(item, other)
            else:
                return item == other

        # First loop over data: collect and check metadata
        for sdata in data:
            for key in ("frequencies", "temperature", "sample_form"):
                if key not in metadata:
                    metadata[key] = getattr(sdata, f"get_{key}")()
                else:
                    if not near_enough(metadata[key], getattr(sdata, f"get_{key}")()):
                        raise ValueError(f"Property '{key}' must agree for all SData being collected.")

        atom_keys = list(data[0]._data.keys())
        sdata_collection = cls.get_empty(
            angles=angles, atom_keys=atom_keys, order_keys=list(data[0]._data[atom_keys[0]]["s"].keys()), **metadata
        )

        # Second loop over data: collect scattering data
        for angle_index, sdata in enumerate(data):
            sdata_collection.set_angle_data(angle_index, sdata)

        return sdata_collection

    def sum_over_angles(self, average: bool = False, weights: Sequence[float] = None) -> SData:
        """Combine S values over all angles

        :param average:
            Weight all angle contributions by 1/N, where N is number of angles

        :param weights:
            Weights corresponding to angles; total S is obtained by multiplying
            each angle by its weight and summing.

        :returns SData:

        """
        n_angles = len(self.angles)
        n_frequencies = len(self.frequencies)
        atom_keys = list(self._data.keys())
        order_keys = list(self._data[atom_keys[0]]["s"])

        if average:
            if weights is not None:
                raise ValueError("Cannot set weights while average=True")
            weights = np.full(n_angles, 1.0 / n_angles)
        elif weights:
            if len(weights) != n_angles:
                raise IndexError("Length of weights must match sampled angles")
        else:
            weights = np.ones(n_angles)

        assert isinstance(weights, (Sequence, np.ndarray))

        flattened_data = {atom_key: {"s": {order_key: np.zeros(n_frequencies) for order_key in order_keys}} for atom_key in atom_keys}

        for angle_index, weight in enumerate(weights):
            for atom_key in atom_keys:
                for order_key in order_keys:
                    flattened_data[atom_key]["s"][order_key] += weight * self._data[atom_key]["s"][order_key][angle_index, :]

        return SData(data=flattened_data, **self._metadata)


class AbinsSpectrum1DCollection(Spectrum1DCollection):
    """Minor patch to euphonic Spectrum1DCollection, to be moved upstream"""

    def group_by(self, *line_data_keys: str) -> Self:
        # Remove any keys that are in top-level of metadata rather than in
        # line_data: this means that all rows have the same value, so it is
        # not useful for splitting!
        keys = [key for key in line_data_keys if key not in self.metadata]

        if keys:
            return super().group_by(*keys)
        else:
            # No remaining keys to split on: return collection with single sum
            return self.from_spectra([self.sum()])

    def _get_line_data_vals(self, *line_data_keys: str) -> List[tuple]:
        """
        Get value of the key(s) for each element in
        metadata['line_data']. Returns a 1D array of tuples, where each
        tuple contains the value(s) for each key in line_data_keys, for
        a single element in metadata['line_data']. This allows easy
        grouping/selecting by specific keys

        For example, if we have a Spectrum1DCollection with the following
        metadata:
            {'desc': 'Quartz', 'line_data': [
                {'inst': 'LET', 'sample': 0, 'index': 1},
                {'inst': 'MAPS', 'sample': 1, 'index': 2},
                {'inst': 'MARI', 'sample': 1, 'index': 1},
            ]}
        Then:
            _get_line_data_vals('inst', 'sample') = [('LET', 0),
                                                     ('MAPS', 1),
                                                     ('MARI', 1)]

        Raises a KeyError if 'line_data' or the key doesn't exist
        """
        line_data = self.metadata["line_data"]
        line_data_vals = []
        for data in line_data:
            line_data_vals.append(tuple([data[key] for key in line_data_keys]))
        return line_data_vals


XTickLabels = Sequence[Tuple[int, str]]
LineData = Sequence[Dict[str, Union[str, int]]]
Metadata = Dict[str, Union[str, int, LineData]]


class AbinsSpectrum2DCollection(collections.abc.Sequence, Spectrum):
    """A collection of Spectrum2D with common x_data, y_data and x_tick_labels

    Intended for convenient storage/manipulation of contributions to a 2D
    spectrum. This object can be indexed or iterated to obtain individual
    Spectrum2D.

    Attributes
    ----------
    x_data
        Shape (n_x_data,) or (n_x_data + 1,) float Quantity. The x_data
        points (if size == (n_x_data,)) or x_data bin edges (if size
        == (n_x_data + 1,))
    y_data
        Shape (n_y_data,) or (n_y_data + 1,) float Quantity. The y_data
        bin points (if size == (n_y_data,)) or y_data bin edges (if size
        == (n_y_data + 1,))
    z_data
        Shape (n_entries, n_x_data, n_y_data) float Quantity. The plot data in
        z, in rows corresponding to separate 2D spectra
    x_tick_labels
        Sequence[Tuple[int, str]] or None. Special tick labels e.g. for
        high-symmetry points. The int refers to the index in x_data the
        label should be applied to
    metadata
        Dict[str, Union[int, str, LineData]] or None. Contains metadata
        about the spectra. Keys should be strings and values should be
        strings or integers.
        There are some functional keys:

          - 'line_data' : LineData
                          This is a Sequence[Dict[str, Union[int, str]],
                          it contains metadata for each spectrum in
                          the collection, and must be of length
                          n_entries
    """

    def __init__(
        self,
        x_data: Quantity,
        y_data: Quantity,
        z_data: Quantity,
        x_tick_labels: Optional[XTickLabels] = None,
        metadata: Optional[Metadata] = None,
    ):
        _check_constructor_inputs(
            [z_data, x_tick_labels, metadata],
            [Quantity, [list, type(None)], [dict, type(None)]],
            [(-1, -1, -1), (), ()],
            ["z_data", "x_tick_labels", "metadata"],
        )
        nx = z_data.shape[1]
        ny = z_data.shape[2]
        _check_constructor_inputs(
            [x_data, y_data],
            [Quantity, Quantity],
            [
                [
                    (nx,),
                    (nx + 1,),
                ],
                [(ny,), (ny + 1,)],
            ],
            ["x_data", "y_data"],
        )

        self._set_data(x_data, "x")
        self._set_data(y_data, "y")
        self._set_data(z_data, "z")
        self.x_tick_labels = x_tick_labels
        if metadata and "line_data" in metadata.keys():
            if len(metadata["line_data"]) != len(z_data):
                raise ValueError(
                    f'y_data contains {len(y_data)} spectra, but metadata["line_data"] contains {len(metadata["line_data"])} entries'
                )
        self.metadata = {} if metadata is None else metadata

    def _split_by_indices(self, indices: Union[Sequence[int], np.ndarray]) -> List[Self]:
        """Split data along x-axis at given indices"""
        ranges = self._ranges_from_indices(indices)
        return [
            type(self)(
                self.x_data[x0:x1],
                self.y_data,
                self.z_data[:, x0:x1, :],
                x_tick_labels=self._cut_x_ticks(self.x_tick_labels, x0, x1),
                metadata=self.metadata,
            )
            for x0, x1 in ranges
        ]

    def broaden(self: Self, **kwargs) -> Self:
        """Broaden all spectra

        See Spectrum2D.broaden() for **kwargs
        """
        return type(self).from_spectra([spectrum.broaden(**kwargs) for spectrum in self])

    def __add__(self, other: Self) -> Self:
        """Concatenate the z_data of two Spectrum2DCollection objects

        x_data and y_data must be the same, z_data must have compatible units

        Any metadata key/value pairs that are common to both spectra are
        retained in the top level dictionary, any others are put in the
        individual 'line_data' entries
        """
        return type(self).from_spectra([*self, *other])

    def __len__(self):
        return self.z_data.shape[0]

    @overload
    def __getitem__(self, item: int) -> Spectrum2D: ...

    @overload
    def __getitem__(self, item: slice) -> Self: ...

    @overload
    def __getitem__(self, item: Union[Sequence[int], np.ndarray]) -> Self: ...

    def __getitem__(self, item: Union[int, slice, Sequence[int], np.ndarray]):
        new_metadata = deepcopy(self.metadata)
        line_metadata = new_metadata.pop("line_data", [{} for _ in self._z_data])
        if isinstance(item, Integral):
            new_metadata.update(line_metadata[item])
            return Spectrum2D(self.x_data, self.y_data, self.z_data[item, :], x_tick_labels=self.x_tick_labels, metadata=new_metadata)
        if isinstance(item, slice):
            if (item.stop is not None) and (item.stop >= len(self)):
                raise IndexError(f'Index "{item.stop}" out of range')
            new_metadata.update(self._combine_metadata(line_metadata[item]))
        else:
            try:
                item = list(item)
                if not all([isinstance(i, Integral) for i in item]):
                    raise TypeError
            except TypeError:
                raise TypeError(f'Index "{item}" should be an integer, slice or sequence of ints')
            new_metadata.update(self._combine_metadata([line_metadata[i] for i in item]))
        return type(self)(self.x_data, self.y_data, self.z_data[item, :, :], x_tick_labels=self.x_tick_labels, metadata=new_metadata)

    @classmethod
    def from_spectra(cls, spectra: Sequence[Spectrum2D]) -> Self:
        if len(spectra) < 1:
            raise IndexError("At least one spectrum is needed for collection")

        def _type_check(spectrum):
            if not isinstance(spectrum, Spectrum2D):
                raise TypeError("from_spectra() requires a sequence of Spectrum2D")

        _type_check(spectra[0])
        x_data = spectra[0].x_data
        y_data = spectra[0].y_data
        x_tick_labels = spectra[0].x_tick_labels
        z_data_shape = spectra[0].z_data.shape
        z_data_magnitude = np.empty((len(spectra), *z_data_shape))
        z_data_magnitude[0, :, :] = spectra[0].z_data.magnitude
        z_data_units = spectra[0].z_data.units

        for i, spectrum in enumerate(spectra[1:]):
            _type_check(spectrum)
            assert spectrum.z_data.units == z_data_units
            assert np.allclose(spectrum.x_data, x_data)
            assert spectrum.x_data.units == x_data.units
            assert np.allclose(spectrum.y_data, y_data)
            assert spectrum.y_data.units == y_data.units
            assert spectrum.x_tick_labels == x_tick_labels

            z_data_magnitude[i + 1, :, :] = spectrum.z_data.magnitude

        metadata = cls._combine_metadata([spec.metadata for spec in spectra])
        z_data = Quantity(z_data_magnitude, z_data_units)
        return cls(x_data, y_data, z_data, x_tick_labels=x_tick_labels, metadata=metadata)

    def group_by(self, *line_data_keys: str) -> Self:
        """
        Group and sum z_data for each spectrum according to the values
        mapped to the specified keys in metadata['line_data']

        Parameters
        ----------
        line_data_keys
            The key(s) to group by. If only one line_data_key is
            supplied, if the value mapped to a key is the same for
            multiple spectra, they are placed in the same group and
            summed. If multiple line_data_keys are supplied, the values
            must be the same for all specified keys for them to be
            placed in the same group

        Returns
        -------
        grouped_spectrum
            A new Spectrum2DCollection with one 2-D map for each group. Any
            metadata in 'line_data' not common across all spectra in a
            group will be discarded
        """
        from euphonic.util import _get_unique_elems_and_idx

        # Remove any keys that are in top-level of metadata rather than in
        # line_data: this means that all rows have the same value, so it is
        # not useful for splitting!
        line_data_keys = [key for key in line_data_keys if key not in self.metadata]

        if not line_data_keys:
            # No remaining keys to split on: return collection with single sum
            return self.from_spectra([self.sum()])

        grouping_dict = _get_unique_elems_and_idx(self._get_line_data_vals(*line_data_keys))

        new_z_data = np.zeros((len(grouping_dict), *self._z_data.shape[1:]))
        group_metadata = deepcopy(self.metadata)
        group_metadata["line_data"] = [{} for _ in grouping_dict]
        for i, idxs in enumerate(grouping_dict.values()):
            # Look for any common key/values in grouped metadata
            group_i_metadata = self._combine_line_metadata(idxs)
            group_metadata["line_data"][i] = group_i_metadata
            new_z_data[i] = np.sum(self._z_data[idxs], axis=0)
        new_z_data = new_z_data * ureg(self._internal_z_data_unit).to(self.z_data_unit)

        new_data = self.copy()
        new_data.z_data = new_z_data
        new_data.metadata = group_metadata

        return new_data

    def sum(self) -> Spectrum2D:
        """
        Sum z_data over all spectra

        Returns
        -------
        summed_spectrum
            A Spectrum2D created from the summed z_data. Any metadata
            in 'line_data' not common across all spectra will be
            discarded
        """
        metadata = deepcopy(self.metadata)
        metadata.pop("line_data", None)
        metadata.update(self._combine_line_metadata())
        summed_z_data = np.sum(self._z_data, axis=0) * ureg(self._internal_z_data_unit).to(self.z_data_unit)
        return Spectrum2D(
            np.copy(self.x_data),
            np.copy(self.y_data),
            summed_z_data,
            x_tick_labels=deepcopy(self.x_tick_labels),
            metadata=deepcopy(metadata),
        )

    # The following methods are unchanged from Spectrum1DCollection and could
    # be moved to a common parent class

    def select(self, **select_key_values: Union[str, int, Sequence[str], Sequence[int]]) -> Self:
        return Spectrum1DCollection.select(self, **select_key_values)

    @staticmethod
    def _combine_metadata(all_metadata: Sequence[Dict[str, Union[int, str]]]) -> Dict[str, Union[int, str, LineData]]:
        return Spectrum1DCollection._combine_metadata(all_metadata)

    def _combine_line_metadata(self, indices: Optional[Sequence[int]] = None) -> Dict[str, Any]:
        return Spectrum1DCollection._combine_line_metadata(self, indices=indices)

    def _get_line_data_vals(self, *line_data_keys: str) -> np.ndarray:
        return Spectrum1DCollection._get_line_data_vals(self, *line_data_keys)

    # The following methods are unchanged from Spectrum2D and could be moved
    # to a common parent class

    @property
    def z_data(self) -> Quantity:
        return self._z_data * ureg(self._internal_z_data_unit).to(self.z_data_unit)

    @z_data.setter
    def z_data(self, value: Quantity) -> None:
        self.z_data_unit = str(value.units)
        self._z_data = value.to(self._internal_z_data_unit).magnitude

    def __imul__(self, other: Real) -> Self:
        self.z_data = self.z_data * other
        return self

    def __setattr__(self, name: str, value: Any) -> None:
        _check_unit_conversion(self, name, value, ["z_data_unit"])
        Spectrum.__setattr__(self, name, value)

    def copy(self: Self) -> Self:
        return Spectrum2D.copy(self)

    def get_bin_edges(self, bin_ax: Literal["x", "y"] = "x") -> Quantity:
        return Spectrum2D.get_bin_edges(self, bin_ax=bin_ax)

    def get_bin_centres(self, bin_ax: Literal["x", "y"] = "x") -> Quantity:
        return Spectrum2D.get_bin_centres(self, bin_ax=bin_ax)

    def get_bin_widths(self, bin_ax: Literal["x", "y"] = "x") -> Quantity:
        return Spectrum2D.get_bin_widths(self, bin_ax=bin_ax)

    def assert_regular_bins(self, bin_ax: Literal["x", "y"], message: str = "", rtol: float = 1e-5, atol: float = 0.0) -> None:
        return Spectrum2D.assert_regular_bins(self, message=message, rtol=rtol, atol=atol)

    def to_dict(self) -> Dict[str, Any]:
        return Spectrum2D.to_dict(self)

    @classmethod
    def from_dict(cls, d: Dict[str, Any]) -> Self:
        return Spectrum2D.from_dict(d)
