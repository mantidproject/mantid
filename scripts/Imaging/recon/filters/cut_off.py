from __future__ import (absolute_import, division, print_function)
from recon.helper import Helper


def execute(data, cut_off_level, h=None):
    # Apply cut-off for the normalization?
    h = Helper.empty_init() if h is None else h

    if cut_off_level and cut_off_level > 0.0:
        import numpy as np
        h.pstart("Applying cut-off with level: {0}".format(cut_off_level))
        dmin = np.amin(data)
        dmax = np.amax(data)
        rel_cut_off = dmin + cut_off_level * (dmax - dmin)

        data = np.minimum(data, rel_cut_off)

        h.pstop("Finished cut-off step, with pixel data type: {0}.".format(
                data.dtype))
    else:
        h.tomo_print_note(
            "NOT applying cut-off, because no --cut-off-pre or --cut-off-post was specified.")

    return data
