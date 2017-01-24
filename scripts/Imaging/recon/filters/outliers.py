from __future__ import (absolute_import, division, print_function)
from recon.helper import Helper


def execute(data, outliers_threshold, outliers_mode, h=None):
    # Apply outliers for the normalization?
    h = Helper.empty_init() if h is None else h

    if outliers_threshold and outliers_threshold > 0.0 and outliers_mode is not None:
        import numpy as np
        h.pstart("Applying outliers with level: {0} and mode {1}".format(
            outliers_threshold, outliers_mode))

        min_pix = np.amin(data)
        if outliers_mode == 'both' or outliers_mode == 'dark':
            # anything lower than this
            rel_min = min_pix + (min_pix * outliers_threshold)
        else:
            # this shouldn't clip anything out
            rel_min = min_pix

        max_pix = np.amax(data)

        if outliers_mode == 'both' or outliers_mode == 'bright':
            # anything higher than this
            rel_max = min_pix + (outliers_threshold * (max_pix - min_pix))
        else:
            # this shouldn't clip anything out
            rel_max = max_pix

        data = np.clip(data, rel_min, rel_max)

        h.pstop("Finished outliers step, with pixel data type: {0}.".format(
                data.dtype))
    else:
        h.tomo_print_note(
            "NOT applying outliers, because no --pre-outliers, --pre-outliers-mode, --post-outliers or "
            "--post-outliers-mode was specified.")

    return data
