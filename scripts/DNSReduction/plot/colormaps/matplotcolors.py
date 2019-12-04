# -*- coding: utf-8 -*-
"""
Created on Wed Jul 10 14:08:05 2019

@author: thomasm
"""

#import numpy as np
#import matplotlib.pyplot as plt
#import matplotlib.cm as colormaps

colormaps = [
    u'Accent', u'afmhot', u'autumn', u'binary', u'Blues', u'bone', u'BrBG',
    u'brg', u'BuGn', u'BuPu', u'bwr', u'cividis', u'CMRmap', u'cool',
    u'coolwarm', u'copper', u'cubehelix', u'Dark2', u'flag', u'gist_earth',
    u'gist_gray', u'gist_heat', u'gist_ncar', u'gist_rainbow', u'gist_stern',
    u'gist_yarg', u'GnBu', u'gnuplot', u'gnuplot2', u'gray', u'Greens',
    u'Greys', u'hot', u'hsv', u'inferno', u'jet', u'magma', u'nipy_spectral',
    u'ocean', u'Oranges', u'OrRd', u'Paired', u'Pastel1', u'Pastel2', u'pink',
    u'PiYG', u'plasma', u'PRGn', u'prism', u'PuBu', u'PuBuGn', u'PuOr',
    u'PuRd', u'Purples', u'rainbow', u'RdBu', u'RdGy', u'RdPu', u'RdYlBu',
    u'RdYlGn', u'Reds', u'seismic', u'Set1', u'Set2', u'Set3', u'Spectral',
    u'spring', u'summer', u'tab10', u'tab20', u'tab20b', u'tab20c', u'terrain',
    u'viridis', u'winter', u'Wistia', u'YlGn', u'YlGnBu', u'YlOrBr', u'YlOrRd'
]


#def create_images():
#    gradient = np.linspace(0, 1, 256)
#    gradient = np.vstack((gradient, gradient))
#    #colormaps=[m for m in colormaps.cmap_d.keys() if not m.endswith("_r")] ## this returns a mix of unicode and string
#    #colormaps.sort(key=unicode.lower)
#    fig = plt.figure(frameon=False)
#    fig.set_size_inches(20, 1)
#    ax = plt.Axes(fig, [0., 0., 1., 1.])
#    ax.set_axis_off()
#    fig.add_axes(ax)
#    print(colormaps)
#    for m in colormaps:
#        ax.imshow(gradient, cmap=plt.get_cmap(m), aspect='auto')
#        fig.savefig(m + '.png', dpi=15)
