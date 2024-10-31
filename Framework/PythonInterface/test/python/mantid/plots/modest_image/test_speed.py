# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from time import time

from matplotlib import pyplot as plt
import matplotlib.image as mi

import numpy as np

from mantid.plots.modest_image import ModestImage


x, y = np.mgrid[0:40000, 0:1000]
data = np.sin(x / 10.0) * np.cos(y / 30.0)


def setup(img_cls):

    plt.clf()
    plt.cla()
    ax = plt.gca()

    artist = img_cls(ax, data=data)

    ax.add_artist(artist)
    ax.set_aspect("equal")
    artist.norm.vmin = -1
    artist.norm.vmax = 1
    ax.set_xlim(0, 2000)
    ax.set_ylim(0, 2000)

    return artist, time()


def report(label, t0, t1, niter):
    print("%15s: %i ms per operation" % (label, (t1 - t0) * 1000 / niter))


def time_draw(img_cls, repeat=5):
    """render time for first draw after cache clear

    Simulates render after changing data or cmap
    """

    a, t = setup(img_cls)
    for i in range(repeat):
        a.changed()  # clear caches, simulate first render
        a.axes.figure.canvas.draw()
    report("time_draw", t, time(), repeat)


def time_move(img_cls, repeat=10):
    """Render time for moving an image without cache clear"""
    lims = [(0, 2000), (500, 2500)]
    a, t = setup(img_cls)
    ax = plt.gca()
    a.axes.figure.canvas.draw()
    t = time()
    for i in range(repeat):
        ax.set_xlim(lims[i % 2])
        ax.set_ylim(lims[i % 2])
        a.axes.figure.canvas.draw()
    report("time_move", t, time(), repeat)


def time_move_zoom(img_cls, repeat=10):
    """Move image, at high zoom setting"""
    lims = [(0, 100), (500, 600)]
    a, t = setup(img_cls)
    ax = plt.gca()
    a.axes.figure.canvas.draw()
    t = time()
    for i in range(repeat):
        ax.set_xlim(lims[i % 2])
        ax.set_ylim(lims[i % 2])
        a.axes.figure.canvas.draw()
    report("time_move_zoom", t, time(), repeat)


def main():
    print("Test image dimensions: %i x %i" % data.shape)
    for im in [mi.AxesImage, ModestImage]:
        print("**********************************")
        print("Performace Tests for %s" % im.__name__)
        print("**********************************")
        time_draw(im)
        time_move(im)
        time_move_zoom(im)


if __name__ == "__main__":
    main()
