# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
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
import mantidqtinterfaces.dns_powder_tof.helpers.file_processing as file_helper
from mantidqtinterfaces.dns_powder_tof.data_structures.object_dict import \
    ObjectDict
from mantidqtinterfaces.dns_single_crystal_elastic.plot.elastic_single_crystal_helpers import \
    angle_to_q, get_hkl_float_array


def _get_mesh(omega, two_theta, z_mesh):
    omega_mesh, two_theta_mesh = np.meshgrid(omega, two_theta)
    nan_pos = ~np.isnan(z_mesh)
    omega_mesh = omega_mesh[nan_pos]
    two_theta_mesh = two_theta_mesh[nan_pos]
    z_mesh = z_mesh[nan_pos]
    return omega_mesh, two_theta_mesh, z_mesh


def _is_rectangular_mesh(omega, two_theta, z_mesh):
    return z_mesh.size == len(omega) * len(two_theta)


def _correct_rect_grid(z_mesh, omega_mesh, two_theta_mesh, omega,
                       two_theta):
    rectangular_grid = _is_rectangular_mesh(omega, two_theta, z_mesh)
    if rectangular_grid:
        omega_mesh, two_theta_mesh = np.meshgrid(omega, two_theta)
        z_mesh = np.reshape(z_mesh, omega_mesh.shape)
    return z_mesh, omega_mesh, two_theta_mesh, rectangular_grid


def _correct_omega_offset(omega, omega_offset):
    return np.subtract(omega, omega_offset)


def _get_unique(omega_mesh, two_theta_mesh):
    return np.unique(omega_mesh), np.unique(two_theta_mesh)


def _get_q_mesh(omega_mesh, two_theta_mesh, wavelength):
    return angle_to_q(two_theta=two_theta_mesh,
                      omega=omega_mesh,
                      wavelength=wavelength)


def _get_hkl_mesh(qx_mesh, qy_mesh, dx, dy):
    hklx_mesh = qx_mesh * dx / 2.0 / np.pi
    hkly_mesh = qy_mesh * dy / 2.0 / np.pi
    return hklx_mesh, hkly_mesh


def _get_interpolated(value, interpolated):
    return np.linspace(value.min(), value.max(),
                       value.size * (interpolated + 1))


class DNSScMap(ObjectDict):
    """
    Class for storing data of a single DNS single crystal plot.
    This is a dictionary  but can also be accessed like attributes.
    """

    def __init__(self,
                 two_theta=None,
                 omega=None,
                 z_mesh=None,
                 error_mesh=None,
                 parameter=None):
        super().__init__()
        # non interpolated data:
        omega = _correct_omega_offset(omega, parameter['omega_offset'])
        omega_mesh, two_theta_mesh, z_mesh = _get_mesh(omega, two_theta, z_mesh)
        omega, two_theta = _get_unique(omega_mesh, two_theta_mesh)
        z_mesh, omega_mesh, two_theta_mesh, rectangular_grid = \
            _correct_rect_grid(z_mesh, omega_mesh, two_theta_mesh, omega, two_theta)
        qx_mesh, qy_mesh = _get_q_mesh(
            omega_mesh, two_theta_mesh, parameter['wavelength'])
        hklx_mesh, hkly_mesh = _get_hkl_mesh(
            qx_mesh, qy_mesh, parameter['dx'], parameter['dy'])
        # setting attributes dictionary keys
        self.omega_interpolated = None
        self.rectangular_grid = rectangular_grid
        self.two_theta = two_theta
        self.omega = omega
        self.omega_offset = parameter['omega_offset']
        self.omega_mesh = omega_mesh
        self.two_theta_mesh = two_theta_mesh
        self.qx_mesh = qx_mesh
        self.qy_mesh = qy_mesh
        self.hklx_mesh = hklx_mesh
        self.hkly_mesh = hkly_mesh
        self.z_mesh = z_mesh
        self.error_mesh = error_mesh
        self.dx = parameter['dx']
        self.dy = parameter['dy']
        self.hkl1 = parameter['hkl1']
        self.hkl2 = parameter['hkl2']
        self.wavelength = parameter['wavelength']
        self.two_theta_and_omega_mesh = [self.two_theta_mesh, self.omega_mesh,
                                         self.z_mesh]
        self.hkl_mesh = [self.hklx_mesh, self.hkly_mesh, self.z_mesh]
        self.qxqy_mesh = [self.qx_mesh, self.qy_mesh, self.z_mesh]

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

    def save_ascii(self, filename):
        if filename.endswith('.txt'):
            filename = filename[0:-4]
        not_interpolated_filename = filename + '_no_interp.txt'
        interpolated_filename = filename + '_interp.txt'
        file_helper.create_dir_from_filename(not_interpolated_filename)
        not_interpolated, interpolated = self.create_np_array()
        header = ' 2theta, omega, qx,      qy ,      hklx,' \
                 '     hkly,         Intensity,          Error'
        np.savetxt(not_interpolated_filename,
                   not_interpolated,
                   fmt='%7.3f %7.3f %8.5f %8.5f %9.5f %9.5f %15.5f %15.5f',
                   delimiter=' ',
                   newline='\n',
                   header=header)
        if self.rectangular_grid:
            np.savetxt(interpolated_filename,
                       interpolated,
                       fmt='%7.3f %7.3f %8.5f %8.5f %9.5f %9.5f %15.5f',
                       delimiter=' ',
                       newline='\n',
                       header=header[0:-16])

    def _get_z_mesh_interpolation(self):
        f = scipy.interpolate.RectBivariateSpline( self.two_theta, self.omega, self.z_mesh, kx=2, ky=2)
        return f(self.two_theta_interpolated, self.omega_interpolated)

    def interpolate_quad_mesh(self, interpolation=3):
        if not self.rectangular_grid or interpolation <= 0:
            return
        self.two_theta_interpolated = _get_interpolated(self.two_theta, interpolation)
        self.omega_interpolated = _get_interpolated(self.omega, interpolation)
        omega_mesh_interpolated, two_theta_mesh_interpolated = np.meshgrid(
            self.omega_interpolated, self.two_theta_interpolated)
        qx_mesh_interpolated, qy_mesh_interpolated = _get_q_mesh(
            omega_mesh_interpolated, two_theta_mesh_interpolated, self.wavelength)
        hklx_mesh_interpolated, hkly_mesh_interpolated = _get_hkl_mesh(
            qx_mesh_interpolated, qy_mesh_interpolated, self.dx, self.dy)
        self.z_mesh_interpolated = self._get_z_mesh_interpolation()
        self.omega_mesh_interpolated = omega_mesh_interpolated
        self.two_theta_mesh_interpolated = two_theta_mesh_interpolated
        self.qx_mesh_interpolated = qx_mesh_interpolated
        self.qy_mesh_interpolated = qy_mesh_interpolated
        self.hklx_mesh_interpolated = hklx_mesh_interpolated
        self.hkly_mesh_interpolated = hkly_mesh_interpolated
        self.two_theta_and_omega_mesh_interpolated = [
            self.two_theta_mesh_interpolated, self.omega_mesh_interpolated, self.z_mesh_interpolated
        ]
        self.hkl_mesh_interpolated = [
            self.hklx_mesh_interpolated, self.hkly_mesh_interpolated, self.z_mesh_interpolated
        ]
        self.qxqy_mesh_interpolated = [
            self.qx_mesh_interpolated, self.qy_mesh_interpolated, self.z_mesh_interpolated
        ]

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
        triangulator_refiner, z_test_refiner = refiner.refine_field(z,
                                                                    subdiv=interpolation,
                                                                    triinterpolator=interpolator)
        if interpolation <= 0:
            return [triangulator, z.flatten()]
        return [triangulator_refiner, z_test_refiner.flatten()]

    def get_dns_map_border(self, mesh_name):
        two_theta = self.two_theta
        omega = self.omega
        dns_path = np.zeros((2 * two_theta.size + 2 * omega.size, 2))
        hkl_path = np.zeros((2 * two_theta.size + 2 * omega.size, 2))
        dns_path[:, 0] = np.concatenate(
            (two_theta[0] * np.ones(omega.size), two_theta,
             two_theta[-1] * np.ones(omega.size), np.flip(two_theta)))
        dns_path[:, 1] = np.concatenate(
            (np.flip(omega), omega[0] * np.ones(two_theta.size), omega,
             omega[-1] * np.ones(two_theta.size)))
        dns_path[:, 0], dns_path[:, 1] = angle_to_q(dns_path[:, 0], dns_path[:, 1],
                                                    self.wavelength)
        if 'hkl' in mesh_name:
            hkl_path[:, 0] = dns_path[:, 0] * self.dx / 2.0 / np.pi
            hkl_path[:, 1] = dns_path[:, 1] * self.dy / 2.0 / np.pi
            return path.Path(hkl_path)
        return path.Path(dns_path)

    def mask_triangles(self, mesh_name):
        if 'two_theta_and_omega' in mesh_name:
            return self.triangulation
        x, y, _z = getattr(self, mesh_name)
        x = x.flatten()
        y = y.flatten()
        dns_path = self.get_dns_map_border(mesh_name)
        triangles = self.triangulation.triangles
        x_y_triangles = np.zeros((len(x[triangles]), 2))
        x_y_triangles[:, 0] = np.mean(x[triangles], axis=1)
        x_y_triangles[:, 1] = np.mean(y[triangles], axis=1)
        maxi = dns_path.contains_points(x_y_triangles)
        self.triangulation.set_mask(np.invert(maxi))

        return self.triangulation

    def return_changing_indexes(self):
        hkl1 = get_hkl_float_array(self.hkl1)
        hkl2 = get_hkl_float_array(self.hkl2)
        hkl = np.add(np.outer(self.hklx_mesh.flatten()[0:10], hkl1),
                     np.outer(self.hkly_mesh.flatten()[0:10], hkl2))
        mylist = [
            len(np.unique(hkl[:, 0])),
            len(np.unique(hkl[:, 1])),
            len(np.unique(hkl[:, 2]))
        ]
        new_list = list(range(len(mylist)))
        del new_list[mylist.index(min(mylist))]
        return new_list

    def get_crystal_axis_names(self):
        changing_index = self.return_changing_indexes()
        name_dict = {0: 'h (r.l.u)', 1: 'k (r.l.u)', 2: 'l (r.l.u)'}
        return name_dict[changing_index[0]], name_dict[changing_index[1]]

    def get_changing_hkl_components(self):
        index = self.return_changing_indexes()
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
