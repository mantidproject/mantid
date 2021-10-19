# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

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


def get_data_with_euphonic(filename: str, cutoff: float = 20.):
    """
    Read force constants file with Euphonic and sample frequencies/modes

    :param filename: Input data
    :param cutoff:
        Sampling density of Brillouin-zone. Specified as real-space length
        cutoff.

    :returns: 2-tuple (dict of structure and vibration data,
                       dict of elements and isotopes)

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
    logger.notice('Calculating phonon modes on {} grid'.format(
        'x'.join(map(str, mp_sampling))))
    modes = fc.calculate_qpoint_phonon_modes(qpts, use_c=True, n_threads=4)

    file_data = {'num_ions': len(fc.crystal.atom_type),
                 'num_branches': modes.frequencies.magnitude.shape[1],
                 'unit_cell': fc.crystal.cell_vectors.to('angstrom').magnitude}

    file_data['ir_intensities'] = np.array([])
    file_data['raman_intensities'] = np.array([])
    file_data['frequencies'] = modes.frequencies.to('1/cm').magnitude.flatten()
    eigenvectors = modes.eigenvectors.real
    # Eigenvectors should be indexed (qpts, mode/ion combination, dim)
    # (to match order in CASTEP .phonon files)
    # so reshape data into this format.
    file_data['eigenvectors'] = eigenvectors.reshape(
            (len(qpts), file_data['num_branches'] * file_data['num_ions'], 3))
    file_data['weights'] = (modes.weights * np.ones((file_data['num_branches'], 1))).flatten()

    ions = [{'index': i,
             'species': element,
             'fract_coord': position,
             'isotope_number': mass}
            for i, (element, position, mass)
            in enumerate(zip(fc.crystal.atom_type,
                             fc.crystal.atom_r,
                             fc.crystal.atom_mass.magnitude))]

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
