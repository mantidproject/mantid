from __future__ import (absolute_import, division, print_function)

import numpy as np


def execute(sample, flat, dark, config, indices):
    from imopr import helper
    from imopr.visualiser import show_3d
    helper.print_start("Running IMOPR with action SINOGRAM")

    if len(indices) == 0:
        show_3d(sample[:], axis=1)
    elif len(indices) == 1:
        show_image(sample[:, indices[0], :])
    else:
        i1, i2 = helper.handle_indices(indices)

        show_3d(sample[i1:i2], axis=1)
    import matplotlib.pyplot as plt
    plt.show()


def make_sinogram(sample):
    return np.swapaxes(sample, 0, 1)
