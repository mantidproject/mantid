# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import collections.abc
from copy import deepcopy
from functools import partial
from itertools import chain, groupby, repeat
from multiprocessing import Pool
from numbers import Integral, Real
from operator import itemgetter
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
    Union,
)
from typing_extensions import Self

from euphonic import Quantity, ureg
from euphonic.spectra import Spectrum, Spectrum1D, Spectrum1DCollection, Spectrum2D
from euphonic.validate import _check_constructor_inputs, _check_unit_conversion
import numpy as np
from scipy.signal import convolve

import abins
from abins.constants import FLOAT_TYPE
from abins.instruments.directinstrument import DirectInstrument
from abins.logging import get_logger, Logger
import abins.parameters

# Avoid inconsistent unit registries when (un)pickling for multiprocessing
import pint

pint.set_application_registry(ureg)


# Parallel threads for autoconvolution
N_THREADS = abins.parameters.performance.get("threads")


def _iter_check_thresholds(
    items: Iterable[Tuple[int, int, np.ndarray]]
) -> Generator[Tuple[int, int, float], None, None]:

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


XTickLabels = Sequence[Tuple[int, str]]
OneLineData = Dict[str, Union[str, int]]
LineData = Sequence[OneLineData]
Metadata = Dict[str, Union[str, int, LineData]]


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

    def select(self, **select_key_values: Union[str, int, Sequence[str], Sequence[int]]) -> Self:
        """
        Modify select() to allow keys that are common to all items

        metadata dict is of form {"key1": "value1", "key2": "value2",
                                   line_data: [{"key3": "value3"}, ...]}
        and select() is a query over all rows corresponding to line_data.

        We should treat top level-keys as equivalent to a key in all line_data
        rows, but Spectrum1DCollection select() method currently only looks at
        line_data. We remove common keys that are found, and ValueError if they
        don't match the data.

        """
        select_items = {}

        for key, value in select_key_values.items():
            if key in self.metadata:
                # Top-level metadata: either all rows pass or none do
                if isinstance(value, Sequence) and self.metadata[key] in value:
                    pass
                elif self.metadata[key] == value:
                    pass
                else:
                    raise ValueError(
                        f"No data matched the select() criterion {key}={value}, " f"as rows have common metadata {key}={self.metadata[key]}"
                    )
            else:
                select_items[key] = value

        return Spectrum1DCollection.select(self, **select_items)

    def iter_metadata(self) -> Generator[OneLineData, None, None]:
        common_metadata = dict((key, self.metadata[key]) for key in self.metadata.keys() - set("line_data"))

        line_data = self.metadata.get("line_data")
        if line_data is None:
            line_data = repeat({}, len(self._y_data))

        for one_line_data in line_data:
            yield common_metadata | one_line_data

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
        for line in line_data:
            all_metadata = line | self.metadata
            line_data_vals.append(tuple([all_metadata[key] for key in line_data_keys]))
        return line_data_vals

    def __add__(self, other: Self) -> Self:
        """
        Appends the y_data of 2 Spectrum1DCollection objects,
        creating a single Spectrum1DCollection that contains
        the spectra from both objects. The two objects must
        have equal x_data axes, and their y_data must
        have compatible units and the same number of y_data
        entries

        Any metadata key/value pairs that are common to both
        spectra are retained in the top level dictionary, any
        others are put in the individual 'line_data' entries
        """
        assert np.allclose(self.x_data.magnitude, other.x_data.magnitude)
        assert self.x_data_unit == other.x_data_unit

        return type(self)(
            x_data=self.x_data,
            y_data=np.concatenate((self.y_data, other.y_data)),
            metadata=self._concatenate_metadata(self.metadata, other.metadata),
        )

    @staticmethod
    def _concatenate_metadata(a: Metadata, b: Metadata) -> Metadata:
        """
        Common top-level key-value pairs are retained at top level, while
        differing top-level key-value pairs are added to line_data
        """
        common_items = {key: value for (key, value) in a.items() if key != "line_data" and b.get(key) == value}
        a_only_items = {key: a[key] for key in a if key != "line_data" and key not in common_items}
        b_only_items = {key: b[key] for key in b if key != "line_data" and key not in common_items}

        line_data = [entry | a_only_items for entry in a["line_data"]] + [entry | b_only_items for entry in b["line_data"]]

        return common_items | {"line_data": line_data}


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
        assert np.allclose(self.x_data.magnitude, other.x_data.magnitude)
        assert np.allclose(self.y_data.magnitude, other.y_data.magnitude)
        assert self.x_data_unit == other.x_data_unit
        assert self.y_data_unit == other.y_data_unit

        return type(self)(
            x_data=self.x_data,
            y_data=self.y_data,
            z_data=np.concatenate((self.z_data, other.z_data)),
            metadata=AbinsSpectrum1DCollection._concatenate_metadata(self.metadata, other.metadata),
        )

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
            return Spectrum2D(
                self.x_data,
                self.y_data,
                self.z_data[item, :],
                x_tick_labels=self.x_tick_labels,
                metadata=new_metadata,
            )

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
        return type(self)(
            self.x_data,
            self.y_data,
            self.z_data[item, :, :],
            x_tick_labels=self.x_tick_labels,
            metadata=new_metadata,
        )

    def iter_metadata(self) -> Generator[OneLineData, None, None]:
        common_metadata = dict((key, self.metadata[key]) for key in self.metadata.keys() - set("line_data"))

        line_data = self.metadata.get("line_data")
        if line_data is None:
            line_data = repeat({}, len(self._z_data))

        for one_line_data in line_data:
            yield common_metadata | one_line_data

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
        z_data = ureg.Quantity(z_data_magnitude, z_data_units)
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
        new_z_data = Quantity(new_z_data, units=self._internal_z_data_unit).to(self.z_data_unit)

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
        summed_z_data = Quantity(np.sum(self._z_data, axis=0), units=self._internal_z_data_unit).to(self.z_data_unit)
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
        return AbinsSpectrum1DCollection.select(self, **select_key_values)

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
        return Quantity(self._z_data, units=self._internal_z_data_unit).to(self.z_data_unit)

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

    def assert_regular_bins(
        self,
        bin_ax: Literal["x", "y"],
        message: str = "",
        rtol: float = 1e-5,
        atol: float = 0.0,
    ) -> None:

        return Spectrum2D.assert_regular_bins(self, message=message, rtol=rtol, atol=atol)

    def to_dict(self) -> Dict[str, Any]:
        return Spectrum2D.to_dict(self)

    @classmethod
    def from_dict(cls, d: Dict[str, Any]) -> Self:
        return Spectrum2D.from_dict(d)

    # These methods are _slightly_ modified from Spectrum2D and could be more
    # efficient at the cost of increased verbosity
    def get_bin_edges(self, bin_ax: Literal["x", "y"] = "x") -> Quantity:
        return Spectrum2D.get_bin_edges(self[0], bin_ax=bin_ax)

    def get_bin_centres(self, bin_ax: Literal["x", "y"] = "x") -> Quantity:
        return Spectrum2D.get_bin_centres(self[0], bin_ax=bin_ax)

    def get_bin_widths(self, bin_ax: Literal["x", "y"] = "x") -> Quantity:
        return Spectrum2D.get_bin_widths(self[0], bin_ax=bin_ax)


def apply_kinematic_constraints(spectra: AbinsSpectrum2DCollection, instrument: DirectInstrument) -> None:
    """Replace inaccessible intensity bins with NaN

    This passes frequencies to instrument.get_abs_q_limits() method to get
    accessible q-range at each energy, and masks out other bins with NaN.
    Data must be 2-D and q_bins must be available.

    Values will be modified in-place.

    Args:
        instrument - this must have the get_abs_q_limits() method
            (i.e. inherit DirectInstrument).
    """
    q_lower, q_upper = instrument.get_abs_q_limits(spectra.y_data.to("1/cm").magnitude)
    q_values = spectra.get_bin_centres(bin_ax="x").to("1/angstrom").magnitude
    mask = np.logical_or(
        q_values[:, np.newaxis] < q_lower[np.newaxis, :],
        q_values[:, np.newaxis] > q_upper[np.newaxis, :],
    )

    # Applying NaN to spectra.z_data doesn't seem to work? Use private attribute
    spectra._z_data[:, mask] = float("nan")


def add_autoconvolution_spectra(
    spectra: AbinsSpectrum1DCollection, max_order: Optional[int] = None, output_bins: Optional[Quantity] = None
) -> AbinsSpectrum1DCollection:
    """
    Atom-by-atom, add higher order spectra by convolution with fundamentals

    Strictly this is only autoconvolution when forming order-2 from order-1;
    higher orders are formed by repeated convolution with the fundamentals.

    Data should not have been broadened before applying this operation,
    or this will lead to repeated broadening of higher orders.

    The process will begin with the highest existing order, and repeat until
    a spectrum of MAX_ORDER is obtained.

    This operation is parallelised over atoms: the number of tasks is controlled
    by abins.parameters.performance["threads"]

    Results are rebinned into ``output_bins`` if this is provided

    Arguments:
        spectra: spectra with "atom_index" and "quantum_order" metadata to
            which higher orders will be added by convolution with
            quantum_order=1 data
        max_order: highest required quantum order
        output_bins: if provided, output spectra are re-binned to these x-values.

    """

    if max_order is None:
        max_order = abins.parameters.autoconvolution["max_order"]

    assert isinstance(spectra, AbinsSpectrum1DCollection)

    # group spectra by atom (using sort and itertools groupby)
    spectra_by_atom = groupby(
        sorted(spectra, key=(lambda spec: (spec.metadata["atom_index"], spec.metadata["quantum_order"]))),
        key=(lambda spec: spec.metadata["atom_index"]),
    )
    # Iterator tweaking: drop the keys and convert each group to list
    spectra_by_atom = (list(sba_item[1]) for sba_item in spectra_by_atom)

    # create new spectra using first and last spectra in initial group, iterating until order = max_order
    x_data = spectra.x_data

    with Pool(N_THREADS) as p:
        output_spectra = p.map(partial(_autoconvolve_atom_spectra, x_data=x_data, max_order=max_order), map(list, spectra_by_atom))

        if output_bins is not None:
            output_spectra = p.map(partial(_resample_spectra, input_bins=x_data, output_bins=output_bins), output_spectra)

    return _fast_1d_from_spectra(chain(*output_spectra))


def _fast_1d_from_spectra(spectra: Iterable[Spectrum1D]) -> AbinsSpectrum1DCollection:
    """Quickly produce a Spectrum1DCollection, trusting that data is consistent"""
    first_spectrum = next(spectra)
    x_data = first_spectrum.x_data
    y_data_list = [first_spectrum._y_data]
    metadata_list = [first_spectrum.metadata]

    for spectrum in spectra:
        y_data_list.append(spectrum._y_data)
        metadata_list.append(spectrum.metadata)

    metadata = AbinsSpectrum1DCollection._combine_metadata(metadata_list)

    return AbinsSpectrum1DCollection(
        x_data=x_data, y_data=ureg.Quantity(np.asarray(y_data_list), units=first_spectrum._internal_y_data_unit), metadata=metadata
    )


def _autoconvolve_atom_spectra(atom_spectra: List[Spectrum1D], x_data: Quantity, max_order: int) -> AbinsSpectrum1DCollection:
    """
    Autoconvolve pre-sorted list of spectra; convolve first element with last until order reaches max_order

    Args:
        atom_spectra: a list of spectra belonging to the same atom, pre-sorted by quantum_order

        x_data: Corresponding x-values to atom_spectra. This is required as an
            optimisation to prevent too many calls to spectrum.x_data.

        max_order: Highest quantum order for autoconvolution
    """

    assert atom_spectra[0].metadata["quantum_order"] == 1
    fundamentals = atom_spectra[0]._y_data
    y_units = atom_spectra[0]._internal_y_data_unit

    highest_initial_order = atom_spectra[-1].metadata["quantum_order"]

    new_y_data = [spectrum._y_data for spectrum in atom_spectra]
    new_line_data = [spectrum.metadata for spectrum in atom_spectra]
    for order in range(highest_initial_order, max_order):
        autoconv_spectrum = convolve(new_y_data[-1], fundamentals, mode="full")[: fundamentals.size]

        new_y_data.append(autoconv_spectrum)
        new_line_data.append({"quantum_order": order + 1})

    y_data = ureg.Quantity(np.asarray(new_y_data), units=y_units)
    metadata = atom_spectra[0].metadata | {"line_data": new_line_data}

    return AbinsSpectrum1DCollection(x_data=x_data, y_data=y_data, metadata=metadata)


def _resample_spectra(spectra: AbinsSpectrum1DCollection, input_bins: Quantity, output_bins: Quantity) -> AbinsSpectrum1DCollection:
    """
    Naive resampling by rebuilding histogram on new bins

    The intensity is not corrected for change in bin size: that happens elsewhere for now

    Approaches not used here:
    - for good performance when downsampling by an integer factor, consider reshaping array and summing over new axis
    - for more accurate final shape, apply some antialiasing filtering (at simplest level, linear binning)
    """
    assert input_bins.units == output_bins.units

    output_ydata = [
        np.histogram(input_bins.magnitude, bins=output_bins.magnitude, weights=weights, density=0)[0] for weights in spectra._y_data
    ]

    return Spectrum1DCollection(
        x_data=output_bins,
        y_data=ureg.Quantity(
            np.asarray(output_ydata, dtype=FLOAT_TYPE), units=(ureg(f"{spectra.y_data_unit} {spectra.x_data_unit}") / output_bins.units)
        ),
        metadata=spectra.metadata,
    )
