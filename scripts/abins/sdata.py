# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import collections.abc
from copy import deepcopy
import re
from typing import Any, Dict, List, Optional, overload, Sequence, TypeVar, Union
from typing_extensions import Self

from pydantic import BaseModel, ConfigDict, PositiveFloat, validate_call
import numpy as np
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

    def check_thresholds(self, return_cases: bool = False, logger: Optional[Logger] = None, logging_level: str = "warning"):
        """
        Compare the S data values to minimum thresholds and warn if the threshold appears large relative to the data

        Warnings will be raised if [max(S) * s_relative_threshold] is less than s_absolute_threshold. These
        thresholds are defined in the abins.parameters.sampling dictionary.

        :param return_cases: If True, return a list of cases where S was small compared to threshold.
        :type return_cases: bool
        :param logger: Alternative logging object. (Defaults to Mantid logger)
        :param logging_level: logging level of warnings that a significant
            portion of S is being removed. Usually this will be 'information' or 'warning'.

        :returns: If return_cases=True, this method returns a list of cases which failed the test, as tuples of
            ``(atom_key, order_number, max(S))``. Otherwise, the method returns ``None``.

        """

        logger = get_logger(logger=logger)
        logger_call = getattr(logger, logging_level)

        warning_cases = []

        absolute_threshold = abins.parameters.sampling["s_absolute_threshold"]
        relative_threshold = abins.parameters.sampling["s_relative_threshold"]
        for key, entry in self._data.items():
            if ATOM_LABEL in key:
                for order, s in entry["s"].items():
                    if max(s.flatten()) * relative_threshold < absolute_threshold:
                        warning_cases.append((key, order, max(s.flatten())))

        if len(warning_cases) > 0:
            logger_call("Warning: some contributions had small S compared to threshold.")
            logger_call(
                "The minimum S threshold ({}) is greater than {}% of the " "maximum S for the following:".format(
                    absolute_threshold, relative_threshold * 100
                )
            )

            # Sort the warnings by atom number, order number
            # Assuming that keys will be of form "atom_1", "atom_2", ...
            # and "order_1", "order_2", ...
            def int_key(case):
                key, order, _ = case
                return (int(key.split("_")[-1]), int(order.split("_")[-1]))

            for case in sorted(warning_cases, key=int_key):
                logger_call("{0}, {1}: max S {2:10.4E}".format(*case))

        if return_cases:
            return warning_cases
        else:
            return None

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
                    raise IndexError("SDataByAngle input should have 2D array " "in (angles, frequencies)")

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
                        raise ValueError(f"Property '{key}' must agree for all " "SData being collected.")

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
