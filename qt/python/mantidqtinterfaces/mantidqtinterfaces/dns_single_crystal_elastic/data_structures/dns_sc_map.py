# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
"""
Class which loads and stores a single DNS datafile in a dictionary
"""

import numpy as np
import scipy

from matplotlib import path
from matplotlib import tri
from matplotlib.tri import LinearTriInterpolator, UniformTriRefiner
import mantidqtinterfaces.dns_powder_tof.helpers.file_processing as file_helper
from mantidqtinterfaces.dns_powder_tof.data_structures.object_dict import \
    ObjectDict
from mantidqtinterfaces.dns_sc_elastic.plot.elastic_sc_helpers import \
    angle_to_q, get_hkl_float_array


def _get_mesh(omega, ttheta, z_mesh):
    omega_mesh, ttheta_mesh = np.meshgrid(omega, ttheta)
    nan_pos = ~np.isnan(z_mesh)
    omega_mesh = omega_mesh[nan_pos]
    ttheta_mesh = ttheta_mesh[nan_pos]
    z_mesh = z_mesh[nan_pos]
    return omega_mesh, ttheta_mesh, z_mesh


def _is_rectangular_mesh(omega, ttheta, z_mesh):
    return z_mesh.size == len(omega) * len(ttheta)


def _correct_rect_grid(z_mesh, omega_mesh, ttheta_mesh, omega,
                       ttheta):
    rectangular_grid = _is_rectangular_mesh(omega, ttheta, z_mesh)
    if rectangular_grid:
        omega_mesh, ttheta_mesh = np.meshgrid(omega, ttheta)
        z_mesh = np.reshape(z_mesh, omega_mesh.shape)
    return z_mesh, omega_mesh, ttheta_mesh, rectangular_grid


def _correct_omegaoffset(omega, omegaoffset):
    return np.subtract(omega, omegaoffset)


def _get_unique(omega_mesh, ttheta_mesh):
    return np.unique(omega_mesh), np.unique(ttheta_mesh)


def _get_q_mesh(omega_mesh, ttheta_mesh, wavelength):
    return angle_to_q(ttheta=ttheta_mesh,
                      omega=omega_mesh,
                      wavelength=wavelength)


def _get_hkl_mesh(qx_mesh, qy_mesh, dx, dy):
    hklx_mesh = qx_mesh * dx / 2.0 / np.pi
    hkly_mesh = qy_mesh * dy / 2.0 / np.pi
    return hklx_mesh, hkly_mesh


def _get_interpolated(value, interp):
    return np.linspace(value.min(), value.max(),
                       value.size * (interp + 1))


class DNSScMap(ObjectDict):
    """
    class for storing data of a single dns sc plot
    this is a dictionary  but can also be accessed like atributes
    """

    # pylint: disable=too-many-instance-attributes, attribute-defined-outside-init, too-many-arguments
    # arguments are dictionary keys here

    def __init__(self,
                 ttheta=None,
                 omega=None,
                 z_mesh=None,
                 error_mesh=None,
                 parameter=None):
        super().__init__()
        # non interpolated data:
        omega = _correct_omegaoffset(omega, parameter['omega_offset'])
        omega_mesh, ttheta_mesh, z_mesh = _get_mesh(omega, ttheta, z_mesh)
        omega, ttheta = _get_unique(omega_mesh, ttheta_mesh)
        z_mesh, omega_mesh, ttheta_mesh, rectangular_grid = \
            _correct_rect_grid(z_mesh, omega_mesh, ttheta_mesh, omega, ttheta)
        qx_mesh, qy_mesh = _get_q_mesh(
            omega_mesh, ttheta_mesh, parameter['wavelength'])
        hklx_mesh, hkly_mesh = _get_hkl_mesh(
            qx_mesh, qy_mesh, parameter['dx'], parameter['dy'])
        # setting atributes dictionary keys
        self.omega_intp = None
        self.rectangular_grid = rectangular_grid
        self.ttheta = ttheta
        self.omega = omega
        self.omegaoffset = parameter['omega_offset']
        self.omega_mesh = omega_mesh
        self.ttheta_mesh = ttheta_mesh
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
        self.tthomega_mesh = [self.ttheta_mesh, self.omega_mesh,
                              self.z_mesh]
        self.hkl_mesh = [self.hklx_mesh, self.hkly_mesh, self.z_mesh]
        self.qxqy_mesh = [self.qx_mesh, self.qy_mesh, self.z_mesh]

    def create_np_array(self):
        interpolated = None
        non_interpolated = np.zeros((self.omega_mesh.size, 8))
        non_interpolated[:, 0] = self.ttheta_mesh.flatten()
        non_interpolated[:, 1] = self.omega_mesh.flatten()
        non_interpolated[:, 2] = self.qx_mesh.flatten()
        non_interpolated[:, 3] = self.qy_mesh.flatten()
        non_interpolated[:, 4] = self.hklx_mesh.flatten()
        non_interpolated[:, 5] = self.hkly_mesh.flatten()
        non_interpolated[:, 6] = self.z_mesh.flatten()
        non_interpolated[:, 7] = self.error_mesh.flatten()
        if self.rectangular_grid and self.omega_intp is not None:
            interpolated = np.zeros((self.omega_mesh_intp.size, 7))
            interpolated[:, 0] = self.ttheta_mesh_intp.flatten()
            interpolated[:, 1] = self.omega_mesh_intp.flatten()
            interpolated[:, 2] = self.qx_mesh_intp.flatten()
            interpolated[:, 3] = self.qy_mesh_intp.flatten()
            interpolated[:, 4] = self.hklx_mesh_intp.flatten()
            interpolated[:, 5] = self.hkly_mesh_intp.flatten()
            interpolated[:, 6] = self.z_mesh_intp.flatten()
        return [non_interpolated, interpolated]

    def save_ascii(self, filename):
        if filename.endswith('.txt'):
            filename = filename[0:-4]
        nintpfilename = filename + '_no_interp.txt'
        intpfilename = filename + '_interp.txt'
        file_helper.create_dir_from_filename(nintpfilename)
        nintp, intp = self.create_np_array()
        header = ' 2theta, omega, qx,      qy ,      hklx,' \
                 '     hkly,         Intensity,          Error'
        np.savetxt(nintpfilename,
                   nintp,
                   fmt='%7.3f %7.3f %8.5f %8.5f %9.5f %9.5f %15.5f %15.5f',
                   delimiter=' ',
                   newline='\n',
                   header=header)
        if self.rectangular_grid:
            np.savetxt(intpfilename,
                       intp,
                       fmt='%7.3f %7.3f %8.5f %8.5f %9.5f %9.5f %15.5f',
                       delimiter=' ',
                       newline='\n',
                       header=header[0:-16])

    def _get_z_mesh_interp(self):
        f = scipy.interpolate.interp2d(self.omega, self.ttheta, self.z_mesh)
        return f(self.omega_intp, self.ttheta_intp)

    def interpolate_quad_mesh(self, interp=3):
        if not self.rectangular_grid or interp <= 0:
            return
        self.ttheta_intp = _get_interpolated(self.ttheta, interp)
        self.omega_intp = _get_interpolated(self.omega, interp)
        omega_mesh_intp, ttheta_mesh_intp = np.meshgrid(
            self.omega_intp, self.ttheta_intp)
        qx_mesh_intp, qy_mesh_intp = _get_q_mesh(
            omega_mesh_intp, ttheta_mesh_intp, self.wavelength)
        hklx_mesh_intp, hkly_mesh_intp = _get_hkl_mesh(
            qx_mesh_intp, qy_mesh_intp, self.dx, self.dy)
        self.z_mesh_intp = self._get_z_mesh_interp()
        self.omega_mesh_intp = omega_mesh_intp
        self.ttheta_mesh_intp = ttheta_mesh_intp
        self.qx_mesh_intp = qx_mesh_intp
        self.qy_mesh_intp = qy_mesh_intp
        self.hklx_mesh_intp = hklx_mesh_intp
        self.hkly_mesh_intp = hkly_mesh_intp
        self.tthomega_mesh_intp = [
            self.ttheta_mesh_intp, self.omega_mesh_intp, self.z_mesh_intp
        ]
        self.hkl_mesh_intp = [
            self.hklx_mesh_intp, self.hkly_mesh_intp, self.z_mesh_intp
        ]
        self.qxqy_mesh_intp = [
            self.qx_mesh_intp, self.qy_mesh_intp, self.z_mesh_intp
        ]

    def triangulate(self, meshname, switch=False):
        plotx, ploty, _z = getattr(self, meshname)
        if switch:
            plotx, ploty = ploty, plotx
        self.triang = tri.Triangulation(plotx.flatten(), ploty.flatten())
        return self.triang

    def interpolate_triangulation(self, interp=0):
        if self.triang is None:
            return None
        z = self.z_mesh
        triang = self.triang
        ipolator = LinearTriInterpolator(triang, z.flatten())
        refiner = UniformTriRefiner(triang)
        tri_refi, z_test_refi = refiner.refine_field(z,
                                                     subdiv=interp,
                                                     triinterpolator=ipolator)
        if interp <= 0:
            return [triang, z.flatten()]
        return [tri_refi, z_test_refi.flatten()]

    def get_dns_map_border(self, meshname):
        ttheta = self.ttheta
        omega = self.omega
        dnspath = np.zeros((2 * ttheta.size + 2 * omega.size, 2))
        hklpath = np.zeros((2 * ttheta.size + 2 * omega.size, 2))
        dnspath[:, 0] = np.concatenate(
            (ttheta[0] * np.ones(omega.size), ttheta,
             ttheta[-1] * np.ones(omega.size), np.flip(ttheta)))
        dnspath[:, 1] = np.concatenate(
            (np.flip(omega), omega[0] * np.ones(ttheta.size), omega,
             omega[-1] * np.ones(ttheta.size)))
        dnspath[:, 0], dnspath[:, 1] = angle_to_q(dnspath[:, 0], dnspath[:, 1],
                                                  self.wavelength)
        if 'hkl' in meshname:
            hklpath[:, 0] = dnspath[:, 0] * self.dx / 2.0 / np.pi
            hklpath[:, 1] = dnspath[:, 1] * self.dy / 2.0 / np.pi
            return path.Path(hklpath)
        return path.Path(dnspath)

    def mask_triangles(self, meshname):
        if 'tthomega' in meshname:
            return self.triang
        x, y, _z = getattr(self, meshname)
        x = x.flatten()
        y = y.flatten()
        dnspath = self.get_dns_map_border(meshname)
        triangles = self.triang.triangles
        x_y_tri = np.zeros((len(x[triangles]), 2))
        x_y_tri[:, 0] = np.mean(x[triangles], axis=1)
        x_y_tri[:, 1] = np.mean(y[triangles], axis=1)
        maxi = dnspath.contains_points(x_y_tri)
        self.triang.set_mask(np.invert(maxi))

        return self.triang

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
        newlist = list(range(len(mylist)))
        del newlist[mylist.index(min(mylist))]
        return newlist

    def get_crystal_axis_names(self):
        changing_index = self.return_changing_indexes()
        namedict = {0: 'h (r.l.u)', 1: 'k (r.l.u)', 2: 'l (r.l.u)'}
        return namedict[changing_index[0]], namedict[changing_index[1]]

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
