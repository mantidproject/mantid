from __future__ import (absolute_import, division, print_function)
from imopr.sinogram import make_sinogram
import numpy as np


def execute(sample, flat, dark, config, indices):
    from imopr import helper
    helper.print_start(
        "Running IMOPR with action COR using tomopy find_center_pc")

    from recon.tools import importer
    tool = importer.timed_import(config.func.tool)

    inc = float(config.func.max_angle) / sample.shape[0]
    proj_angles = np.arange(0, sample.shape[0] * inc, inc)
    proj_angles = np.radians(proj_angles)

    from imopr.sinogram import make_sinogram
    sample = make_sinogram(sample)

    i1, i2 = helper.handle_indices(indices)

    initial_guess = config.func.cor if config.func.cor is not None else None

    cor = tool._tomopy.find_center_pc(sample[i1], sample[i2])
    print(cor)

    return sample
