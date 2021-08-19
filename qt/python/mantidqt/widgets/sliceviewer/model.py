# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#
from enum import Enum
from typing import Dict, List, Sequence, Tuple, Optional

from mantid.api import MatrixWorkspace, MultipleExperimentInfos
from mantid.kernel import SpecialCoordinateSystem
from mantid.plots.datafunctions import get_indices
from mantid.simpleapi import BinMD, IntegrateMDHistoWorkspace, TransposeMD
import numpy as np

from .roi import extract_cuts_matrix, extract_roi_matrix
from .sliceinfo import SliceInfo
from .transform import NonOrthogonalTransform

# Constants
PROJ_MATRIX_LOG_NAME = "W_MATRIX"
LOG_GET_WS_MDE_ALGORITHM_CALLS = False
# min width between data limits (data_min, data_max)
MIN_WIDTH = 1e-5


class WS_TYPE(Enum):
    MDE = 0
    MDH = 1
    MATRIX = 2


class SliceViewerModel:
    """Store the workspace to be plotted. Can be MatrixWorkspace, MDEventWorkspace or MDHistoWorkspace"""

    def __init__(self, ws):
        # reference to the workspace requested to be viewed
        self._ws = ws
        if isinstance(ws, MatrixWorkspace):
            if ws.getNumberHistograms() < 2:
                raise ValueError("workspace must contain at least 2 spectrum")
            if ws.getDimension(0).getNBins() < 2:  # Check number of x bins
                raise ValueError("workspace must contain at least 2 bin")
        elif isinstance(ws, MultipleExperimentInfos):
            if ws.isMDHistoWorkspace():
                if ws.getNumNonIntegratedDims() < 2:
                    raise ValueError("workspace must have at least 2 non-integrated dimensions")
            else:
                if ws.getNumDims() < 2:
                    raise ValueError("workspace must have at least 2 dimensions")
        else:
            raise ValueError("only works for MatrixWorkspace and MDWorkspace")

        wsname = self.get_ws_name()
        self._rebinned_name = wsname + '_svrebinned'
        self._xcut_name, self._ycut_name = wsname + '_cut_x', wsname + '_cut_y'
        self._roi_name = wsname + '_roi'

        ws_type = self.get_ws_type()
        if ws_type == WS_TYPE.MDE:
            self.get_ws = self.get_ws_MDE
            self.get_data = self.get_data_MDE
        else:
            self.get_ws = self._get_ws
            self.get_data = self.get_data_MDH

        if self.get_ws_type() == WS_TYPE.MDE:
            self.export_roi_to_workspace = self.export_roi_to_workspace_mdevent
            self.export_cuts_to_workspace = self.export_cuts_to_workspace_mdevent
            self.export_pixel_cut_to_workspace = self.export_pixel_cut_to_workspace_md
        elif ws_type == WS_TYPE.MDH:
            self.export_roi_to_workspace = self.export_roi_to_workspace_mdhisto
            self.export_cuts_to_workspace = self.export_cuts_to_workspace_mdhisto
            self.export_pixel_cut_to_workspace = self.export_pixel_cut_to_workspace_md
        else:
            self.export_roi_to_workspace = self.export_roi_to_workspace_matrix
            self.export_cuts_to_workspace = self.export_cuts_to_workspace_matrix
            self.export_pixel_cut_to_workspace = self.export_pixel_cut_to_workspace_matrix

    def can_normalize_workspace(self) -> bool:
        if self.get_ws_type() == WS_TYPE.MATRIX and not self._get_ws().isDistribution():
            return True
        return False

    def can_support_nonorthogonal_axes(self) -> bool:
        """
        Query if the workspace can support non-orthogonal axes.
        Workspace must be an MD workspace, in HKL coordinate system
        and have a defined oriented lattice on the first experiment info.
        """
        ws_type = self.get_ws_type()
        if ws_type == WS_TYPE.MDE or ws_type == WS_TYPE.MDH:
            if self.get_frame() != SpecialCoordinateSystem.HKL:
                return False

            try:
                sample = self._get_ws().getExperimentInfo(0).sample()
                if not sample.hasOrientedLattice():
                    return False
            except ValueError:
                return False

            return True
        else:
            return False

    def can_support_peaks_overlays(self) -> bool:
        """
        Check if the given workspace can support peaks overlay
        """
        try:
            if self._get_ws().getNumDims() > 2:
                return True
        except AttributeError:
            pass

        return False

    def can_support_dynamic_rebinning(self) -> bool:
        """
        Check if the given workspace can multiple BinMD calls.
        """
        ws_type = self.get_ws_type()
        return ws_type == WS_TYPE.MDE or (ws_type == WS_TYPE.MDH and self._get_ws().hasOriginalWorkspace(
            0) and self._get_ws().getOriginalWorkspace(0).getNumDims() == self._get_ws().getNumDims())

    def get_ws_name(self) -> str:
        """Return the name of the workspace being viewed"""
        return self._ws.name()

    def get_frame(self) -> SpecialCoordinateSystem:
        """Return the coordinate system of the workspace"""
        return self._ws.getSpecialCoordinateSystem()

    def get_title(self, ws_name=None) -> str:
        """Return a title for the model, given a workspace name. Default to the name
        of the model's workspace if none supplied.
        """
        if not ws_name:
            ws_name = self.get_ws_name()
        return f'Sliceviewer - {ws_name}'

    def get_ws_MDE(self,
                   slicepoint: Sequence[Optional[float]],
                   bin_params: Optional[Sequence[float]],
                   limits: Optional[tuple] = None,
                   dimension_indices: Optional[tuple] = None):
        """
        :param slicepoint: ND sequence of either None or float. A float defines the point
                        in that dimension for the slice.
        :param bin_params: ND sequence containing the number of bins for each dimension or None to use
                           the existing data
        :param limits: An optional 2-tuple sequence containing limits for plotting dimensions. If
                       not provided the full extent of each dimension is used
        """
        workspace = self._get_ws()

        params, _, __ = _roi_binmd_parameters(workspace, slicepoint, bin_params, limits, dimension_indices)
        params['EnableLogging'] = LOG_GET_WS_MDE_ALGORITHM_CALLS
        binned = BinMD(InputWorkspace=workspace, OutputWorkspace=self._rebinned_name, **params)
        return binned

    def get_data_MDH(self, slicepoint, transpose=False):
        indices, _ = get_indices(self.get_ws(), slicepoint=slicepoint)
        if transpose:
            return np.ma.masked_invalid(self.get_ws().getSignalArray()[indices]).T
        else:
            return np.ma.masked_invalid(self.get_ws().getSignalArray()[indices])

    def get_data_MDE(self, slicepoint, bin_params, dimension_indices, limits=None, transpose=False):
        """
        :param slicepoint: ND sequence of either None or float. A float defines the point
                           in that dimension for the slice.
        :param bin_params: ND sequence containing the number of bins for each dimension
        :param limits: An optional ND sequence containing limits for plotting dimensions. If
                       not provided the full extent of each dimension is used. The limits
                       should be provided in the order of the workspace not the display
        :param transpose: If true then transpose the data before returning
        """
        if transpose:
            return np.ma.masked_invalid(
                self.get_ws_MDE(slicepoint, bin_params, limits, dimension_indices).getSignalArray().squeeze()).T
        else:
            return np.ma.masked_invalid(
                self.get_ws_MDE(slicepoint, bin_params, limits, dimension_indices).getSignalArray().squeeze())

    def get_dim_limits(self, slicepoint, transpose):
        """
        Return a xlim, ylim) for the display dimensions where xlim, ylim are tuples
        :param slicepoint: Sequence containing either a float or None where None indicates a display dimension
        :param transpose: A boolean flag indicating if the display dimensions are transposed
        """
        xindex, yindex = _display_indices(slicepoint, transpose)
        workspace = self._get_ws()
        xdim, ydim = workspace.getDimension(xindex), workspace.getDimension(yindex)
        return (xdim.getMinimum(), xdim.getMaximum()), (ydim.getMinimum(), ydim.getMaximum())

    def is_ragged_matrix_plotted(self):
        """
        :return: bool for if workspace is matrix workspace with non common bins
        """
        return self.get_ws_type() == WS_TYPE.MATRIX and not self._get_ws().isCommonBins()

    def get_dim_info(self, n: int) -> dict:
        """
        returns dict of (minimum :float, maximum :float, number_of_bins :int,
                         width :float, name :str, units :str, type :str, can_rebin: bool, qdim: bool) for dimension n
        """
        workspace = self._get_ws()
        dim = workspace.getDimension(n)
        return {
            'minimum': dim.getMinimum(),
            'maximum': dim.getMaximum(),
            'number_of_bins': dim.getNBins(),
            'width': dim.getBinWidth(),
            'name': dim.name,
            'units': dim.getUnits(),
            'type': self.get_ws_type().name,
            'can_rebin': self.can_support_dynamic_rebinning(),
            'qdim': dim.getMDFrame().isQ()
        }

    def get_dimensions_info(self) -> List[Dict]:
        """
        returns a list of dict for each dimension containing dim_info
        """
        return [self.get_dim_info(n) for n in range(self._get_ws().getNumDims())]

    def get_ws_type(self) -> WS_TYPE:
        if isinstance(self._get_ws(), MatrixWorkspace):
            return WS_TYPE.MATRIX
        elif isinstance(self._get_ws(), MultipleExperimentInfos):
            if self._get_ws().isMDHistoWorkspace():
                return WS_TYPE.MDH
            else:
                return WS_TYPE.MDE
        else:
            raise ValueError("Unsupported workspace type")

    def get_properties(self):
        """
        @return: a dictionary of properties about this model to compare new models against,
        for example when the model workspace changes outside of the slice viewer.
        """
        return {
            "workspace_type": self.get_ws_type(),
            "supports_normalise": self.can_normalize_workspace(),
            "supports_nonorthogonal_axes": self.can_support_nonorthogonal_axes(),
            "supports_dynamic_rebinning": self.can_support_dynamic_rebinning(),
            "supports_peaks_overlays": self.can_support_peaks_overlays()
        }

    def create_nonorthogonal_transform(self, slice_info: SliceInfo):
        """
        Calculate the transform object for nonorthogonal axes.
        :param slice_info: SliceInfoType object giving information on the slice
        :raises: RuntimeError if model does not support nonorthogonal axes
        """
        if not self.can_support_nonorthogonal_axes():
            raise RuntimeError("Workspace cannot support non-orthogonal axes view")

        expt_info = self._get_ws().getExperimentInfo(0)
        lattice = expt_info.sample().getOrientedLattice()
        try:
            proj_matrix = np.array(expt_info.run().get(PROJ_MATRIX_LOG_NAME).value, dtype=float)
            proj_matrix = proj_matrix.reshape(3, 3)
        except (AttributeError, KeyError):  # run can be None so no .get()
            # assume orthogonal projection
            proj_matrix = np.eye(3)
        display_indices = list(range(0, proj_matrix.shape[0]))
        display_indices.pop(slice_info.z_index)
        return NonOrthogonalTransform.from_lattice(lattice,
                                                   x_proj=proj_matrix[:, display_indices[0]],
                                                   y_proj=proj_matrix[:, display_indices[1]])

    def export_roi_to_workspace_mdevent(self, slicepoint: Sequence[Optional[float]],
                                        bin_params: Sequence[float], limits: tuple,
                                        transpose: bool, dimension_indices: Sequence[int]):
        """
        Export 2D roi to a separate workspace.
        :param slicepoint: ND sequence of either None or float. A float defines the point
                           in that dimension for the slice.
        :param bin_params: ND sequence containing the number of bins for each dimension
        :param limits: An optional ND sequence containing limits for plotting dimensions. If
                       not provided the full extent of each dimension is used
        :param dimension_indices: A list where the value (None, 0, or 1) at index i denotes the index of the axis
                                  dimension i is displayed on
        :param transpose: If true the limits are transposed w.r.t the data
        """
        workspace = self._get_ws()
        params, xindex, yindex = _roi_binmd_parameters(workspace, slicepoint, bin_params, limits, dimension_indices)
        params['OutputWorkspace'] = self._roi_name
        roi_ws = BinMD(InputWorkspace=self._get_ws(), **params)
        roi_ws.clearOriginalWorkspaces()
        if transpose:
            _inplace_transposemd(self._roi_name, axes=[yindex, xindex])

        return f'ROI created: {self._roi_name}'

    def export_cuts_to_workspace_mdevent(self, slicepoint: Sequence[Optional[float]],
                                         bin_params: Sequence[float], limits: tuple,
                                         transpose: bool, dimension_indices: Sequence[int], cut: str):
        """
        Export 1D cuts in the X/Y direction for the extent.
        :param slicepoint: ND sequence of either None or float. A float defines the point
                           in that dimension for the slice.
        :param bin_params: ND sequence containing the number of bins for each dimension.
        :param limits: An optional ND sequence containing limits for plotting dimensions. If
                       not provided the full extent of each dimension is used
        :param transpose: If true then the limits are transposed w.r.t to the data
        :param cut: A string denoting which type to export. Options=s,c,x,y.
        """
        workspace = self._get_ws()
        xcut_name, ycut_name, help_msg = self._cut_names(cut)
        params, xindex, yindex = _roi_binmd_parameters(workspace, slicepoint, bin_params, limits, dimension_indices)
        output_bins = params['OutputBins']
        xbins, ybins = output_bins[xindex], output_bins[yindex]
        if transpose:
            xindex, yindex = yindex, xindex
            xbins, ybins = ybins, xbins

        if xcut_name:
            params['OutputWorkspace'] = xcut_name
            output_bins[xindex] = xbins
            output_bins[yindex] = 1
            params['OutputBins'] = output_bins
            BinMD(InputWorkspace=self._get_ws(), **params)
            _keep_dimensions(xcut_name, xindex)
        if ycut_name:
            params['OutputWorkspace'] = ycut_name
            output_bins[xindex] = 1
            output_bins[yindex] = ybins
            params['OutputBins'] = output_bins
            BinMD(InputWorkspace=self._get_ws(), **params)
            _keep_dimensions(ycut_name, yindex)

        return help_msg

    def export_roi_to_workspace_mdhisto(self, slicepoint: Sequence[Optional[float]],
                                        bin_params: Sequence[float], limits: tuple,
                                        transpose: bool, dimension_indices: Sequence[int]):
        """
        Export 2D ROI to a workspace.
        :param slicepoint: ND sequence of either None or float. A float defines the point
                           in that dimension for the slice.
        :param bin_params: ND sequence containing the number of bins for each dimension. Can be None for HistoWorkspaces
        :param limits: An optional ND sequence containing limits for plotting dimensions. If
                       not provided the full extent of each dimension is used
        :param transpose: If true then the limits are transposed w.r.t to the data
        """
        workspace = self._get_ws()
        if transpose:
            # swap back to model order
            limits = limits[1], limits[0]
        xindex, yindex = _display_indices(slicepoint)
        dim_limits = _dimension_limits(workspace, slicepoint, limits)
        # Construct parameters to integrate everything first and override per cut
        params = {f'P{n + 1}Bin': [*dim_limits[n]] for n in range(workspace.getNumDims())}

        xdim_min, xdim_max = dim_limits[xindex]
        ydim_min, ydim_max = dim_limits[yindex]
        params['OutputWorkspace'] = self._roi_name
        params[f'P{xindex + 1}Bin'] = [xdim_min, 0, xdim_max]
        params[f'P{yindex + 1}Bin'] = [ydim_min, 0, ydim_max]
        IntegrateMDHistoWorkspace(InputWorkspace=workspace, **params)
        if transpose:
            _inplace_transposemd(self._roi_name, axes=[yindex, xindex])

        return f'ROI created: {self._roi_name}'

    def export_cuts_to_workspace_mdhisto(self, slicepoint: Sequence[Optional[float]],
                                         bin_params: Sequence[float], limits: tuple,
                                         transpose: bool,  dimension_indices: Sequence[int], cut: str):
        """
        Export 1D cuts in the X/Y direction for the extent.
        :param slicepoint: ND sequence of either None or float. A float defines the point
                           in that dimension for the slice.
        :param bin_params: ND sequence containing the number of bins for each dimension. Can be None for HistoWorkspaces
        :param limits: An optional ND sequence containing limits for plotting dimensions. If
                       not provided the full extent of each dimension is used
        :param transpose: If true then the limits are transposed w.r.t to the data
        :param cut: A string denoting which type to export. Options=c,x,y.
        """
        workspace = self._get_ws()
        if transpose:
            # swap back to model order
            limits = limits[1], limits[0]
        dim_limits = _dimension_limits(workspace, slicepoint, limits)
        # Construct paramters to integrate everything first and overrid per cut
        params = {f'P{n + 1}Bin': [*dim_limits[n]] for n in range(workspace.getNumDims())}
        xindex, yindex = _display_indices(slicepoint, transpose)

        xcut_name, ycut_name, help_msg = self._cut_names(cut)
        xdim_min, xdim_max = dim_limits[xindex]
        ydim_min, ydim_max = dim_limits[yindex]
        if xcut_name:
            params['OutputWorkspace'] = xcut_name
            params[f'P{xindex + 1}Bin'] = [xdim_min, 0, xdim_max]
            IntegrateMDHistoWorkspace(InputWorkspace=workspace, **params)
            _keep_dimensions(xcut_name, xindex)
        if ycut_name:
            params['OutputWorkspace'] = ycut_name
            params[f'P{xindex + 1}Bin'] = [xdim_min, xdim_max]
            params[f'P{yindex + 1}Bin'] = [ydim_min, 0, ydim_max]
            IntegrateMDHistoWorkspace(InputWorkspace=workspace, **params)
            _keep_dimensions(ycut_name, yindex)

        return help_msg

    def export_cuts_to_workspace_matrix(self, slicepoint, bin_params, limits: tuple,
                                        transpose: bool,  dimension_indices: Sequence[int], cut: str):
        """
        Export 1D cuts in the X/Y direction for the extent. Signature matches other export functions.
        slicepoint, bin_params are unused
        :param limits: An optional ND sequence containing limits for plotting dimensions. If
                       not provided the full extent of each dimension is used
        :param transpose: If true then the limits are transposed .w.r.t. the data
        :param cut: A string denoting which cut to export. Options=c,x,y.
        """
        workspace = self._get_ws()
        if transpose:
            # swap back to model order
            limits = limits[1], limits[0]
        yaxis = workspace.getAxis(1)
        (xmin, xmax), (ymin, ymax) = limits[0], limits[1]
        xcut_name, ycut_name, help_msg = self._cut_names(cut)
        if transpose:
            xcut_name, ycut_name = ycut_name, xcut_name

        if yaxis.isSpectra() or yaxis.isNumeric():
            extract_cuts_matrix(workspace, xmin, xmax, ymin, ymax, xcut_name, ycut_name)
        else:
            help_msg = 'Unknown Y axis type. Unable to perform cuts'

        return help_msg

    def export_roi_to_workspace_matrix(self, slicepoint, bin_params, limits: tuple,
                                       transpose: bool,  dimension_indices: Sequence[int]):
        """
        Export 2D region as a workspace. Signature matches other export functions
        slicepoint, bin_params are unused
        :param limits: An ND sequence containing limits for plotting dimensions. If
                       not provided the full extent of each dimension is used
        :param transpose:  If true then the limits are transposed .w.r.t. the data
        """
        if transpose:
            # swap back to model order
            limits = limits[1], limits[0]
        extract_roi_matrix(self._get_ws(), *limits[0], *limits[1], transpose, self._roi_name)
        return f'ROI created: {self._roi_name}'

    def export_pixel_cut_to_workspace_md(self, slicepoint, bin_params, pos: tuple, transpose: bool,
                                         axis: str):
        """
        Export a single row/column as a workspace. Signature matches other export functions
        slicepoint, bin_params are unused
        :param pos: A 2-tuple containing the position of the pixel whose row/column should be exported
        :param transpose:  If true then the limits are transposed .w.r.t. the data
        :param axis: A string 'x' or 'y' identifying the axis to cut along
        """
        # Form single pixel limits for a cut
        xindex, yindex = _display_indices(slicepoint, transpose)
        workspace = self._get_ws()
        deltax, deltay = workspace.getDimension(xindex).getBinWidth(), \
            workspace.getDimension(yindex).getBinWidth()
        xpos, ypos = pos
        limits = (xpos - 0.5 * deltax, xpos + 0.5 * deltax), (ypos - 0.5 * deltay,
                                                              ypos + 0.5 * deltay)
        return self.export_cuts_to_workspace_mdevent(slicepoint, bin_params, limits, transpose,
                                                     axis)

    def export_pixel_cut_to_workspace_matrix(self, slicepoint, bin_params, pos: tuple,
                                             transpose: bool, axis: str):
        """
        Export a single row/column as a workspace. Signature matches other export functions
        slicepoint, bin_params are unused
        :param pos: A 2-tuple containing the position of the pixel whose row/column should be exported
        :param transpose:  If true then the limits are transposed .w.r.t. the data
        :param axis: A string 'x' or 'y' identifying the axis to cut along
        """
        workspace = self._get_ws()
        xpos, ypos = pos
        if transpose:
            # swap back to model order
            xpos, ypos = ypos, xpos

        xcut_name, ycut_name, help_msg = self._cut_names(axis)
        if transpose:
            xcut_name, ycut_name = ycut_name, xcut_name
        if xcut_name:
            extract_roi_matrix(workspace, None, None, ypos, ypos, False, xcut_name)
        if ycut_name:
            extract_roi_matrix(workspace, xpos, xpos, None, None, True, ycut_name)

        return help_msg

    def workspace_equals(self, ws_name):
        return str(self._get_ws()) == ws_name

    # private api
    def _get_ws(self):
        return self._ws

    def _cut_names(self, cut: str):
        """
        Return 3-tuple of xcut_name,ycut_name,status message depending on the cut type request.
        :param cut: Single letter denoting the cut type: x, y, c
        :raises RuntimeError: if an unkown cut type is requested
        """
        xcut_name, ycut_name = self._xcut_name, self._ycut_name
        if cut == 'x':
            ycut_name = ''
            help_msg = f'Cut along X created: {xcut_name}'
        elif cut == 'y':
            xcut_name = ''
            help_msg = f'Cut along Y created: {ycut_name}'
        elif cut == 'c':
            help_msg = f'Cuts along X/Y created: {xcut_name} & {ycut_name}'
        else:
            raise RuntimeError(f'Unknown cut requested {cut}')

        return xcut_name, ycut_name, help_msg


# private functions
def _roi_binmd_parameters(workspace, slicepoint: Sequence[Optional[float]],
                          bin_params: Optional[Sequence[float]],
                          limits: tuple,
                          dimension_indices: tuple) -> Tuple[dict, int, int]:
    """
    Return a sequence of 2-tuples defining the limits for MDEventWorkspace binning
    :param workspace: MDEventWorkspace that is to be binned
    :param slicepoint: ND sequence of either None or float. A float defines the point
                    in that dimension for the slice.
    :param bin_params: A list of binning paramaters for each dimension or thickness for slicing dimensions
    :param limits: An optional Sequence of length 2 containing limits for plotting dimensions. If
                    not provided the full extent of each dimension is used.
    :return: 3-tuple (binmd parameters, index of X dimension, index of Y dimension)
    """
    xindex, yindex = _display_indices(slicepoint)
    dim_limits = _dimension_limits(workspace, dimension_indices, limits)
    ndims = workspace.getNumDims()
    ws_basis = np.eye(ndims)
    output_extents, output_bins = [], []
    params = {'AxisAligned': False}
    if workspace.getSpecialCoordinateSystem() == SpecialCoordinateSystem.HKL:
        params['NormalizeBasisVectors'] = False  # Default is True
    for n in range(ndims):
        dimension = workspace.getDimension(n)
        basis_vec_n = _to_str(ws_basis[:, n])
        slice_pt = slicepoint[n]
        nbins = bin_params[n] if bin_params is not None else dimension.getNBins()
        if slice_pt is None:
            dim_min, dim_max = dim_limits[n]
        else:
            dim_min, dim_max = slice_pt - nbins / 2, slice_pt + nbins / 2
            nbins = 1
        if dim_max - dim_min < MIN_WIDTH:
            dim_max = dim_min + MIN_WIDTH
        params[f'BasisVector{n}'] = f'{dimension.name},{dimension.getUnits()},{basis_vec_n}'
        output_extents.append(dim_min)
        output_extents.append(dim_max)
        output_bins.append(nbins)
    params['OutputExtents'] = output_extents
    params['OutputBins'] = output_bins

    return params, xindex, yindex


def _dimension_limits(workspace,
                      dimension_indices: Optional[tuple],
                      limits: Optional[Sequence[tuple]] = None) -> Sequence[tuple]:
    """
    Return a sequence of 2-tuples defining the limits for MDEventWorkspace binning
    :param workspace: MDEventWorkspace that is to be binned
    :param slicepoint: ND sequence of either None or float. A float defines the point
                    in that dimension for the slice.
    :param limits: An optional Sequence of length 2 containing limits for plotting dimensions. If
                    not provided the full extent of each dimension is used.
    """
    dim_limits = [(dim.getMinimum(), dim.getMaximum())
                  for dim in [workspace.getDimension(i) for i in range(workspace.getNumDims())]]
    if limits is not None:
        # Match the view limits to the dimension they're for.
        for dim, axis in enumerate(dimension_indices):
            if axis is not None:
                dim_limits[dim] = limits[axis]

    return dim_limits


def _dimension_bins(workspace) -> Sequence[float]:
    """
    Given a workspace return a sequence of float containing the number of bins
    in each dimension
    :param workspace: A reference to an MDHistoWorkspace
    :return: A sequence of floats
    """
    return [workspace.getDimension(i).getNBins() for i in range(workspace.getNumDims())]


def _display_indices(slicepoint: Sequence[Optional[float]], transpose: bool = False):
    """
    Given a slicepoint sequence return the indices of the display
    dimensions.
    :param slicepoint: ND sequence of either None or float. A float defines the point
                    in that dimension for the slice.
    :param transpose: If True then swap the indices before return
    """
    xindex = slicepoint.index(None)
    yindex = slicepoint.index(None, xindex + 1)
    if transpose:
        return yindex, xindex
    else:
        return xindex, yindex


def _keep_dimensions(workspace, index):
    """
    Drop other dimensions than that specified
    :param workspace: A reference to an MD workspace
    :param index: The index of the only dimension to keep in the workspace
    """
    # Transpose is able to do this for us
    _inplace_transposemd(workspace, axes=[index])


def _inplace_transposemd(workspace, axes):
    """Transpose a workspace inplace
    :param workspace: A reference to an MD workspace
    :param axes: The axes parameter for TransposeMD
    """
    TransposeMD(InputWorkspace=workspace, OutputWorkspace=workspace, Axes=axes)


def _to_str(seq: Sequence):
    """Given a sequence turn it into a comma-separate string of each element
    """
    return ','.join(map(str, seq))
