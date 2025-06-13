# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from functools import partial
from itertools import chain, groupby
from multiprocessing import Pool
from operator import itemgetter
from typing import (
    Dict,
    Generator,
    Iterable,
    List,
    Optional,
    Sequence,
    Tuple,
    Union,
)

from euphonic import Quantity, ureg
from euphonic.spectra import Spectrum1DCollection, Spectrum2DCollection
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

    :param items: Components of S data to check with form (atom_index, quantum_order_index, s_array)
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


def apply_kinematic_constraints(spectra: Spectrum2DCollection, instrument: DirectInstrument) -> None:
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

    spectra.z_data[:, mask] = float("nan") * ureg(spectra.z_data_unit)


def add_autoconvolution_spectra(
    spectra: Spectrum1DCollection, max_order: Optional[int] = None, output_bins: Optional[Quantity] = None
) -> Spectrum1DCollection:
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

    assert isinstance(spectra, Spectrum1DCollection)

    # group spectra by atom (using sort and itertools groupby)
    spectra_by_atom = groupby(
        sorted(spectra, key=(lambda spec: (spec.metadata["atom_index"], spec.metadata["quantum_order"]))),
        key=(lambda spec: spec.metadata["atom_index"]),
    )

    # Iterator tweaking: drop the keys and convert each group to Spectrum1DCollection
    spectra_by_atom = (Spectrum1DCollection.from_spectra(list(sba_item[1]), unsafe=True) for sba_item in spectra_by_atom)

    # create new spectra using first and last spectra in initial group, iterating until order = max_order
    x_data = spectra.x_data

    with Pool(N_THREADS) as p:
        output_spectra = p.map(partial(_autoconvolve_atom_spectra, x_data=x_data, max_order=max_order), spectra_by_atom)

        if output_bins is not None:
            output_spectra = p.map(partial(_resample_spectra, input_bins=x_data, output_bins=output_bins), output_spectra)

    return Spectrum1DCollection.from_spectra(list(chain(*output_spectra)), unsafe=True)


def _autoconvolve_atom_spectra(atom_spectra: Spectrum1DCollection, x_data: Quantity, max_order: int) -> Spectrum1DCollection:
    """
    Autoconvolve pre-sorted list of spectra; convolve first element with last until order reaches max_order

    Args:
        atom_spectra: a list of spectra belonging to the same atom, pre-sorted by quantum_order

        x_data: Corresponding x-values to atom_spectra. This is required as an
            optimisation to prevent too many calls to spectrum.x_data.

        max_order: Highest quantum order for autoconvolution
    """

    first_spectrum = atom_spectra[0]
    assert first_spectrum.metadata["quantum_order"] == 1
    fundamentals = first_spectrum.y_data.magnitude
    y_units = first_spectrum.y_data_unit

    highest_initial_order = atom_spectra[-1].metadata["quantum_order"]
    if max_order < highest_initial_order:
        raise ValueError("Max order should not be lower than highest quantum order before autoconvolution.")

    new_y_data = list(atom_spectra.y_data.magnitude)
    new_line_data = list(atom_spectra.iter_metadata())
    for order in range(highest_initial_order, max_order):
        autoconv_spectrum = convolve(new_y_data[-1], fundamentals, mode="full")[: fundamentals.size]

        new_y_data.append(autoconv_spectrum)
        new_line_data.append({"quantum_order": order + 1})

    y_data = ureg.Quantity(np.asarray(new_y_data), units=y_units)
    metadata = atom_spectra[0].metadata | {"line_data": new_line_data}

    return Spectrum1DCollection(x_data=x_data, y_data=y_data, metadata=metadata)


def _resample_spectra(spectra: Spectrum1DCollection, input_bins: Quantity, output_bins: Quantity) -> Spectrum1DCollection:
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
