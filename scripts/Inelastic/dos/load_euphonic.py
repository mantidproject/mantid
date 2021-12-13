# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from typing import Optional

import numpy as np

from mantid.kernel import logger


def euphonic_available():
    """Find out if Euphonic modules can be imported, without raising an error

    This allows the Simulation Interface to query the availability of these
    formats without complex error-handling.
    """
    try:
        from euphonic.cli.utils import force_constants_from_file  # noqa: F401
        from euphonic.util import mp_grid  # noqa: F401
    except ImportError:
        return False
    return True


def euphonic_calculate_modes(filename: str, cutoff: float = 20.,
                             gamma: bool = True,
                             acoustic_sum_rule: Optional[str] = 'reciprocal'):
    """
    Read force constants file with Euphonic and sample frequencies/modes

    :param filename: Input data
    :param cutoff:
        Sampling density of Brillouin-zone. Specified as real-space length
        cutoff in Angstrom.
    :param gamma:
        Shift sampling grid to include the Gamma-point.
    :param acoustic_sum_rule:
        Apply acoustic sum rule correction to force constants: options are
        'realspace' and 'reciprocal', specifying different implementations of
        the correction. If None, no correction is applied. This option is
        referred to as "asr" in the Euphonic python API and command-line tools.

    :returns: euphonic.QpointPhononModes

    """

    from math import ceil
    from euphonic.cli.utils import force_constants_from_file
    from euphonic.util import mp_grid

    fc = force_constants_from_file(filename)
    recip_lattice_lengths = np.linalg.norm(
        fc.crystal.reciprocal_cell().to('1/angstrom').magnitude, axis=1)
    mp_sampling = [ceil(x)
                   for x in (cutoff * recip_lattice_lengths / (2 * np.pi))]
    qpts = mp_grid(mp_sampling)

    if gamma:
        mp_sampling = np.array(mp_sampling, dtype=int)
        # Shift directions with even number of samples by half the grid spacing
        offsets = ((mp_sampling + 1) % 2) * (0.5 / mp_sampling)
        qpts += offsets

    logger.notice('Calculating phonon modes on {} grid'.format(
        'x'.join(map(str, mp_sampling))))
    modes = fc.calculate_qpoint_phonon_modes(qpts, asr=acoustic_sum_rule)

    return modes


def get_data_with_euphonic(filename: str,
                           cutoff: float = 20.,
                           gamma: bool = True,
                           acoustic_sum_rule: Optional[str] = None):
    """
    Read force constants file with Euphonic and sample frequencies/modes

    :param filename: Input data
    :param cutoff:
        Sampling density of Brillouin-zone. Specified as real-space length
        cutoff.
    :param gamma:
        Shift sampling grid to include the Gamma-point.
    :param acoustic_sum_rule:
        Apply acoustic sum rule correction to force constants: options are
        'realspace' and 'reciprocal', specifying different implementations of
        the correction. If None, no correction is applied.

    :returns: 2-tuple (dict of structure and vibration data,
                       dict of elements and isotopes)

    """
    modes = euphonic_calculate_modes(filename=filename,
                                     cutoff=cutoff,
                                     gamma=gamma,
                                     acoustic_sum_rule=acoustic_sum_rule)

    file_data = {'num_ions': len(modes.crystal.atom_type),
                 'num_branches': modes.frequencies.magnitude.shape[1],
                 'unit_cell': modes.crystal.cell_vectors.to('angstrom').magnitude}

    file_data['ir_intensities'] = np.array([])
    file_data['raman_intensities'] = np.array([])
    file_data['frequencies'] = modes.frequencies.to('1/cm').magnitude.flatten()
    eigenvectors = modes.eigenvectors.real
    # Eigenvectors should be indexed (qpts, mode/ion combination, dim)
    # (to match order in CASTEP .phonon files)
    # so reshape data into this format.
    file_data['eigenvectors'] = eigenvectors.reshape(
            (len(modes.qpts), file_data['num_branches'] * file_data['num_ions'], 3))
    file_data['weights'] = (modes.weights * np.ones((file_data['num_branches'], 1))).flatten()

    ions = [{'index': i,
             'species': element,
             'fract_coord': position,
             'isotope_number': mass}
            for i, (element, position, mass)
            in enumerate(zip(modes.crystal.atom_type,
                             modes.crystal.atom_r,
                             modes.crystal.atom_mass.magnitude))]

    file_data['ions'] = ions

    # Store some redundant information as a side-effect for compatibility
    # with the existing code

    element_isotopes = {ion['species']: ion['isotope_number']
                        for ion in ions}

    # Generate index per element
    ion_counts = {element: 0 for element in element_isotopes}
    for ion in ions:
        species = ion['species']
        ion_counts[species] += 1
        ion['bond_number'] = ion_counts[species]

    return file_data, element_isotopes
