# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

"""
Class which loads and stores a single DNS datafile in a dictionary.
"""

import numpy as np
import scipy
from matplotlib import path
from matplotlib import tri
from matplotlib.tri import LinearTriInterpolator, UniformTriRefiner
from mantidqtinterfaces.dns_powder_tof.data_structures.object_dict import ObjectDict
from mantidqtinterfaces.dns_single_crystal_elastic.plot.elastic_single_crystal_helpers import angle_to_q, get_hkl_float_array


def _get_mesh(omega, two_theta, z_mesh, z_error_mesh):
    omega_mesh_no_nan, two_theta_mesh_no_nan = np.meshgrid(omega, two_theta)
    not_nan_pos = ~np.isnan(z_mesh)
    omega_mesh_no_nan = omega_mesh_no_nan[not_nan_pos]
    two_theta_mesh_no_nan = two_theta_mesh_no_nan[not_nan_pos]
    z_mesh_no_nan = z_mesh[not_nan_pos]
    z_error_mesh_no_nan = z_error_mesh[not_nan_pos]
    return omega_mesh_no_nan, two_theta_mesh_no_nan, z_mesh_no_nan, z_error_mesh_no_nan


def _is_rectangular_mesh(omega, two_theta, z_mesh):
    return z_mesh.size == len(omega) * len(two_theta)


def _correct_rect_grid(omega_mesh, two_theta_mesh, z_mesh, z_error_mesh, omega, two_theta):
    rectangular_grid = _is_rectangular_mesh(omega, two_theta, z_mesh)
    if rectangular_grid:
        omega_mesh, two_theta_mesh = np.meshgrid(omega, two_theta)
        z_mesh = np.reshape(z_mesh, omega_mesh.shape)
        z_error_mesh = np.reshape(z_error_mesh, omega_mesh.shape)
    return omega_mesh, two_theta_mesh, z_mesh, z_error_mesh, rectangular_grid


def _correct_omega_offset(omega, omega_offset):
    return np.subtract(omega, omega_offset)


def _get_unique(omega_mesh, two_theta_mesh):
    omega = np.unique(omega_mesh)
    two_theta = np.unique(two_theta_mesh)
    return omega, two_theta


def _get_q_mesh(omega_mesh, two_theta_mesh, wavelength):
    q_x, q_y = angle_to_q(two_theta=two_theta_mesh, omega=omega_mesh, wavelength=wavelength)
    return q_x, q_y


def _get_hkl_mesh(qx_mesh, qy_mesh, dx, dy):
    hklx_mesh = qx_mesh * dx / 2.0 / np.pi
    hkly_mesh = qy_mesh * dy / 2.0 / np.pi
    return hklx_mesh, hkly_mesh


def _get_interpolated(value, interpolation_order):
    # interpolation_order = 0: no interpolation
    # interpolation_order = 1 (2, or 3): the amount of datapoints is
    # doubled (tripled or quadrupled), the datapoints are uniformly
    # spaced between min and max values
    interpolated_values = np.linspace(value.min(), value.max(), value.size * (interpolation_order + 1))
    return interpolated_values


class DNSScMap(ObjectDict):
    """
    Class for storing data of a single DNS single crystal plot.
    """

    def __init__(self, parameter, two_theta=None, omega=None, z_mesh=None, error_mesh=None):
        super().__init__()
        # non interpolated data:
        omega_corrected = _correct_omega_offset(omega, parameter["omega_offset"])
        omega_mesh, two_theta_mesh, z_mesh, z_error_mesh = _get_mesh(omega_corrected, two_theta, z_mesh, error_mesh)
        omega_unique, two_theta_unique = _get_unique(omega_mesh, two_theta_mesh)
        omega_mesh, two_theta_mesh, z_mesh, z_error_mesh, rectangular_grid = _correct_rect_grid(
            omega_mesh, two_theta_mesh, z_mesh, z_error_mesh, omega_corrected, two_theta_unique
        )
        qx_mesh, qy_mesh = _get_q_mesh(omega_mesh, two_theta_mesh, parameter["wavelength"])
        hklx_mesh, hkly_mesh = _get_hkl_mesh(qx_mesh, qy_mesh, parameter["dx"], parameter["dy"])
        # setting attributes dictionary keys
        self.omega_interpolated = None
        self.rectangular_grid = rectangular_grid
        self.two_theta = two_theta_unique
        self.omega = omega_unique
        self.omega_offset = parameter["omega_offset"]
        self.omega_mesh = omega_mesh
        self.two_theta_mesh = two_theta_mesh
        self.qx_mesh = qx_mesh
        self.qy_mesh = qy_mesh
        self.hklx_mesh = hklx_mesh
        self.hkly_mesh = hkly_mesh
        self.z_mesh = z_mesh
        self.error_mesh = z_error_mesh
        self.dx = parameter["dx"]
        self.dy = parameter["dy"]
        self.hkl1 = parameter["hkl1"]
        self.hkl2 = parameter["hkl2"]
        self.wavelength = parameter["wavelength"]
        self.hkl_mesh = [self.hklx_mesh, self.hkly_mesh, self.z_mesh]
        self.qxqy_mesh = [self.qx_mesh, self.qy_mesh, self.z_mesh]
        self.angular_mesh = [self.two_theta_mesh, self.omega_mesh, self.z_mesh]

    def create_np_array(self):
        interpolated = None
        non_interpolated = np.zeros((self.omega_mesh.size, 8))
        non_interpolated[:, 0] = self.two_theta_mesh.flatten()
        non_interpolated[:, 1] = self.omega_mesh.flatten()
        non_interpolated[:, 2] = self.qx_mesh.flatten()
        non_interpolated[:, 3] = self.qy_mesh.flatten()
        non_interpolated[:, 4] = self.hklx_mesh.flatten()
        non_interpolated[:, 5] = self.hkly_mesh.flatten()
        non_interpolated[:, 6] = self.z_mesh.flatten()
        non_interpolated[:, 7] = self.error_mesh.flatten()
        if self.rectangular_grid and self.omega_interpolated is not None:
            interpolated = np.zeros((self.omega_mesh_interpolated.size, 7))
            interpolated[:, 0] = self.two_theta_mesh_interpolated.flatten()
            interpolated[:, 1] = self.omega_mesh_interpolated.flatten()
            interpolated[:, 2] = self.qx_mesh_interpolated.flatten()
            interpolated[:, 3] = self.qy_mesh_interpolated.flatten()
            interpolated[:, 4] = self.hklx_mesh_interpolated.flatten()
            interpolated[:, 5] = self.hkly_mesh_interpolated.flatten()
            interpolated[:, 6] = self.z_mesh_interpolated.flatten()
        return [non_interpolated, interpolated]

    def _get_z_mesh_interpolation(self):
        f = scipy.interpolate.RectBivariateSpline(self.two_theta, self.omega, self.z_mesh, kx=1, ky=1)
        return f(self.two_theta_interpolated, self.omega_interpolated)

    def triangulate(self, mesh_name, switch=False):
        plot_x, plot_y, _z = getattr(self, mesh_name)
        if switch:
            plot_x, plot_y = plot_y, plot_x
        self.triangulation = tri.Triangulation(plot_x.flatten(), plot_y.flatten())
        return self.triangulation

    def interpolate_triangulation(self, interpolation=0):
        if self.triangulation is None:
            return None
        z = self.z_mesh
        triangulator = self.triangulation
        interpolator = LinearTriInterpolator(triangulator, z.flatten())
        refiner = UniformTriRefiner(triangulator)
        triangulator_refiner, z_test_refiner = refiner.refine_field(z, subdiv=interpolation, triinterpolator=interpolator)
        if interpolation <= 0:
            return [triangulator, z.flatten()]
        return [triangulator_refiner, z_test_refiner.flatten()]

    def interpolate_quad_mesh(self, interpolation=3):
        if not self.rectangular_grid or interpolation <= 0:
            return
        self.two_theta_interpolated = _get_interpolated(self.two_theta, interpolation)
        self.omega_interpolated = _get_interpolated(self.omega, interpolation)
        omega_mesh_interpolated, two_theta_mesh_interpolated = np.meshgrid(self.omega_interpolated, self.two_theta_interpolated)
        qx_mesh_interpolated, qy_mesh_interpolated = _get_q_mesh(omega_mesh_interpolated, two_theta_mesh_interpolated, self.wavelength)
        hklx_mesh_interpolated, hkly_mesh_interpolated = _get_hkl_mesh(qx_mesh_interpolated, qy_mesh_interpolated, self.dx, self.dy)
        self.z_mesh_interpolated = self._get_z_mesh_interpolation()
        self.omega_mesh_interpolated = omega_mesh_interpolated
        self.two_theta_mesh_interpolated = two_theta_mesh_interpolated
        self.qx_mesh_interpolated = qx_mesh_interpolated
        self.qy_mesh_interpolated = qy_mesh_interpolated
        self.hklx_mesh_interpolated = hklx_mesh_interpolated
        self.hkly_mesh_interpolated = hkly_mesh_interpolated
        self.angular_mesh_interpolated = [self.two_theta_mesh_interpolated, self.omega_mesh_interpolated, self.z_mesh_interpolated]
        self.hkl_mesh_interpolated = [self.hklx_mesh_interpolated, self.hkly_mesh_interpolated, self.z_mesh_interpolated]
        self.qxqy_mesh_interpolated = [self.qx_mesh_interpolated, self.qy_mesh_interpolated, self.z_mesh_interpolated]

    def get_dns_map_border(self, mesh_name, switch):
        two_theta = self.two_theta
        omega = self.omega
        dns_path = np.zeros((2 * two_theta.size + 2 * omega.size, 2))
        angular_border = np.zeros((2 * two_theta.size + 2 * omega.size, 2))
        hkl_border = np.zeros((2 * two_theta.size + 2 * omega.size, 2))
        qxqy_border = np.zeros((2 * two_theta.size + 2 * omega.size, 2))
        # In two_theta-omega plane we set borders of the rectangle, specifying edge sides of the rectangle.
        # For a typical measurement at the DNS (two theta: 5...124, omega: 135...304 deg) it would be:
        # a) left side two_theta=5; omega=135, 136, ... ,304
        # b) bottom side two_theta=5, 6, ..., 124; omega=135
        # c) right side two_theta=124, omega=135, 136, ... ,304
        # d) upper side two_theta=5, 6, ..., 124; omega=304
        # set first column values (two theta)
        angular_border[:, 0] = np.concatenate(
            (two_theta[0] * np.ones(omega.size), two_theta, two_theta[-1] * np.ones(omega.size), np.flip(two_theta))
        )  # [5,5,..,5(170), 5,6,7,...,124(120), 124,124,..,124(170), 124,123,...,5(120)]
        # set second column values (omega)
        angular_border[:, 1] = np.concatenate(
            (np.flip(omega), omega[0] * np.ones(two_theta.size), omega, omega[-1] * np.ones(two_theta.size))
        )
        # [304,303,...,135(170), 135,135,...,135(120), 135,136,...,304(170), 304,304,..,304(120)]
        qxqy_border[:, 0], qxqy_border[:, 1] = angle_to_q(angular_border[:, 0], angular_border[:, 1], self.wavelength)
        hkl_border[:, 0], hkl_border[:, 1] = qxqy_border[:, 0] * self.dx / 2.0 / np.pi, qxqy_border[:, 1] * self.dy / 2.0 / np.pi
        if "angular" in mesh_name:
            dns_path = angular_border
        elif "qxqy" in mesh_name:
            dns_path = qxqy_border
        elif "hkl" in mesh_name:
            dns_path = hkl_border
        if switch:
            dns_path = dns_path[::-1, ::-1]
        return path.Path(dns_path)

    def mask_triangles(self, mesh_name, switch):
        if "angular" in mesh_name:
            return self.triangulation
        x, y, _z = getattr(self, mesh_name)
        if switch:
            x, y = y, x
        x = x.flatten()
        y = y.flatten()
        dns_path = self.get_dns_map_border(mesh_name, switch)
        triangles = self.triangulation.triangles
        x_y_triangles = np.zeros((len(x[triangles]), 2))
        x_y_triangles[:, 0] = np.mean(x[triangles], axis=1)
        x_y_triangles[:, 1] = np.mean(y[triangles], axis=1)
        maxi = dns_path.contains_points(x_y_triangles)
        self.triangulation.set_mask(np.invert(maxi))
        return self.triangulation

    def get_changing_indexes(self):
        """
        Keeps track of the coordinates after transformation to the horizontal
        scattering plane and returns the list of relevant indices of (x,y)
        crystallographic coordinates.
        """
        hkl1 = get_hkl_float_array(self.hkl1)
        hkl2 = get_hkl_float_array(self.hkl2)
        hkl = np.add(np.outer(self.hklx_mesh.flatten()[0:10], hkl1), np.outer(self.hkly_mesh.flatten()[0:10], hkl2))
        projection_dims = [len(np.unique(hkl[:, 0])), len(np.unique(hkl[:, 1])), len(np.unique(hkl[:, 2]))]
        basis_indexes = list(range(len(projection_dims)))
        del basis_indexes[projection_dims.index(min(projection_dims))]
        return basis_indexes

    def get_crystal_axis_names(self):
        changing_index = self.get_changing_indexes()
        name_dict = {0: "h (r.l.u)", 1: "k (r.l.u)", 2: "l (r.l.u)"}
        return name_dict[changing_index[0]], name_dict[changing_index[1]]

    def get_changing_hkl_components(self):
        index = self.get_changing_indexes()
        hkl1 = get_hkl_float_array(self.hkl1)
        hkl2 = get_hkl_float_array(self.hkl2)
        # transformation to crystal axis following
        # hkl_comp1 = a * x + b * y
        # hkl_comp2 = c * x + d * y
        a = hkl1[index[0]]
        b = hkl2[index[0]]
        c = hkl1[index[1]]
        d = hkl2[index[1]]
        return a, b, c, d
