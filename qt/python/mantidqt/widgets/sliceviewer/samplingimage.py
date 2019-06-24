import matplotlib.image as mimage
import matplotlib.colors
from mantid.plots.plotfunctions import _setLabels2D
import mantid.api
import numpy as np


class SamplingImage(mimage.AxesImage):
    def __init__(self, ax, workspace,
                 transpose=False,
                 cmap=None,
                 norm=None,
                 interpolation=None,
                 origin=None,
                 extent=None,
                 filternorm=1,
                 filterrad=4.0,
                 resample=False,
                 **kwargs):
        super(SamplingImage, self).__init__(
            ax,
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
        self.transpose = transpose

    def _xlim_changed(self, ax):
        if self._update_extent():
            self._resample_image()

    def _ylim_changed(self, ax):
        if self._update_extent():
            self._resample_image()

    def _resize(self, canvas):
        self._resample_image()

    def _resample_image(self, xbins=None, ybins=None):
        extent = self.get_extent()
        if xbins is None or ybins is None:
            bbox=self.get_window_extent().transformed(self.axes.get_figure().dpi_scale_trans.inverted())
            dpi = self.axes.get_figure().dpi
            xbins = np.ceil(bbox.width*dpi)
            ybins = np.ceil(bbox.height*dpi)
        x=np.linspace(extent[0], extent[1], int(xbins))
        y=np.linspace(extent[2], extent[3], int(ybins))
        X,Y = np.meshgrid(x,y)
        if self.transpose:
            xy = np.vstack((Y.ravel(),X.ravel())).T
        else:
            xy = np.vstack((X.ravel(),Y.ravel())).T
        data = self.ws.getSignalAtCoord(xy, mantid.api.MDNormalization.NoNormalization).reshape(X.shape)
        self.set_data(data)

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


def imshow_sampling(axes, workspace, cmap=None, norm=None, aspect=None,
                    interpolation=None, alpha=None, vmin=None, vmax=None,
                    origin=None, extent=None, shape=None, filternorm=1,
                    filterrad=4.0, imlim=None, resample=None, url=None, **kwargs):
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

    _setLabels2D(axes, workspace, transpose=transpose, xscale='linear')
    if not extent:
        extent = (workspace.getDimension(0).getMinimum(),
                  workspace.getDimension(0).getMaximum(),
                  workspace.getDimension(1).getMinimum(),
                  workspace.getDimension(1).getMaximum())
    if transpose:
        e1, e2, e3, e4 = extent
        extent = e3, e4, e1, e2

    # from matplotlib.axes.Axes.imshow
    if norm is not None and not isinstance(norm, matplotlib.colors.Normalize):
        raise ValueError(
            "'norm' must be an instance of 'mcolors.Normalize'")
    if aspect is None:
        aspect = matplotlib.rcParams['image.aspect']
    axes.set_aspect(aspect)
    im = SamplingImage(axes, workspace, transpose, cmap, norm, interpolation, origin, extent,
                       filternorm=filternorm, filterrad=filterrad,
                       resample=resample, **kwargs)

    im.set_extent(im.get_extent())
    im._resample_image(100, 100)
    im.set_alpha(alpha)
    if im.get_clip_path() is None:
        # image does not already have clipping set, clip to axes patch
        im.set_clip_path(axes.patch)
    if vmin is not None or vmax is not None:
        im.set_clim(vmin, vmax)
    else:
        im.autoscale_None()
    im.set_url(url)

    # update ax.dataLim, and, if autoscaling, set viewLim
    # to tightly fit the image, regardless of dataLim.
    im.set_extent(im.get_extent())

    axes.add_image(im)

    axes.get_figure().canvas.mpl_connect('resize_event', im._resize)
    axes.callbacks.connect('xlim_changed', im._xlim_changed)
    axes.callbacks.connect('ylim_changed', im._ylim_changed)

    return im
