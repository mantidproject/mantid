# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from collections import namedtuple
from collections.abc import Mapping
import functools
import numpy as np
from mantid.geometry import OrientedLattice
import mantid.kernel as mk
from enum import Enum
import warnings

warnings.filterwarnings("ignore", category=RuntimeWarning)


__all__ = ["qangle", "ErrorCodes"]


def namedtuplefy(func):
    r"""
    Decorator to transform the return dictionary of a function into
    a namedtuple
    Parameters
    ----------
    func: Function
        Function to be decorated
    name: str
        Class name for the namedtuple. If None, the name of the function
        will be used
    Returns
    -------
    Function
    """

    @functools.wraps(func)
    def wrapper(*args, **kwargs):
        res = func(*args, **kwargs)
        if wrapper.nt is None:
            if isinstance(res, Mapping) is False:
                raise ValueError("Cannot namedtuplefy a non-dict")
            wrapper.nt = namedtuple(func.__name__ + "_nt", res.keys())
        return wrapper.nt(**res)

    wrapper.nt = None
    return wrapper


class ErrorCodes(Enum):
    """
    Error codes for qangle
        - 0 correct result
        - 1 input parameters are wrong for the corresponding hkl
        - 2 conservation of energy and momentum can't be satisfied
        - 3 out of plane angle too high
        - 4 scattering outside detector
        - 5 scattering in the beam stop
        - 6 goniometer out of range
    """

    CORRECT = 0
    WRONG_INPUT = 1
    QE_CONSERVATION = 2
    OUT_OF_PLANE = 3
    OUTSIDE_DETECTOR = 4
    INSIDE_BEAMSTOP = 5
    GONIOMETR = 6


def _qangle_validate_inputs(
    hkl: np.array,
    Ei: float or np.array,
    DeltaE: float or np.array,
    sign: float or np.array,
    lattice: OrientedLattice,
    detector_constraints: bool,
    horizontal_extent: np.array,
    vertical_extent: np.array,
    horizontal_extent_low: np.array,
    vertical_extent_low: np.array,
    goniometer_constraints: bool,
    goniometer_range,
):
    """
    Validate inputs for qangle function, according to the rules for
    that function
    """
    try:
        len_hkl = len(hkl)
        if len(hkl[0]) != 3:
            raise ValueError()
    except (TypeError, ValueError):
        raise ValueError("hkl is not an array of triplets")

    try:
        # check if float
        Ei = float(Ei)
        Ei = np.full(len_hkl, Ei)
    except ValueError:
        raise ValueError("Ei is not a float or numpy array")
    except TypeError:
        if len(Ei) != len_hkl:
            raise ValueError("Ei has different length than hkl")
    try:
        # check if float
        DeltaE = float(DeltaE)
        DeltaE = np.full(len_hkl, DeltaE)
    except ValueError:
        raise ValueError("DeltaE is not a float or numpy array")
    except TypeError:
        if len(DeltaE) != len_hkl:
            raise ValueError("DeltaE has different length than hkl")

    try:
        # check if int
        sign = int(sign)
        sign = np.full(len_hkl, sign)
    except ValueError:
        raise ValueError("sign is not an int or numpy array")
    except TypeError:
        if len(sign) != len_hkl:
            raise ValueError("sign has different length than hkl")

    try:
        UB = lattice.getUB() * 2.0 * np.pi
    except:
        raise ValueError("Can't get the UB matrix from the lattice object")

    # inputs for geometry and goniometer constraints
    if detector_constraints:
        if horizontal_extent[0] < -180 or horizontal_extent[1] < horizontal_extent[0] or horizontal_extent[1] > 180:
            raise ValueError(
                f"Horizontal constraints must obey -180 <= horizontal_extent[0]"
                f" ({horizontal_extent[0]}) <= horizontal_extent[1] ({horizontal_extent[1]})<=180"
            )
        if vertical_extent[0] < -180 or vertical_extent[1] < vertical_extent[0] or vertical_extent[1] > 180:
            raise ValueError(
                f"Vertical constraints must obey -180 <= vertical_extent[0] ({vertical_extent[0]}) "
                f"<= vertical_extent[1] ({vertical_extent[1]}) <=180"
            )
        if horizontal_extent_low[0] < -180 or horizontal_extent_low[1] < horizontal_extent_low[0] or horizontal_extent_low[1] > 180:
            raise ValueError(
                f"Horizontal constraints must obey -180 <= horizontal_extent_low[0]"
                f" ({horizontal_extent_low[0]}) <= horizontal_extent_low[1] ({horizontal_extent_low[1]}) "
                f"<=180"
            )
        if vertical_extent_low[0] < -180 or vertical_extent_low[1] < vertical_extent_low[0] or vertical_extent_low[1] > 180:
            raise ValueError(
                f"Vertical constraints must obey -180 <= vertical_extent_low[0] ({vertical_extent_low[0]}) "
                f"<= vertical_extent_low[1] ({vertical_extent_low[1]}) <=180"
            )

    if goniometer_constraints:
        if goniometer_range[1] < goniometer_range[0] or goniometer_range[0] < -180.0 or goniometer_range[1] > 180.0:
            raise ValueError("goniometer_range must be an increasing array, with both limits between -180 and 180 degrees")

    return (Ei, DeltaE, sign, UB)


@namedtuplefy
def qangle(
    *,  # force keyword arguments
    Ei: float or np.array,
    hkl: np.array,  # numpy array of triplets
    DeltaE: float or np.array,
    sign: int or np.array,
    lattice: OrientedLattice,
    detector_constraints: bool = False,
    horizontal_extent: np.array = np.array([-180.0, 180.0]),
    vertical_extent: np.array = np.array([-90.0, 90.0]),
    horizontal_extent_low: np.array = np.array([0.0, 0.0]),
    vertical_extent_low: np.array = np.array([0.0, 0.0]),
    goniometer_constraints: bool = False,
    goniometer_range: np.array = np.array([-180.0, 180.0]),
) -> namedtuple:
    """
    The function calculates the momentum transfer in the lab frame,
    scattered momentum, and vertical goniometer angle,
    based on HKL, DeltaE and Ei.
    Detailed documentation at https://code.ornl.gov/spectroscopy/dgs/status/-/issues/31

    Parameters:
    -----------
      Ei (float or numpy array): Incident energy in meV
      hkl (numpy array of triplets): Reciprocal lattice units for momentum transfer
      DeltaE (float or numpy array): Energy transfer in meV
      sign (int or numpy array): Scattering left of the beam (+1) or right (-1)
      lattice (OrientedLattice): Mantid OrientedLattice object (contains UB matrix)
      detector_constraints (bool): Flag to consider finite size of detector
      horizontal_extent (numpy array): Horizontal angle extent of the detector in degrees (-180 to 180)
      vertical_extent (numpy array): Vertical angle extent of the detector in degrees (-90 to 90)
      horizontal_extent_low (numpy array): Horizontal angle extent of the beam stop in degrees (-180 to 180)
      vertical_extent_low (numpy array): Vertical angle extent of the beam stop in degrees
      goniometer constraints (bool): Flag to restrict the range of the goniometer
      goniometer_range (numpy array): Goniometer angle range in degrees (in the -180 to 180 range)

    Retuns:
    -------
      (namedtuple): namedtuple containing
        - Q_lab_x, Q_lab_y, Q_lab_z (numpy arrays): Q in the lab frame
        - in_plane_Q_angle, out_plane_Q_angle (numpy arrays): in plane and out of plane angles for Q_lab
        - in_plane_kf_angle, out_plane_kf_angle (numpy arrays): in plane and out of plane angles for scattered beam
        - goniometer (numpy array): goniometer angle in degrees (-180 to 180)
        - error code (numpy array): error codes

    Raises:
    -------
      ValueError if input parameters are not as described above

    Notes:
    -----
    1. Where the error code is not 0, the corresponding entries in the other vector might be set to NaN
    2. If detector_constraints is False, entries for horizontal_extent, vertical_extent,
    horizontal_extent_low, vertical_extent_low are ignored
    3. If goniometer_constraints is False, goniometer_range is ignored
    """
    # Note for developers: formulas from
    # https://github.com/mantidproject/documents/blob/master/Design/UBMatriximplementationnotes.pdf

    # check input parameters
    Ei, DeltaE, sign, UB = _qangle_validate_inputs(
        hkl,
        Ei,
        DeltaE,
        sign,
        lattice,
        detector_constraints,
        horizontal_extent,
        vertical_extent,
        horizontal_extent_low,
        vertical_extent_low,
        goniometer_constraints,
        goniometer_range,
    )

    error_code = np.full((len(hkl)), ErrorCodes.CORRECT)

    # conversion factor from momentum (inverse Angstroms) to energy (meV) - E = E2k * k**2
    E2k = mk.PhysicalConstants.E_mev_toNeutronWavenumberSq

    # Q_sample =2Pi(hkl) (the multiplication with 2pi was done in validation) (eq 5-7)
    Q_sample_x, Q_sample_y, Q_sample_z = np.matmul(UB, hkl.T)
    k_i = np.sqrt(Ei / E2k)
    k_f = np.sqrt((Ei - DeltaE) / E2k)

    # if any of the above elements in the array is NaN or inf, you get error_code 1 (or 2 if only kf is wrong)
    error_code[np.logical_not(np.isfinite(k_f))] = ErrorCodes.QE_CONSERVATION
    error_code[np.logical_not(np.isfinite(Q_sample_x + Q_sample_y + Q_sample_z + k_i))] = ErrorCodes.WRONG_INPUT

    # calculate delta (eq 121)
    error_code[np.abs(Q_sample_y) > 0.9998 * k_f] = ErrorCodes.OUT_OF_PLANE
    delta = -np.arcsin(Q_sample_y / k_f)

    # calculate chi (eq 125)
    chi = np.arccos((k_i**2 + k_f**2 - Q_sample_x**2 - Q_sample_y**2 - Q_sample_z**2) / (2 * k_i * k_f * np.cos(delta)))
    bad_chi = np.logical_not(np.isfinite(chi))
    error_code[np.logical_and(bad_chi, error_code == ErrorCodes.CORRECT)] = ErrorCodes.QE_CONSERVATION

    # check left/right
    bad_sign = np.abs(sign) != 1
    chi *= sign
    chi[bad_sign] = np.nan
    delta[bad_sign] = np.nan
    error_code[bad_sign] = ErrorCodes.WRONG_INPUT

    # calculate Q_lab (eq 118)
    Q_lab_x = -k_f * np.cos(delta) * np.sin(chi)
    Q_lab_y = Q_sample_y
    Q_lab_z = k_i - k_f * np.cos(delta) * np.cos(chi)

    # calculate angles Q_lab
    in_plane_Q_angle = np.arctan2(Q_lab_x, Q_lab_z)
    out_plane_Q_angle = np.arcsin(Q_lab_y / np.sqrt(Q_lab_x**2 + Q_lab_y**2 + Q_lab_z**2))

    # calculate omega (eq 126)
    omega = np.arctan2(Q_lab_x, Q_lab_z) - np.arctan2(Q_sample_x, Q_sample_z)
    omega[omega > np.pi] -= 2 * np.pi
    omega[omega < -np.pi] += 2 * np.pi

    # transform all angles to degrees
    chi = np.degrees(chi)
    delta = np.degrees(delta)
    omega = np.degrees(omega)
    in_plane_Q_angle = np.degrees(in_plane_Q_angle)
    out_plane_Q_angle = np.degrees(out_plane_Q_angle)

    if detector_constraints:
        error_code[
            ((chi < horizontal_extent[0]) | (chi > horizontal_extent[1]) | (delta < vertical_extent[0]) | (delta > vertical_extent[1]))
            & (error_code == ErrorCodes.CORRECT)
        ] = ErrorCodes.OUTSIDE_DETECTOR
        error_code[
            (chi > horizontal_extent_low[0])
            & (chi < horizontal_extent_low[1])
            & (delta > vertical_extent_low[0])
            & (delta < vertical_extent_low[1])
            & (error_code == ErrorCodes.CORRECT)
        ] = ErrorCodes.INSIDE_BEAMSTOP

    if goniometer_constraints:
        error_code[((omega < goniometer_range[0]) | (omega > goniometer_range[1])) & (error_code == ErrorCodes.CORRECT)] = (
            ErrorCodes.GONIOMETR
        )

    return dict(
        Q_lab_x=Q_lab_x,
        Q_lab_y=Q_lab_y,
        Q_lab_z=Q_lab_z,
        in_plane_Q_angle=in_plane_Q_angle,
        out_plane_Q_angle=out_plane_Q_angle,
        in_plane_kf_angle=chi,
        out_plane_kf_angle=delta,
        omega=omega,
        error_code=error_code,
    )
