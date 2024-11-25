# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

import unittest
import numpy as np
import matplotlib

matplotlib.use("agg")
import matplotlib.pyplot as plt

from mantid.plots.modest_image import ModestImage, imshow


def default_data():
    x, y = np.mgrid[0:100, 0:100]
    return np.sin(x / 25) * np.cos(y / 50)


def setup_function(func):
    plt.clf()
    plt.cla()


def teardown_function(func):
    plt.close()


class ImshowTest(unittest.TestCase):
    def check_props(self, obj1, obj2, props):
        for p in props:
            v1 = obj1.__getattribute__("get_%s" % p)()
            v2 = obj2.__getattribute__("get_%s" % p)()
            self.assertEqual(v1, v2)

    def check_axes_props(self, ax1, ax2):
        props = ["aspect"]
        self.check_props(ax1, ax2, props)

    def check_artist_props(self, art1, art2):
        """Assert that properties of two artists are equal"""

        props = ["alpha", "clim", "clip_on", "clip_path", "interpolation", "rasterized", "resample", "snap", "url", "visible", "zorder"]
        self.check_props(art1, art2, props)

    def test_imshow_creates_modest_image(self):
        """returns a modestImage"""
        data = default_data()
        fig = plt.figure()
        ax = fig.add_subplot(111)

        artist = imshow(ax, data)

        self.assertIsInstance(artist, ModestImage)
        self.assertIn(artist, artist.axes.images)

    def test_imshow_mimics_mpl(self):
        """properties of two axes and artists objects should be same"""

        data = default_data()
        fig = plt.figure()
        ax1 = fig.add_subplot(121)
        ax2 = fig.add_subplot(122)

        artist1 = ax1.imshow(data)
        artist2 = imshow(ax2, data)

        self.check_artist_props(artist1, artist2)
        self.check_axes_props(ax1, ax2)


if __name__ == "__main__":
    unittest.main()
