from __future__ import (absolute_import, division, print_function)
from imopr.sinogram import make_sinogram
import numpy as np


def execute(sample, flat, dark, config, indices):
    from imopr import helper
    helper.print_start("Running IMOPR with action COR")

    from recon.tools import importer
    tool = importer.timed_import(config)

    print("Calculating projection angles..")
    inc = float(config.func.max_angle) / sample.shape[0]
    proj_angles = np.arange(0, sample.shape[0] * inc, inc)
    proj_angles = np.radians(proj_angles)

    print("Converting to sinograms..")
    from imopr.sinogram import make_sinogram
    # sample = make_sinogram(sample)
    print("Reading indices..")
    i1, i2 = helper.handle_indices(indices)
    initial_guess = config.func.cor if config.func.cor is not None else None

    print("Running COR..")
    cor = tool.find_center(
        tomo=sample[:, i1:i2, :],
        theta=proj_angles,
        sinogram_order=True,
        ind=0,
        init=initial_guess)
    print(cor)
    # for i in range(i1, i2):
    #     cor = tool.find_center(
    #         tomo=sample,
    #         theta=proj_angles,
    #         sinogram_order=True,
    #         ind=i,
    #         init=initial_guess)
    #     print(cor)

    return sample
