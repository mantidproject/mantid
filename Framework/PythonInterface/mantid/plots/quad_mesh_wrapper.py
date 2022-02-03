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

    def get_array_clipped_to_bounds(self):
        """
        Get the color data for the subset of the array within the axes limits. This is necessary for autoscaling of the
        clim to work when plotting MDHisto workspaces in nonorthognonal view in sliceviewer
        :return: masked array of color data clipped to axes limits
        """
        xlim, ylim = self.axes.get_xlim(), self.axes.get_ylim()

        # reshape array
        nbins0, nbins1 = self._mesh._coordinates.shape[0:2]
        arr = self._mesh.get_array().reshape((nbins0 - 1, nbins1 - 1))  # ny, nx

        # y-axis is parallel to axes box so can determine ylimit as so
        ydata = self._mesh._coordinates[:-1, 0, 1]
        iy = np.logical_and(ydata >= ylim[0], ydata < ylim[1])
        xdata = self._mesh._coordinates[:-1, :-1, 0][iy, :]  # unlike y need whole 2D array
        ix = np.logical_and(xdata >= xlim[0], xdata < xlim[1])

        return safe_masked_invalid(arr[iy, :][ix])
