from __future__ import (absolute_import, division, print_function)


def execute(data, config):
    """
    normalise by beam intensity. This is not directly about proton
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

    normalise_air_region = config.pre.normalise_air_region
    if normalise_air_region:
        if not isinstance(normalise_air_region, list) or \
                4 != len(normalise_air_region):
            raise ValueError(
                "Wrong air region coordinates when trying to use them to normalise images: {0}".
                format(normalise_air_region))

        if not all(
                isinstance(crd, int)
                for crd in normalise_air_region):
            raise ValueError(
                "Cannot use non-integer coordinates to use the normalization region "
                "(air region). Got these coordinates: {0}".format(
                    normalise_air_region))

        air_right, air_top, air_left, air_bottom = translate_coords_onto_cropped_picture(
            config.pre.region_of_interest, normalise_air_region, config)

        h.pstart("Starting normalization by air region...")
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
            "Finished normalization by air region. Average: {0}, max ratio: {1}, min ratio: {2}.".
            format(avg, max_avg, min_avg))

    else:
        h.tomo_print_note(
            "NOT normalizing by air region, because no --air-region coordinates were given.")

    h.check_data_stack(data)
    return data


def translate_coords_onto_cropped_picture(crop_coords, normalise_air_region, config):

    air_right = normalise_air_region[2]
    air_top = normalise_air_region[1]
    air_left = normalise_air_region[0]
    air_bottom = normalise_air_region[3]

    if not config.pre.crop_before_normalise:
        return air_right, air_top, air_left, air_bottom

    crop_right = crop_coords[2]
    crop_top = crop_coords[1]
    crop_left = crop_coords[0]
    crop_bottom = crop_coords[3]

    _check_air_region_in_bounds(air_bottom, air_left, air_right, air_top,
                                crop_bottom, crop_left, crop_right, crop_top)

    # Translate the air region coordinates to the crop.
    air_right -= crop_left
    air_top -= crop_top
    air_left -= crop_left
    air_bottom -= crop_top

    _check_air_region_in_bounds(air_bottom, air_left, air_right, air_top,
                                crop_bottom, crop_left, crop_right, crop_top)

    return air_right, air_top, air_left, air_bottom


def _check_air_region_in_bounds(air_bottom, air_left, air_right, air_top,
                                crop_bottom, crop_left, crop_right, crop_top):
    # sanity check just in case
    if air_top < crop_top or \
            air_bottom > crop_bottom or \
            air_left < crop_left or \
            air_right > crop_right:
        raise ValueError(
            "Selected air region is outside of the cropped data range.")
