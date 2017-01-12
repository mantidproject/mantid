from __future__ import (absolute_import, division, print_function)


def execute(data, config):
    """
    Normalize by beam intensity. This is not directly about proton
    charg - not using the proton charge field as usually found in
    experiment/nexus files. This uses an area of normalization, if
    provided in the pre-processing configuration. TODO: much
    of this method should be moved into filters.

    @param data :: stack of images as a 3d numpy array
    @param config :: reconstruction configuration

    Returns :: filtered data (stack of images)

    """
    import numpy as np
    from recon.helper import Helper
    h = Helper(config)
    h.check_data_stack(data)

    normalize_air_region = config.pre.normalize_air_region
    if normalize_air_region:
        if not isinstance(normalize_air_region, list) or \
                4 != len(normalize_air_region):
            raise ValueError(
                "Wrong air region coordinates when trying to use them to normalize images: {0}".
                format(normalize_air_region))

        if not all(
                isinstance(crd, int)
                for crd in normalize_air_region):
            raise ValueError(
                "Cannot use non-integer coordinates to use the normalization region "
                "(air region). Got these coordinates: {0}".format(
                    normalize_air_region))

        right = normalize_air_region[2]
        top = normalize_air_region[1]
        left = normalize_air_region[0]
        bottom = normalize_air_region[3]

        # skip if for example: 0, 0, 0, 0 (empty selection)
        if top >= bottom or left >= right:
            h.tomo_print(" * NOTE: NOT applying Normalise by Air Region. Reason: Empty Selection")
            return data

        h.pstart(" * Starting normalization by air region...")

        air_sums = []
        for idx in range(0, data.shape[0]):
            air_data_sum = data[idx, top:bottom, left:right].sum()
            air_sums.append(air_data_sum)

        air_sums = np.true_divide(air_sums, np.amax(air_sums))

        h.tomo_print(" Air region sums (relative to maximum): {0}".format(air_sums))

        for idx in range(0, data.shape[0]):
            data[idx, :, :] = np.true_divide(data[idx, :, :],
                                             air_sums[idx])

        avg = np.average(air_sums)
        max_avg = np.max(air_sums) / avg
        min_avg = np.min(air_sums) / avg

        h.pstop(
            " * Finished normalization by air region. Average: {0}, max ratio: {1}, min ratio: {2}.".
            format(avg, max_avg, min_avg))

    else:
        h.tomo_print(" * Note: NOT normalizing by air region")

    h.check_data_stack(data)
    return data
