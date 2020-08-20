# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import matplotlib.image as mimage
import matplotlib.colors
import numpy as np

from mantid.plots.datafunctions import get_matrix_2d_ragged, get_normalize_by_bin_width

class SamplingImage(mimage.AxesImage):
    def __init__(self,
                 ax,
                 workspace,
                 transpose=False,
                 cmap=None,
                 norm=None,
                 interpolation=None,
                 origin=None,
                 extent=None,
                 filternorm=1,
                 filterrad=4.0,
                 resample=False,
                 normalization=None,
                 **kwargs):
        super().__init__(ax,
                         cmap=cmap,
                         norm=norm,
                         interpolation=interpolation,
                         origin=origin,
                         extent=extent,
                         filternorm=filternorm,
                         filterrad=filterrad,
                         resample=resample,
                         **kwargs)
        self.ws = workspace
        try:
            self.spectrum_info = workspace.spectrumInfo()
        except Exception:
            self.spectrum_info = None
        self.transpose = transpose
        self.normalization = normalization
        self._resize_cid, self._xlim_cid, self._ylim_cid = None, None, None
        self._resample_required = True
        self._full_extent = extent
        self._xbins, self._ybins = 100, 100

    def connect_events(self):
        axes = self.axes
        self._resize_cid = axes.get_figure().canvas.mpl_connect('resize_event', self._resize)
        self._xlim_cid = axes.callbacks.connect('xlim_changed', self._xlim_changed)
        self._ylim_cid = axes.callbacks.connect('ylim_changed', self._ylim_changed)

    def disconnect_events(self):
        axes = self.axes
        axes.get_figure().canvas.mpl_disconnect(self._resize_cid)
        axes.callbacks.disconnect(self._xlim_cid)
        axes.callbacks.disconnect(self._ylim_cid)

    def draw(self, renderer, *args, **kwargs):
        if self._resample_required:
            self._resample_image()
            self._resample_required = False

        super().draw(renderer, *args, **kwargs)

    def remove(self):
        self.disconnect_events()
        super().remove()

    def _xlim_changed(self, ax):
        if self._update_extent():
            self._resample_required = True

    def _ylim_changed(self, ax):
        if self._update_extent():
            self._resample_required = True

    def _resize(self, canvas):
        xbins, ybins = self._calculate_bins_from_extent()
        if  xbins > self._xbins or ybins > self._ybins:
            self._resample_required = True

    def _calculate_bins_from_extent(self):
        bbox = self.get_window_extent().transformed(
            self.axes.get_figure().dpi_scale_trans.inverted())
        dpi = self.axes.get_figure().dpi
        xbins = int(np.ceil(bbox.width * dpi))
        ybins = int(np.ceil(bbox.height * dpi))
        return xbins, ybins

    def _resample_image(self, xbins=None, ybins=None):
        if self._resample_required:
            extent = self.get_extent()

            if xbins is None or ybins is None:
                xbins, ybins = self._calculate_bins_from_extent()

            x, y, data = get_matrix_2d_ragged(self.ws, self.normalization, histogram2D=True, transpose=self.transpose,
                                              extent=extent, xbins=xbins, ybins=ybins, spec_info=self.spectrum_info)
            self.set_data(data)
            self._xbins = xbins
            self._ybins = ybins

    def _update_extent(self):
        """
        Update the extent base on xlim and ylim, should be called after pan or zoom action,
        this limits the range that the data will be sampled. Return True or False if extents have changed.
        """
        new_extent = self.axes.get_xlim() + self.axes.get_ylim()
        if new_extent != self.get_extent():
            self.set_extent(new_extent)
            return True
        else:
            return False

    def get_full_extent(self):
        return self._full_extent


def imshow_sampling(axes,
                    workspace,
                    cmap=None,
                    norm=None,
                    aspect=None,
                    interpolation=None,
                    alpha=None,
                    vmin=None,
                    vmax=None,
                    origin=None,
                    extent=None,
                    shape=None,
                    filternorm=1,
                    filterrad=4.0,
                    imlim=None,
                    resample=None,
                    url=None,
                    **kwargs):
    """Copy of imshow but replaced AxesImage with SamplingImage and added
    callbacks and Mantid Workspace stuff.

    See :meth:`matplotlib.axes.Axes.imshow`

    To test:

    from mantidqt.widgets.sliceviewer.samplingimage import imshow_sampling
    fig, ax = plt.subplots()
    im = imshow_sampling(ax, workspace, aspect='auto', origin='lower')
    fig.show()
    """
    transpose = kwargs.pop('transpose', False)
    normalization, _ = get_normalize_by_bin_width(workspace, axes, **kwargs)
    kwargs.pop('distribution', None)

    if not extent:
        x0, x1, y0, y1 = (workspace.getDimension(0).getMinimum(), workspace.getDimension(0).getMaximum(),
                          workspace.getDimension(1).getMinimum() , workspace.getDimension(1).getMaximum())
        if workspace.getDimension(1).getNBins() == workspace.getAxis(1).length():
            width = workspace.getDimension(1).getBinWidth()
            y0 -= width/2
            y1 += width/2
        extent = (x0, x1, y0, y1)

        if transpose:
            e1, e2, e3, e4 = extent
            extent = e3, e4, e1, e2

    # from matplotlib.axes.Axes.imshow
    if norm is not None and not isinstance(norm, matplotlib.colors.Normalize):
        raise ValueError("'norm' must be an instance of 'mcolors.Normalize'")
    if aspect is None:
        aspect = matplotlib.rcParams['image.aspect']
    axes.set_aspect(aspect)
    im = SamplingImage(axes,
                       workspace,
                       transpose,
                       cmap,
                       norm,
                       interpolation,
                       origin,
                       extent,
                       filternorm=filternorm,
                       filterrad=filterrad,
                       resample=resample,
                       normalization=normalization,
                       **kwargs)
    im._resample_image(100, 100)

    im.set_alpha(alpha)
    im.set_url(url)
    if im.get_clip_path() is None:
        # image does not already have clipping set, clip to axes patch
        im.set_clip_path(axes.patch)
    if vmin is not None or vmax is not None:
        if norm is not None and isinstance(norm, matplotlib.colors.LogNorm):
            if vmin <= 0:
                vmin = 0.0001
            if vmax <= 0:
                vmax = 1
        im.set_clim(vmin, vmax)
    else:
        im.autoscale_None()

    # update ax.dataLim, and, if autoscaling, set viewLim
    # to tightly fit the image, regardless of dataLim.
    im.set_extent(im.get_extent())

    axes.add_image(im)
    if extent:
        axes.set_xlim(extent[0], extent[1])
        axes.set_ylim(extent[2], extent[3])

    im.connect_events()
    return im
