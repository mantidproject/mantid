from __future__ import (absolute_import, division, print_function)
from imopr.sinogram import make_sinogram
import numpy as np


def execute(sample, flat, dark, config, indices):
    from imopr import helper
    helper.print_start(
        "Running IMOPR with action COR using tomopy write_center")

    if not config.func.output_path:
        raise ValueError(
            "The flag -o/--output-path MUST be passed for this IMOPR COR mode!")

    if not len(indices) > 1 or len(indices) != 4:
        raise ValueError(
            "You need to provide input in the format <index_for_recon> <cors_list>[start, end, step]: 31 1 10 1 corwrite"
        )

    from recon.tools import importer
    tool = importer.timed_import(config.func.tool)

    inc = float(config.func.max_angle) / sample.shape[0]
    proj_angles = np.arange(0, sample.shape[0] * inc, inc)
    proj_angles = np.radians(proj_angles)

    from imopr.sinogram import make_sinogram
    sample = make_sinogram(sample)

    initial_guess = config.func.cor if config.func.cor is not None else None

    tool._tomopy.write_center(
        tomo=sample,
        theta=proj_angles,
        dpath=config.func.output_path,
        ind=indices[0],
        sinogram_order=True,
        cen_range=indices[1:])

    return sample
