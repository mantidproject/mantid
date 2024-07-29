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
from matplotlib import path
from matplotlib import tri
from mantidqtinterfaces.dns_powder_tof.data_structures.object_dict import ObjectDict
from mantidqtinterfaces.dns_single_crystal_elastic.plot.elastic_single_crystal_helpers import angle_to_q


def _get_mesh(omega, two_theta, z_mesh):
    omega_mesh_no_nan, two_theta_mesh_no_nan = np.meshgrid(omega, two_theta)
    not_nan_pos = ~np.isnan(z_mesh)
    omega_mesh_no_nan = omega_mesh_no_nan[not_nan_pos]
    two_theta_mesh_no_nan = two_theta_mesh_no_nan[not_nan_pos]
    z_mesh_no_nan = z_mesh[not_nan_pos]
    return omega_mesh_no_nan, two_theta_mesh_no_nan, z_mesh_no_nan


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


class DNSScMap(ObjectDict):
    """
    Class for storing data of a single DNS single crystal plot.
    """

    def __init__(self, parameter, two_theta=None, omega=None, z_mesh=None, error_mesh=None):
        super().__init__()
        # non interpolated data:
        omega_mesh, two_theta_mesh, z_mesh = _get_mesh(omega, two_theta, z_mesh)
        omega_unique, two_theta_unique = _get_unique(omega_mesh, two_theta_mesh)
        qx_mesh, qy_mesh = _get_q_mesh(omega_mesh, two_theta_mesh, parameter["wavelength"])
        hklx_mesh, hkly_mesh = _get_hkl_mesh(qx_mesh, qy_mesh, parameter["dx"], parameter["dy"])
        # setting attributes dictionary keys
        self.omega_interpolated = None
        self.two_theta = two_theta_unique
        self.omega = omega_unique
        self.omega_offset = parameter["omega_offset"]
        self.hklx_mesh = hklx_mesh
        self.hkly_mesh = hkly_mesh
        self.z_mesh = z_mesh
        self.error_mesh = error_mesh
        self.dx = parameter["dx"]
        self.dy = parameter["dy"]
        self.hkl1 = parameter["hkl1"]
        self.hkl2 = parameter["hkl2"]
        self.wavelength = parameter["wavelength"]
        self.hkl_mesh = [self.hklx_mesh, self.hkly_mesh, self.z_mesh]

    def triangulate(self, mesh_name, switch=False):
        plot_x, plot_y, _z = getattr(self, mesh_name)
        self.triangulation = tri.Triangulation(plot_x.flatten(), plot_y.flatten())
        return self.triangulation

    def interpolate_triangulation(self, interpolation=0):
        if self.triangulation is None:
            return None
        z = self.z_mesh
        triangulator = self.triangulation
        return [triangulator, z.flatten()]

    def get_dns_map_border(self, mesh_name):
        two_theta = self.two_theta
        omega = self.omega
        dns_path = np.zeros((2 * two_theta.size + 2 * omega.size, 2))
        hkl_path = np.zeros((2 * two_theta.size + 2 * omega.size, 2))
        dns_path[:, 0] = np.concatenate(
            (two_theta[0] * np.ones(omega.size), two_theta, two_theta[-1] * np.ones(omega.size), np.flip(two_theta))
        )
        dns_path[:, 1] = np.concatenate((np.flip(omega), omega[0] * np.ones(two_theta.size), omega, omega[-1] * np.ones(two_theta.size)))
        dns_path[:, 0], dns_path[:, 1] = angle_to_q(dns_path[:, 0], dns_path[:, 1], self.wavelength)
        if "hkl" in mesh_name:
            hkl_path[:, 0] = dns_path[:, 0] * self.dx / 2.0 / np.pi
            hkl_path[:, 1] = dns_path[:, 1] * self.dy / 2.0 / np.pi
            return path.Path(hkl_path)
        return path.Path(dns_path)

    def mask_triangles(self, mesh_name):
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
