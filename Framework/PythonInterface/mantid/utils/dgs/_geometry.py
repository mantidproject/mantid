from collections import namedtuple
from collections.abc import Mapping
import functools
from typing import Any, Tuple
import numpy as np
#from mantid.kernel import Logger
from mantid.geometry import OrientedLattice 

__all__ = ['qangle']


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
                raise ValueError('Cannot namedtuplefy a non-dict')
            wrapper.nt = namedtuple(func.__name__ + '_nt', res.keys())
        return wrapper.nt(**res)
    wrapper.nt = None
    return wrapper


def _get_angle_from_vectors(x: np.array,
                            y: np.array,
                            z: np.array) -> Tuple[np.array, np.array]:
    """
    Get the in-plane and out of plane angle from x, y, z vectors

    Parameters:
    -----------
      x, y, z (numpy arrays): x, y, z coordinates of a vector

    Returns
    """
    pass

@namedtuplefy
def qangle(*,  # force keyword arguments
           Ei: float or np.array,
           hkl: np.array,  #numpy array of triplets
           DeltaE: float or np.array,
           sign: int or np.array,
           lattice: OrientedLattice,
           detector_constraints: bool = False,
           horizontal_extent: np.array = np.array([-180.,180.]),
           vertical_extent: np.array = np.array([-90.,90.]),
           horizontal_extent_low: np.array = np.array([0.,0.]),
           vertical_extent_low: np.array = np.array([0.,0.]),
           goniometer_constraints: bool = False,
           goniometer_range: np.array = np.array([-180.,180.]),
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
          - 0 correct result
          - 1 input parameters are wrong for the corresponding hkl
          - 2 conservation of energy and momentum can't be satisfied
          - 3 out of plane angle too high
          - 4 scattering outside detector
          - 5 scattering in the beam stop
          - 6 goniometer out of range

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
    # check input parameters
    try:
        len_hkl = len(hkl)
        if len(hkl[0]) != 3:
            raise ValueError()
    except (TypeError, ValueError):
        raise ValueError('hkl is not an array of triplets')

    try:
        # check if float
        Ei = float(Ei)
        Ei = np.full(len_hkl, Ei)
    except ValueError:
        raise ValueError('Ei is not a float or numpy array')
    except TypeError:
        if len(Ei) != len_hkl:
            raise ValueError('Ei has different length than hkl')
    try:
        # check if float
        DeltaE = float(DeltaE)
        DeltaE = np.full(len_hkl, DeltaE)
    except ValueError:
        raise ValueError('DeltaE is not a float or numpy array')
    except TypeError:
        if len(DeltaE) != len_hkl:
            raise ValueError('DeltaE has different length than hkl')

    try:
        # check if int
        sign = int(sign)
        sign = np.full(len_hkl,sign)
    except ValueError:
        raise ValueError('sign is not an int or numpy array')
    except TypeError:
        if len(sign) != len_hkl:
            raise ValueError('sign has different length than hkl')

    error_code = np.zeros(len_hkl, dtype=int)

    try:
        UB = lattice.getUB() * 2. * np.pi
    except:
        raise ValueError("Can't get the UB matrix from the lattice object")

    # inputs for geometry and goniometer constraints
    if detector_constraints:
        if horizontal_extent[0]<-180 or horizontal_extent_low[0]<horizontal_extent[0] or \
           horizontal_extent_low[1]<horizontal_extent_low[0] or \
           horizontal_extent[1]<horizontal_extent_low[1] or horizontal_extent[1]>180:
            raise ValueError("Horizontal constraints must obey -180 <= horizontal_extent[0] "
                             "<= horizontal_extent_low[0] <=horizontal_extent_low[1] <= "
                             "horizontal_extent[1] <=180")
    if goniometer_constraints:
        if goniometer_range[1]<goniometer_range[0] or \
           goniometer_range[0]<-180. or goniometer_range[1]<180.:
            raise ValueError("goniometer_range must be an increasing array, "
                             "with both limits between -180 and 180 degrees")


    Q_sample_x, Q_sample_y, Q_sample_z = np.matmul(UB, hkl.T)
    k_i = np.sqrt(Ei/2.0717)
    k_f = np.sqrt((Ei-DeltaE)/2.0717)
    
    # if any of the above elements in the array is NaN or inf, you get error_code 1
    error_code[np.logical_not(np.isfinite(Q_sample_x + Q_sample_y + Q_sample_z + k_i + k_f))] = 1
    
    # calculate delta
    error_code[np.abs(Q_sample_y) > 0.9998 * k_f] = 3
    delta = -np.arcsin(Q_sample_y / k_f)

    # calculate chi
    chi = np.arccos((k_i**2 + k_f**2 - Q_sample_x**2 - Q_sample_y**2 - Q_sample_z**2)/(2 * k_i * k_f *np.cos(delta)))
    bad_chi = np.logical_not(np.isfinite(chi))
    error_code[np.logical_and(bad_chi, error_code == 0)] = 2
    
    # check left/right
    bad_sign = np.abs(sign)!=1
    chi *= sign
    chi[bad_sign] = np.nan
    delta[bad_sign] = np.nan
    error_code[bad_sign] = 1

    # calculate Q_lab
    Q_lab_x = -k_f * np.cos(delta) * np.sin(chi)
    Q_lab_y = Q_sample_y
    Q_lab_z = k_i - k_f * np.cos(delta) * np.cos(chi)
    
    # calculate angles Q_lab
    in_plane_Q_angle = np.arctan2(Q_lab_x, Q_lab_z)
    out_plane_Q_angle = np.arcsin(Q_lab_y/np.sqrt(Q_lab_x**2 + Q_lab_y**2 + Q_lab_z**2))
    
    # calculate omega
    omega = np.arctan2(Q_lab_x, Q_lab_z) - np.arctan2(Q_sample_x, Q_sample_z)
    omega[omega>np.pi] -= 2*np.pi
    omega[omega<-np.pi] += 2*np.pi

    # transform all angles to degrees
    chi = np.degrees(chi)
    delta = np.degrees(delta)
    omega = np.degrees(omega)
    in_plane_Q_angle = np.degrees(in_plane_Q_angle) 
    out_plane_Q_angle = np.degrees(out_plane_Q_angle) 


        
    
    return dict(Q_lab_x=Q_lab_x, Q_lab_y=Q_lab_y, Q_lab_z=Q_lab_z,
                in_plane_Q_angle = in_plane_Q_angle, out_plane_Q_angle = out_plane_Q_angle,
                in_plane_kf_angle = chi, out_plane_kf_angle = delta,
                omega=omega, error_code=error_code)


