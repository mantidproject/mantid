# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,

#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
"""
DNS elastic powder plot presenter
"""
import numpy as np
import mantidqtinterfaces.dns_sc_elastic.plot.elastic_sc_helpers as helper
from mantidqtinterfaces.dns_powder_tof.data_structures.dns_obs_model \
    import DNSObsModel
from mantidqtinterfaces.dns_sc_elastic.data_structures.dns_sc_map \
    import DNSScMap
from mantidqtinterfaces.dns_powder_tof.data_structures.object_dict \
    import ObjectDict
from mantidqtinterfaces.dns_sc_elastic.plot.elastic_sc_helpers \
    import get_hkl_intensity_from_cursor


class DNSElasticSCPlotModel(DNSObsModel):
    """
    Model for DNS plot calculations
    converts data from omega twotheta into qx,qy and hkl1, hkl2
    creates hull of dnsdata, to filter points
    """

    def __init__(self, parent):
        super().__init__(parent)
        self._sc_map = None

        self._data = ObjectDict()
        self._data.x = None
        self._data.y = None
        self._data.z = None
        self._data.zmin = None
        self._data.zmax = None
        self._data.pzmin = None
        self._data.triang = None
        self._data.ztriang = None

    def create_sc_map(self, data_array, options, initial_values=None):
        ttheta = data_array['ttheta']
        omega = data_array['omega']
        z_mesh = data_array['intensity']
        error = data_array['error']
        parameter = {'wavelength': options['wavelength'],
                     'dx': options['dx'],
                     'dy': options['dy'],
                     'hkl1': options['hkl1'],
                     'hkl2': options['hkl2'],
                     'omega_offset': options['omega_offset']
                     }
        if initial_values is not None:
            parameter.update(initial_values)
        self._sc_map = DNSScMap(ttheta=ttheta,
                                omega=omega,
                                z_mesh=z_mesh,
                                error_mesh=error,
                                parameter=parameter)
        return self._sc_map

    def has_data(self):
        return self._data.x is not None

    def get_projections(self, xlim, ylim):
        limits = np.append(xlim, ylim)
        x, y, z = helper.filter_flattend_meshs(
            self._data.x, self._data.y, self._data.z, limits)
        xprojection = helper.get_projection(x, z)
        yprojection = helper.get_projection(y, z)
        return xprojection, yprojection

    def get_interpolated_quadmesh(self, interpolate, axistype):
        newmeshname = axistype + '_mesh_intp'
        oldmeshname = axistype + '_mesh'
        if interpolate:
            self._sc_map.interpolate_quad_mesh(interpolate)
            x, y, z = getattr(self._sc_map, newmeshname)
        else:
            x, y, z = getattr(self._sc_map, oldmeshname)
        self._data.x = x
        self._data.y = y
        self._data.z = z
        return x, y, z

    def get_interpolated_triangulation(self, interpolate, axistype, switch):
        meshname = axistype + '_mesh'
        self._sc_map.triangulate(meshname=meshname, switch=switch)
        self._sc_map.mask_triangles(meshname=meshname)
        tri_refi, z_refi = self._sc_map.interpolate_triangulation(interpolate)
        self._data.tiang = tri_refi
        self._data.ztiang = z_refi
        # this is important to get the limits
        x, y, z = getattr(self._sc_map, meshname)
        self._data.x = x
        self._data.y = y
        self._data.z = z
        return tri_refi, z_refi

    def get_xy_dy_ratio(self):
        return self._sc_map.dx / self._sc_map.dy

    def get_aspect_ratio(self, axis_type):
        if axis_type['fix_aspect']:
            if axis_type['type'] == 'hkl':
                ratio = self.get_xy_dy_ratio()
                return ratio
            return 1
        return 'auto'

    def get_axis_labels(self, axis_type, crystal_axes, switch=False):
        if crystal_axes:
            return self._sc_map.get_crystal_axis_names()
        hkl1 = self._sc_map.hkl1
        hkl2 = self._sc_map.hkl2
        axis_labels = {
            'tthomega': ['2 Theta (deg)', 'Omega (deg)'],
            'qxqy': ['qx (Ang^-1)', 'qy (Ang^-1)'],
            'hkl': [
                f'{hkl1} (r.l.u.)',
                f'{hkl2} (r.l.u.)'
            ]
        }
        labels = axis_labels[axis_type]
        if switch:
            labels.reverse()
        return labels

    def get_changing_hkl_components(self):
        return self._sc_map.get_changing_hkl_components()

    def get_format_coord(self, axis_type):
        # adds z and hkl label to cursor position
        # this is somehow bad since it backlinks the model function
        # from the view
        def format_coord(x, y):
            h, k, l, z, error = get_hkl_intensity_from_cursor(
                self._sc_map, axis_type, x, y)
            return (f'x={x: 2.4f}, y={y: 2.4f}, hkl=({h: 2.2f},'
                    f'{k: 2.2f},{l: 2.2f}),'
                    f' Intensity={z: 10.2f} ± {error:10.2f}')

        return format_coord

    @staticmethod
    def get_mlimits(*args):
        return [helper.stringrange_to_float(arg) for arg in args]

    @staticmethod
    def get_mzlimit(arg):
        return helper.stringrange_to_float(arg)

    def get_data_zmin_max(self, xlim=None, ylim=None):
        return helper.get_z_min_max(
            self._data.z, xlim, ylim, self._data.x, self._data.y)

    def get_data_xy_lim(self, switch):
        limits = [[min(self._data.x.flatten()), max(self._data.x.flatten())],
                  [min(self._data.y.flatten()), max(self._data.y.flatten())]]
        if switch:
            limits.reverse()
        return limits

    @staticmethod
    def switch_axis(x, y, z, switch):
        if switch:  # switch x and y axes
            nx = np.transpose(y)
            ny = np.transpose(x)
            nz = np.transpose(z)
            return nx, ny, nz
        return x, y, z

    def get_omegaoffset(self):
        return self._sc_map['omegaoffset']

    def get_dx_dy(self):
        return self._sc_map['dx'], self._sc_map['dy']
