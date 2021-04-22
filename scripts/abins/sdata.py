# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import collections.abc
from typing import Dict, List, Optional, overload, Sequence, TypeVar, Union
import numpy as np
from numbers import Real

from mantid.kernel import logger as mantid_logger
import abins
from abins.constants import ALL_KEYWORDS_ATOMS_S_DATA, ALL_SAMPLE_FORMS, ATOM_LABEL, FLOAT_TYPE, S_LABEL

# Type annotation for atom items e.g. data['atom_1']
OneAtomSData = Dict[str, np.ndarray]


class SData(collections.abc.Sequence):
    """
    Class for storing S(Q, omega) with relevant metadata

    Indexing will return dict(s) of S by quantum order for atom(s)
    corresponding to index/slice.
    """

    def __init__(self, *,
                 data: dict,
                 frequencies: np.ndarray,
                 temperature: Optional[float] = None,
                 sample_form: str = '',
                 ) -> None:
        super().__init__()

        if temperature is None:
            self._temperature = None
        elif isinstance(temperature, Real):
            self._temperature = float(temperature)
        else:
            raise TypeError("Temperature must be a real number or None")

        if isinstance(sample_form, str):
            self._sample_form = sample_form
        else:
            raise TypeError("Sample form must be a string. Use '' (default) if unspecified.")

        self._frequencies = np.asarray(frequencies, dtype=FLOAT_TYPE)
        self._check_frequencies()

        self._data = data
        self._check_data()

    def get_frequencies(self) -> np.ndarray:
        return self._frequencies.copy()

    def get_temperature(self) -> Union[float, None]:
        return self._temperature

    def get_sample_form(self) -> str:
        return self._sample_form

    def get_bin_width(self) -> Union[float, None]:
        """Check frequency series and return the bin size

        If the frequency series does not have a consistent step size, return None
        """

        self._check_frequencies()
        step_size = (self._frequencies[-1] - self._frequencies[0]) / (self._frequencies.size - 1)

        if np.allclose(step_size, self._frequencies[1:] - self._frequencies[:-1]):
            return step_size
        else:
            return None

    def get_total_intensity(self) -> np.ndarray:
        """Sum over all atoms and quantum orders to a single spectrum"""

        total = np.zeros_like(self._frequencies)
        for atom_data in self:
            for order_key, data in atom_data.items():
                total += data
        return total

    def check_finite_temperature(self):
        """Raise an error if Temperature is not greater than zero"""
        temperature = self.get_temperature()
        if not (isinstance(temperature, (float, int)) and temperature > 0):
            raise ValueError("Invalid value of temperature.")

    def check_known_sample_form(self):
        """Raise an error if sample form is not known to Abins"""
        sample_form = self.get_sample_form()
        if sample_form not in ALL_SAMPLE_FORMS:
            raise ValueError(
                f"Invalid sample form {sample_form}: known sample forms are {ALL_SAMPLE_FORMS}")

    def _check_frequencies(self):
        # Check frequencies are ordered low to high
        if not np.allclose(np.sort(self._frequencies),
                           self._frequencies):
            raise ValueError("Frequencies not sorted low to high")

    def _check_data(self):
        """Check data set is consistent and has correct types"""
        if not isinstance(self._data, dict):
            raise ValueError("New value of S  should have a form of a dict.")

        for key, item in self._data.items():
            if ATOM_LABEL in key:
                if not isinstance(item, dict):
                    raise ValueError("New value of item from S data should have a form of dictionary.")

                if sorted(item.keys()) != sorted(ALL_KEYWORDS_ATOMS_S_DATA):
                    raise ValueError("Invalid structure of the dictionary.")

                for order in item[S_LABEL]:
                    if not isinstance(item[S_LABEL][order], np.ndarray):
                        raise ValueError("Numpy array was expected.")

            elif item == "frequencies":
                raise Exception("The Abins SData format is changed, do not put frequencies in this dict")

            else:
                raise ValueError("Invalid keyword " + item)

    def extract(self):
        """
        Returns the data.
        :returns: data
        """
        # Use a shallow copy so that 'frequencies' is not added to self._data
        full_data = self._data.copy()
        full_data.update({'frequencies': self._frequencies})
        return full_data

    def check_thresholds(self, return_cases=False, logger=None):
        """
        Compare the S data values to minimum thresholds and warn if the threshold appears large relative to the data

        Warnings will be raised if [max(S) * s_relative_threshold] is less than s_absolute_threshold. These
        thresholds are defined in the abins.parameters.sampling dictionary.

        :param return_cases: If True, return a list of cases where S was small compared to threshold.
        :type return_cases: bool

        :returns: If return_cases=True, this method returns a list of cases which failed the test, as tuples of
            ``(atom_key, order_number, max(S))``. Otherwise, the method returns ``None``.

        """

        if logger is None:
            logger = mantid_logger

        warning_cases = []
        absolute_threshold = abins.parameters.sampling['s_absolute_threshold']
        relative_threshold = abins.parameters.sampling['s_relative_threshold']
        for key, entry in self._data.items():
            if ATOM_LABEL in key:
                for order, s in entry['s'].items():
                    if max(s.flatten()) * relative_threshold < absolute_threshold:
                        warning_cases.append((key, order, max(s.flatten())))

        if len(warning_cases) > 0:
            logger.warning("Warning: some contributions had small S compared to threshold.")
            logger.warning("The minimum S threshold ({}) is greater than {}% of the "
                           "maximum S for the following:".format(absolute_threshold,
                                                                 relative_threshold * 100))

            # Sort the warnings by atom number, order number
            # Assuming that keys will be of form "atom_1", "atom_2", ...
            # and "order_1", "order_2", ...
            def int_key(case):
                key, order, _ = case
                return (int(key.split('_')[-1]), int(order.split('_')[-1]))

            for case in sorted(warning_cases, key=int_key):
                logger.warning("{0}, {1}: max S {2:10.4E}".format(*case))

        if return_cases:
            return warning_cases
        else:
            return None

    def __str__(self):
        return "Dynamical structure factors data"

    def __len__(self) -> int:
        return len(self._data)

    @overload  # noqa F811
    def __getitem__(self, item: int) -> OneAtomSData:
        ...

    @overload  # noqa F811
    def __getitem__(self, item: slice) -> List[OneAtomSData]: # noqa F811
        ...

    def __getitem__(self, item):  # noqa F811
        if isinstance(item, int):
            try:
                return self._data[f"atom_{item}"]['s']
            except KeyError:
                raise IndexError(item)
        elif isinstance(item, slice):
            return [self[i] for i in range(len(self))[item]]
        else:
            raise TypeError(
                "Indices must be integers or slices, not {}.".format(type(item)))


SDBA = TypeVar('SDBA', bound='SDataByAngle')


class SDataByAngle(collections.abc.Sequence):
    def __init__(self, *,
                 data: Dict[str, OneAtomSData],
                 angles: Sequence[float],
                 frequencies: np.ndarray,
                 temperature: Optional[float] = None,
                 sample_form: str = '',
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
            for order, order_data in atom_data['s'].items():
                if order_data.shape != (n_angles, n_frequencies):
                    raise IndexError("SDataByAngle input should have 2D array "
                                     "in (angles, frequencies)")

        self.angles = list(angles)
        self._data = data
        self._metadata = {'frequencies': frequencies,
                          'temperature': temperature,
                          'sample_form': sample_form}

        self.frequencies = self._metadata['frequencies']
        self.temperature = self._metadata['temperature']
        self.sample_form = self._metadata['sample_form']

    def __len__(self) -> int:
        return len(self.angles)

    @overload  # noqa F811
    def __getitem__(self, item: int) -> SData:
        ...

    @overload  # noqa F811
    def __getitem__(self: SDBA, item: slice) -> SDBA: # noqa F811
        ...

    def __getitem__(self, item):  # noqa F811
        if isinstance(item, (int, slice)):
            data = {atom_index: {'s': {order_index:
                                       self._data[atom_index]['s'][order_index][item, :]
                                       for order_index in self._data[atom_index]['s']}}
                    for atom_index in self._data}
        else:
            raise TypeError(
                "Indices must be integers or slices, not {}.".format(type(item)))

        if isinstance(item, int):
            return SData(data=data, **self._metadata)

        else:  # Must be a slice, return angle-resolved data
            return type(self)(data=data,
                              angles=self.angles[item],
                              **self._metadata)

    @classmethod
    def get_empty(cls: SDBA, *,
                  angles: Sequence[float],
                  frequencies: np.ndarray,
                  atom_keys: Sequence[str],
                  order_keys: Sequence[str],
                  **kwargs) -> SDBA:
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

        data = {atom_key: {'s': {order_key: np.zeros((n_angles, n_frequencies))
                                 for order_key in order_keys}}
                for atom_key in atom_keys}

        return cls(data=data, angles=angles, frequencies=frequencies, **kwargs)

    def set_angle_data(self, angle_index: int, sdata: SData,
                       add_to_existing: bool = False) -> None:
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

        if 'frequencies' in data:
            del data['frequencies']

        self.set_angle_data_from_dict(angle_index, data,
                                      add_to_existing=add_to_existing)

    def set_angle_data_from_dict(self, angle_index: int,
                                 data: Dict[str, OneAtomSData],
                                 add_to_existing: bool = False) -> None:

        for atom_key, atom_data in data.items():
            for order_key, order_data in atom_data['s'].items():
                if add_to_existing:
                    self._data[atom_key]['s'][order_key][angle_index, :] += order_data
                else:
                    self._data[atom_key]['s'][order_key][angle_index, :] = order_data

    @classmethod
    def from_sdata_series(cls: SDBA, data: Sequence[SData], *, angles: Sequence[float]) -> SDBA:
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
            if not isinstance(sdata, SData):
                raise TypeError("data must be a sequence of SData")

            for key in ('frequencies', 'temperature', 'sample_form'):
                if key not in metadata:
                    metadata[key] = getattr(sdata, f'get_{key}')()
                else:
                    if not near_enough(metadata[key],
                                       getattr(sdata, f'get_{key}')()):
                        raise ValueError(f"Property '{key}' must agree for all "
                                         "SData being collected.")

        atom_keys = list(data[0]._data.keys())
        sdata_collection = cls.get_empty(angles=angles,
                                         atom_keys=atom_keys,
                                         order_keys=list(data[0]._data[atom_keys[0]]['s'].keys()),
                                         **metadata)

        # Second loop over data: collect scattering data
        for angle_index, sdata in enumerate(data):
            sdata_collection.set_angle_data(angle_index, sdata)

        return sdata_collection

    def sum_over_angles(self,
                        average: bool = False,
                        weights: Sequence[float] = None) -> SData:
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
        order_keys = list(self._data[atom_keys[0]]['s'])

        if average:
            if weights is not None:
                raise ValueError("Cannot set weights while average=True")
            weights = np.full(n_angles, 1. / n_angles)
        elif weights:
            if len(weights) != n_angles:
                raise IndexError("Length of weights must match sampled angles")
        else:
            weights = np.ones(n_angles)

        assert isinstance(weights, (Sequence, np.ndarray))

        flattened_data = {atom_key: {'s': {order_key: np.zeros(n_frequencies)
                                           for order_key in order_keys}}
                          for atom_key in atom_keys}

        for angle_index, weight in enumerate(weights):
            for atom_key in atom_keys:
                for order_key in order_keys:
                    flattened_data[atom_key]['s'][order_key] += (
                        weight * self._data[atom_key]['s'][order_key][angle_index, :])

        return SData(data=flattened_data, **self._metadata)
