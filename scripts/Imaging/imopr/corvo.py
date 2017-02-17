from __future__ import (absolute_import, division, print_function)
from imopr.sinogram import make_sinogram
import numpy as np


def execute(sample, flat, dark, config, indices):
    from imopr import helper
    helper.print_start(
        "Running IMOPR with action COR using tomopy find_center_vo")

    from recon.tools import importer
    tool = importer.timed_import(config.func.tool)

    inc = float(config.func.max_angle) / sample.shape[0]
    proj_angles = np.arange(0, sample.shape[0] * inc, inc)
    proj_angles = np.radians(proj_angles)

    from imopr.sinogram import make_sinogram
    sample = make_sinogram(sample)

    i1, i2 = helper.handle_indices(indices)

    initial_guess = config.func.cor if config.func.cor is not None else None

    # This works on sinograms by default. 
    # Thankfully it's not said anywhere, but the code gets the sinogram
    # in tomopy.rotation.py find_center_vo(...)
    cor = tool.find_center_vo(
        tomo=sample,
        ind=i1,
        smin=243,
        smax=294,
        srad=20,
        step=1,
        ratio=1.0,
        drop=0)
    print(cor)

    return sample
