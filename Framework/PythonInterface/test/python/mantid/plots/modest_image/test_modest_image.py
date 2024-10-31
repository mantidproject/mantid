# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

import itertools
import unittest

import matplotlib

matplotlib.use("agg")

from matplotlib import pyplot as plt
import matplotlib.image as mi

import numpy as np

from mantid.plots.modest_image import ModestImage

x, y = np.mgrid[0:300, 0:300]
_data = np.sin(x / 10.0) * np.cos(y / 30.0)


def setup_function(func):
    plt.clf()
    plt.cla()


def teardown_function(func):
    plt.close()


def default_data():
    return _data


def init(img_cls, data, origin="upper", extent=None):
    fig = plt.figure()
    ax = fig.add_subplot(111)
    artist = img_cls(ax, data=data, origin=origin, interpolation="nearest", extent=extent)
    ax.add_artist(artist)
    ax.set_aspect("equal")
    artist.norm.vmin = -1
    artist.norm.vmax = 1
    if extent is None:
        ax.set_xlim(0, data.shape[1])
        ax.set_ylim(0, data.shape[0])
    else:
        ax.set_xlim(min(extent[:2]), max(extent[:2]))
        ax.set_ylim(min(extent[2:]), max(extent[2:]))

    return artist


def check(label, modest, axes, thresh=2.0e-5):
    """Assert that images are identical, else save and fail"""
    assert modest.figure is not axes.figure
    assert modest.figure is not axes.figure

    modest.figure.canvas.draw()
    axes.figure.canvas.draw()
    str1 = modest.figure.canvas.tostring_rgb()
    str2 = axes.figure.canvas.tostring_rgb()

    if str1 == str2:
        return

    # convert to array
    shp = modest.figure.canvas.get_width_height()[::-1] + (3,)
    data1 = np.fromstring(str1, dtype=np.uint8, sep="")
    data2 = np.fromstring(str2, dtype=np.uint8, sep="")
    data1.shape = shp
    data2.shape = shp

    # Here we need to convert to float to avoid underflow
    rms = np.mean(np.abs(data1.astype(float) - data2.astype(float)))

    result = "PASS" if rms < thresh else "FAIL"
    modest_label = "test_%s_modest" % label
    axes_label = "test_%s_default" % label

    modest.axes.set_title(modest_label + " " + result)
    modest.axes.set_xlabel("mean abs dev = %0.2e" % rms)
    axes.axes.set_title(axes_label + " " + result)

    axes.figure.canvas.draw()
    modest.figure.canvas.draw()

    if rms >= thresh:
        modest.figure.savefig(modest_label + ".pdf")
        axes.figure.savefig(axes_label + ".pdf")

    assert rms < thresh


def set_bounds(modest, ax, x0, x1, y0, y1):
    modest.axes.set_xlim(x0, x1)
    ax.axes.set_xlim(x0, x1)
    modest.axes.set_ylim(y0, y1)
    ax.axes.set_ylim(y0, y1)


class ModestImageTest(unittest.TestCase):
    def test_default(self):
        """Zoomed out view"""
        data = default_data()
        modest = init(ModestImage, data)
        axim = init(mi.AxesImage, data)
        check("default", modest.axes, axim.axes)

    def test_move(self):
        """move at default zoom"""
        data = default_data()
        modest = init(ModestImage, data)
        axim = init(mi.AxesImage, data)
        xlim = modest.axes.get_xlim()
        delta = 50

        modest.axes.set_xlim(xlim[0] + delta, xlim[1] + delta)
        axim.axes.set_xlim(xlim[0] + delta, xlim[1] + delta)
        check("move", modest.axes, axim.axes)

    def test_zoom(self):
        """zoom in"""
        data = default_data()
        modest = init(ModestImage, data)
        axim = init(mi.AxesImage, data)
        lohi = 200, 250
        modest.axes.set_xlim(lohi)
        axim.axes.set_xlim(lohi)
        modest.axes.set_ylim(lohi)
        axim.axes.set_ylim(lohi)

        check("zoom", modest.axes, axim.axes)

    def test_zoom_out(self):
        """zoom out
        do not get exact match when downsampling. Look for 'similar' images'
        """
        data = default_data()
        modest = init(ModestImage, data)
        axim = init(mi.AxesImage, data)
        lohi = -1000, 1000
        modest.axes.set_xlim(lohi)
        axim.axes.set_xlim(lohi)
        modest.axes.set_ylim(lohi)
        axim.axes.set_ylim(lohi)

        check("zoom_out", modest.axes, axim.axes, thresh=0.4)

    def test_interpolate(self):
        INTRP_METHODS = (
            "nearest",
            "bilinear",
            "bicubic",
            "spline16",
            "spline36",
            "hanning",
            "hamming",
            "hermite",
            "kaiser",
            "quadric",
            "catrom",
            "gaussian",
            "bessel",
            "mitchell",
            "sinc",
            "lanczos",
            "none",
        )

        for method in INTRP_METHODS:
            """change interpolation"""
            data = default_data()
            modest = init(ModestImage, data)
            axim = init(mi.AxesImage, data)

            lohi = 100, 150
            modest.axes.set_xlim(lohi)
            axim.axes.set_xlim(lohi)
            modest.axes.set_ylim(lohi)
            axim.axes.set_ylim(lohi)
            modest.set_interpolation(method)
            axim.set_interpolation(method)
            check("interp_%s" % method, modest.axes, axim.axes)

    def test_scale(self):
        """change color scale"""

        data = default_data()
        modest = init(ModestImage, data)
        axim = init(mi.AxesImage, data)

        for im in [modest, axim]:
            im.norm.vmin = 0.7
            im.norm.vmin = 0.8

        check("cmap", modest.axes, axim.axes)

    def test_unequal_limits(self):
        """Test different x/y scalings"""
        data = default_data()
        modest = init(ModestImage, data)
        axim = init(mi.AxesImage, data)

        for im in [modest, axim]:
            im.axes.set_aspect("auto")
            im.axes.set_xlim(20, 30)
            im.axes.set_ylim(10, 80)

        check("unequal_limits", modest.axes, axim.axes)

    def test_alpha(self):
        """alpha changes"""
        data = default_data()
        modest = init(ModestImage, data)
        axim = init(mi.AxesImage, data)

        for im in [modest, axim]:
            im.set_alpha(0.3)

        check("alpha", modest.axes, axim.axes)

    def test_nan(self):
        """Some nan values"""
        data = default_data()
        data.flat[data.size // 2 :] = np.nan

        modest = init(ModestImage, data)
        axim = init(mi.AxesImage, data)

        for im in [modest, axim]:
            im.set_alpha(0.3)

        check("nan", modest.axes, axim.axes)

    def test_get_array(self):
        """get_array should return full-res data"""
        data = default_data()
        modest = init(ModestImage, data)
        ax = init(mi.AxesImage, data)

        # change axes and redraw, forces modest image resampling
        x0, x1, y0, y1 = 20, 25, 20, 25
        set_bounds(modest, ax, x0, x1, y0, y1)
        ax.axes.figure.canvas.draw()
        modest.axes.figure.canvas.draw()
        np.testing.assert_array_equal(modest.get_array(), ax.get_array())

    def test_get_array_clipped_to_bounds_with_full_image(self):
        """get_array_clipped_to_bounds should return the full data if the axes limits are not set"""
        data = default_data()
        modest = init(ModestImage, data)
        ax = init(mi.AxesImage, data)

        result = modest.get_array_clipped_to_bounds()

        np.testing.assert_array_equal(result, ax.get_array())

    def test_get_array_clipped_to_bounds(self):
        """get_array_clipped_to_bounds should return the data clipped to the axes limits"""
        data = default_data()
        modest = init(ModestImage, data)
        ax = init(mi.AxesImage, data)
        x0, x1, y0, y1 = 20, 25, 30, 35
        set_bounds(modest, ax, x0, x1, y0, y1)

        result = modest.get_array_clipped_to_bounds()

        expected = data[y0 : y1 + 1 : 1, x0 : x1 + 1 : 1]
        np.testing.assert_array_equal(result, expected)

    def test_get_array_clipped_to_bounds_of_1x1_pixels(self):
        """test get_array_clipped_to_bounds with limits of 1 pixel"""
        data = default_data()
        modest = init(ModestImage, data)
        ax = init(mi.AxesImage, data)
        x0, x1, y0, y1 = 20, 21, 30, 31
        set_bounds(modest, ax, x0, x1, y0, y1)

        result = modest.get_array_clipped_to_bounds()

        expected = data[y0 : y1 + 1 : 1, x0 : x1 + 1 : 1]
        np.testing.assert_array_equal(result, expected)

    def test_get_array_clipped_to_bounds_for_out_of_bounds_extents(self):
        """test get_array_clipped_to_bounds with out of bounds extents"""
        data = default_data()
        modest = init(ModestImage, data)
        ax = init(mi.AxesImage, data)
        x0, x1, y0, y1 = 320, 330, 330, 340
        set_bounds(modest, ax, x0, x1, y0, y1)

        result = modest.get_array_clipped_to_bounds()

        # No data returned
        np.testing.assert_array_equal(result.shape, (0, 0))

    def test_get_array_clipped_to_bounds_for_partially_out_of_bounds_value(self):
        """test get_array_clipped_to_bounds with partially out of bounds extents"""
        data = default_data()
        modest = init(ModestImage, data)
        ax = init(mi.AxesImage, data)
        x0, x1, y0, y1 = 290, 310, 295, 315
        set_bounds(modest, ax, x0, x1, y0, y1)

        result = modest.get_array_clipped_to_bounds()

        # The data range up to the axes limits is included
        expected = data[y0:300:1, x0:300:1]
        np.testing.assert_array_equal(result, expected)

    def test_extent(self):
        EXTENT_OPTIONS = itertools.product(["upper", "lower"], [None, [1.0, 7.0, -1.0, 5.0]], ["", "x", "y", "xy"])

        for origin, extent, flip in EXTENT_OPTIONS:
            data = default_data()

            # Use extent= keyword for imshow

            modest = init(ModestImage, data, origin=origin, extent=extent)
            axim = init(mi.AxesImage, data, origin=origin, extent=extent)

            if "x" in flip:
                axim.axes.invert_xaxis()
                modest.axes.invert_xaxis()
            if "y" in flip:
                axim.axes.invert_yaxis()
                modest.axes.invert_yaxis()

            check("extent1_{0}_{1}_{2}".format(origin, extent is not None, flip), modest.axes, axim.axes, thresh=0.0)

            # Try using set_extent

            modest = init(ModestImage, data, origin=origin)
            axim = init(mi.AxesImage, data, origin=origin)

            if extent is not None:
                modest.axes.set_autoscale_on(True)
                axim.axes.set_autoscale_on(True)
            if "x" in flip:
                axim.axes.invert_xaxis()
                modest.axes.invert_xaxis()
            if "y" in flip:
                axim.axes.invert_yaxis()
                modest.axes.invert_yaxis()

            if extent is not None:
                modest.set_extent(extent)
                axim.set_extent(extent)

            check("extent2_{0}_{1}_{2}".format(origin, extent is not None, flip), modest.axes, axim.axes, thresh=0.0)


if __name__ == "__main__":
    unittest.main()
