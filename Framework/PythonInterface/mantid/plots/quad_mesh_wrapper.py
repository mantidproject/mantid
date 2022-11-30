import numpy as np
from matplotlib.collections import QuadMesh
from matplotlib.cbook import safe_masked_invalid


class QuadMeshWrapper:
    """
    Class that wraps the QuadMesh object (overriding getattr) but doesn't inherit as can't init QuadMesh.
    Autoscaling of clim requires image object (in this case QuadMesh output from pcolormesh) to have method
    get_array_clipped_to_bounds (as in SamplingImage and ModestImage).
    """

    def __init__(self, mesh: QuadMesh):
        self._mesh = mesh

    def __getattr__(self, item):
        return getattr(self._mesh, item)

    def get_array_clipped_to_bounds(self) -> np.ndarray:
        """
        Get the color data for the subset of the array within the axes limits. This is necessary for autoscaling of the
        clim to work when plotting MDHisto workspaces in nonorthognonal view in sliceviewer
        :return: masked array of color data clipped to axes limits (will be empty container if no data within limits)
        """
        xlim, ylim = self.axes.get_xlim(), self.axes.get_ylim()  # limits for data transformed into orthogonal basis

        # reshape color data array
        nedges_y, nedges_x = self._mesh._coordinates.shape[0:2]
        arr = self._mesh.get_array().reshape((nedges_y - 1, nedges_x - 1))

        # y-axis is parallel to axes box so can determine ylimit as so
        ydata = self._mesh._coordinates[:-1, 0, 1]  # bin edges - excl. last edge as pcolor puts bin center bottom left
        dy = ydata[1] - ydata[0]  # ybin width
        iy_rows = np.logical_and(ydata >= ylim[0] - dy, ydata < ylim[1])  # include bins to bottom left of axes limits
        if any(iy_rows):
            # unlike ydata need whole 2D array for xdata as x-extent of data (in orthogonal basis) depends on y value
            xdata = self._mesh._coordinates[:-1, :-1, 0][iy_rows, :]
            dx = xdata[0][1] - xdata[0][0]  # xbin width
            xmask = np.logical_and(xdata >= xlim[0] - dx, xdata < xlim[1])
            return safe_masked_invalid(arr[iy_rows, :][xmask])
        else:
            return np.array([], dtype=float)
