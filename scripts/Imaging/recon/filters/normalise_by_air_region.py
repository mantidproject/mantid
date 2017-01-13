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

        air_right = normalize_air_region[2]
        air_top = normalize_air_region[1]
        air_left = normalize_air_region[0]
        air_bottom = normalize_air_region[3]

        # skip if for example: 0, 0, 0, 0 (empty selection)
        if air_top >= air_bottom or air_left >= air_right:
            h.tomo_print(
                " * NOTE: NOT applying Normalise by Air Region. Reason: Empty Selection")
            return data

        crop_coords = config.pre.crop_coords
        crop_right = crop_coords[2]
        crop_top = crop_coords[1]
        crop_left = crop_coords[0]
        crop_bottom = crop_coords[3]

        if air_top < crop_top or \
           air_bottom > crop_bottom or \
           air_left < crop_left or \
           air_right > crop_right:
            raise ValueError(
                "Selected air region is outside of the cropped data range.")

        # move the air region coordinates
        air_left -= crop_left
        air_right -= crop_left
        air_bottom -= crop_top
        air_top -= crop_top

        h.pstart(" * Starting normalization by air region...")
        air_sums = []
        for idx in range(0, data.shape[0]):
            air_data_sum = data[
                idx, air_top:air_bottom, air_left:air_right].sum()
            air_sums.append(air_data_sum)

        air_sums = np.true_divide(air_sums, np.amax(air_sums))

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
